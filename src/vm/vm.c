/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Carter
 * @date 2022-06-22
 */

#include "vm/vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#ifndef CynAlign
#define CynAlign(S, A) (((S) + ((A)-1)) & ~((A)-1))
#endif

#if CYN_TRACE_EXEC
attr(always_inline)
static void vmTrace(VM *vm, u32 iip, const Instruction *instr)
{
    printf("\n%08u:: ", iip);
    vmPrintInstruction(instr);
    fputc('\n', stdout);

    printf("\tsp-regs {ip: %llu, sp: %llu, bp: %llu, flg: %08llx}\n",
           REG(vm, ip), REG(vm, sp), REG(vm, bp), REG(vm, flg));
    printf("\tgp-regs {%08llx, %08llx, %08llx, %08llx, %08llx, %08llx}\n",
           REG(vm, r0), REG(vm, r1), REG(vm, r2), REG(vm, r3), REG(vm, r4), REG(vm, r5));
    printf("\tstack {");
    int i = 0;
    for (int start = REG(vm, sp); start >= REG(vm, bp); start--) {
        printf("%02x", MEM(vm, start));
        if (++i % 8 == 0) printf(" ");
    }
    printf("\b}\n");
}
#endif

void vmAbort(VM *vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fputs("\nerror: ", stderr);
    vfprintf(stderr, fmt, args);
    va_end(args);

#if CYN_DEBUG_TRACE
    fprintf(stderr, "\n---- registers -----\n");
    fprintf(stderr, "\tsp  = %llu\n",   REG(vm, sp));
    fprintf(stderr, "\tip  = %llu\n",   REG(vm, ip));
    fprintf(stderr, "\tbp  = %llu\n",   REG(vm, bp));
    fprintf(stderr, "\tflg = %08llx\n", REG(vm, flg));
    fprintf(stderr, "\n");
    for (int i = r0; i < sp; i++) {
        fprintf(stderr, "\tr%d  = %08llx\n", i, REG(vm, i));
    }

    fprintf(stderr, "\n ----- stack frame ---- ");
    int start = REG(vm, sp), i = 0;
    for (; start >= REG(vm, bp); start--) {
        if (i++ % 8 == 0)
            fprintf(stderr, "\n\t%08x:", start);
        fprintf(stderr, " %02x", MEM(vm, start));
    }
    fprintf(stderr, "\n");
#endif

    abort();
}

void vmExit(VM *vm, i32 code)
{

}

attr(always_inline)
u64 vmFetch(VM *vm, Instruction *instr)
{
    u64 ret = REG(vm, ip);
    if (REG(vm, ip) + 1 > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");

    instr->b1 =  *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);
    if (instr->osz == 1) return ret;

    if (REG(vm, ip) + (instr->osz-1) > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");
    instr->b2 = *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);
    if (instr->osz == 3) {
        instr->b3 = *Vector_at(vm->code, REG(vm, ip));
        ++REG(vm, ip);
    }

    if (instr->rdt) {
        if (instr->osz == 2) {
            // fix instruction
            instr->ims = instr->ra;
            instr->ra = 0;
        }

        switch (instr->ims) {
            case szByte:
                instr->ii = (i64)*((i8 *)Vector_at(vm->code, REG(vm, ip)));
                ++REG(vm, ip);
                break;
            case szShort:
                instr->ii = *((i16 *) Vector_at(vm->code, REG(vm, ip)));
                REG(vm, ip) += 2;
                break;
            case szWord:
                instr->ii = *((i32 *) Vector_at(vm->code, REG(vm, ip)));
                REG(vm, ip) += 4;
                break;
            case szQuad:
                instr->ii = *((i64 *) Vector_at(vm->code, REG(vm, ip)));
                REG(vm, ip) += 8;
                break;
            default:
                vmAbort(vm, "unreachable");
        }
    }

    if (REG(vm, ip) > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");

    return  ret;
}

attr(always_inline)
static void vmExecute(VM *vm, Instruction *instr, u64 iip)
{
    void *rA = NULL, *rB = NULL;
    u16 op = instr->opc << 1;

    vmDbgTrace(EXEC, vmTrace(vm, iip, instr));

    switch (instr->osz) {
        case 1: break;
        case 2:
            if (instr->rdt == dtReg) {
                op |= 1;
                rA = instr->iam ? (void *) &MEM(vm, REG(vm, instr->ra)) : (void *) &REG(vm, instr->ra);
            } else {
                rA = instr->iam ? (void *) &MEM(vm, instr->iu) : (void *)&instr->iu;
            }
            break;
        case 3:
            rA = instr->iam ? (void *) &MEM(vm, REG(vm, instr->ra)) : (void *) &REG(vm, instr->ra);
            if (instr->rdt == dtReg) {
                op |= 1;
                rB = instr->ibm ? (void *) &MEM(vm, REG(vm, instr->rb)) : (void *) &REG(vm, instr->rb);
            } else {
                rB = instr->ibm ? (void *) &MEM(vm, instr->iu) : (void *)&instr->iu;
            }
            break;
        default:
            unreachable();
    }

#define OP_CASES(OP, Apply, ...)                                              \
    case ((OP << 1) | 0b1) : { Apply((instr->dsz), (instr->dsz), ##__VA_ARGS__); break; }  \
    case ((OP << 1) | 0b0) : { Apply((instr->dsz), (instr->ims), ##__VA_ARGS__); break; } \

    switch (op) {
#define BINARY_OPS(XX)      \
        XX(Add, +)          \
        XX(Sub, -)          \
        XX(And, &&)         \
        XX(Or,  ||)         \
        XX(Sar, >>)         \
        XX(Sal, <<)         \
        XX(Xor, ^)          \
        XX(Bor, |)          \
        XX(Band, &)         \
        XX(Mul, *)          \
        XX(Div, /)

#define Apply(TA, TB, OP)   vmWrite(rA, (vmRead(rA, TA) OP vmRead(rB, TB)), TA)
#define XX(N, O) OP_CASES(op##N, Apply, O)
        BINARY_OPS(XX)
#undef XX
#undef Apply

#define ApplyMov(TA, TB) vmWrite(rA, vmRead(rB, TB), TA)
        OP_CASES(opMov, ApplyMov)
#undef ApplyMov

#define ApplyBNot(TA, TB) vmWrite(rA, vmRead(rB, TB), TA)
        OP_CASES(opBNot, ApplyBNot)
#undef ApplyBNot

#define ApplyInc(TA, TB) vmWrite(rA, vmRead(rA, TB) + 1, TA)
        OP_CASES(opInc, ApplyInc)
#undef ApplyInc

#define ApplyDec(TA, TB) vmWrite(rA, vmRead(rA, TB) - 1, TA)
        OP_CASES(opDec, ApplyDec)
#undef ApplyDec

#define ApplyPush(TA, TB) vmPushX(vm, rA, vmSizeTbl[TB])
        OP_CASES(opPush, ApplyPush)
#undef ApplyPush

#define ApplyPop(TA, TB)  vmWrite(rA, vmRead(vmPopS(vm, TB), TB), TA)
        OP_CASES(opPop, ApplyPop)
#undef ApplyPop

#define ApplyJmp(TA, TB)  REG(vm, ip) = iip + vmRead(rA, TB);
        OP_CASES(opJmp, ApplyJmp)
#undef ApplyJmp

#define ApplyJmpz(TA, TB)  if (REG(vm, flg) & flgZero) REG(vm, ip) = iip + vmRead(rA, TB);
        OP_CASES(opJmpz, ApplyJmpz)
#undef ApplyJmpz

#define ApplyJmpnz(TA, TB)  if (!(REG(vm, flg) & flgZero)) REG(vm, ip) = iip + vmRead(rA, TB);
        OP_CASES(opJmpnz, ApplyJmpnz)
#undef ApplyJmpnz

#define ApplyJmpg(TA, TB)  if (REG(vm, flg) & flgGreater) REG(vm, ip) = iip + vmRead(rA, TB);
        OP_CASES(opJmpg, ApplyJmpg)
#undef ApplyJmpg

#define ApplyJmps(TA, TB)  if (REG(vm, flg) & flgSmaller) REG(vm, ip) = iip + vmRead(rA, TB);
        OP_CASES(opJmps, ApplyJmps)
#undef ApplyJmps

#define ApplyCmp(TA, TB)                            \
        i64 a = vmRead(rA, TA), b = vmRead(rB, TB); \
        if (a == b)                                 \
            REG(vm, flg) = flgZero;                 \
        else if (a < b)                             \
            REG(vm, flg) = flgSmaller;              \
        else                                        \
            REG(vm, flg) = flgGreater;

        OP_CASES(opCmp, ApplyCmp)
#undef ApplyCmp

#define ApplyCall(TA, TB)               \
            vmPush(vm, REG(vm, ip));    \
            vmPush(vm, REG(vm, bp));    \
            REG(vm, ip) += vmRead(rA, TB)

        OP_CASES(opCall, ApplyCall)
#undef ApplyCall

#define ApplyPutc(TA, TB)  vmPutUtf8Chr_(vm, vmRead(rA, TB), stdout);
        OP_CASES(opPutc, ApplyPutc)
#undef ApplyPutc

#define ApplyPuti(TA, TB)  printf("%" PRId64 "", vmRead(rA, TB))
        OP_CASES(opPuti, ApplyPuti)
#undef ApplyPuti

#define ApplyPuts(TA, TB)                                   \
            if (instr->iam)                                 \
                fputs(rA, stdout);                          \
            else                                            \
                fputs((void *) vmRead(rA, TB), stdout);
        OP_CASES(opPuts, ApplyPuts)
#undef ApplyPuts

#define ApplyAlloc(TA, TB)  vmWrite(rA, vmAlloc(vm, vmRead(rB, TB)), TA)
        OP_CASES(opAlloc, ApplyAlloc)
#undef ApplyAlloc

#define ApplyDlloc(TA, TB)  vmFree(vm, vmRead(rB, TB))
        OP_CASES(opDlloc, ApplyDlloc)
#undef ApplyDlloc

        case (opRet << 1):
        case (opRet << 1)|0b1:
            REG(vm, ip) = vmPop(vm, u64);
            break;

        case (opHalt << 1):
        case (opHalt << 1) | 0b1:
            vm->halt = true;
            break;
        default:
            vmAbort(vm, "Unknown instruction {%0x|%0x|0x -> %04x}",
                        instr->opc, instr->dsz, instr->ims, op);
    }
}

static void vmMemoryInit(Memory *mem, u64 size, u32 ss, u32 hb)
{
    mem->base = malloc(size);
    mem->size = size;
    mem->top = mem->base + size;
    mem->sb = size - ss;
    mem->hb = hb;
    mem->hlm = (mem->sb - CYN_VM_HEAP_ALIGNMENT);
}

void vmInit_(VM *vm, Code *code, u64 mem, u32 ss)
{
    CodeHeader *header = (CodeHeader *) Vector_at(code, 0);
    mem += header->db;
    mem = CynAlign(mem, CYN_VM_HEAP_ALIGNMENT);
    ss  = CynAlign(ss + CYN_VM_HEAP_ALIGNMENT, CYN_VM_HEAP_ALIGNMENT);

    vmMemoryInit(&vm->ram, mem, ss, header->db);
    vmHeapInit(vm);

    // Copy over the code header and constants to ram
    vm->code = code;
    memcpy(vm->ram.base, header, header->db);
    // first stack word
    for (int i = 0; i < 8; i++)
        MEM(vm, ss-i) = 0xA3;
}

void vmDeInit(VM *vm)
{
    if (vm->ram.base) {
        free(vm->ram.base);
    }
    memset(vm, 0, sizeof(*vm));
}

void vmRun(VM *vm, int argc, char *argv[])
{
    CodeHeader *header = (CodeHeader *) Vector_at(vm->code, 0);
    memset(vm->regs, 0, sizeof(vm->regs));

    REG(vm, sp) = vm->ram.size;
    REG(vm, bp) = vm->ram.size;
    REG(vm, ip) = header->db;

    // and call into command line arguments
    REG(vm, r0) = argc;
    for (int i = 0; i < argc; i++)
        vmPush(vm, argv[i]);
    vmPush(vm, REG(vm, ip));
    vmPush(vm, REG(vm, bp));
    REG(vm, bp) = REG(vm, sp);

    while (!vm->halt && REG(vm, ip) < Vector_len(vm->code))
    {
        Instruction instr = {0};
        u64 iip  = vmFetch(vm, &instr);
        vmExecute(vm, &instr, iip);
    }
}

