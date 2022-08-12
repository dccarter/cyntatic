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
#include "vm/builtins.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef CYN_VM_DEBUG_TRACE
attr(always_inline)
static void VM_trace(VM *vm, u32 iip, const Instruction *instr)
{
    printf("\n%08u:: ", iip);
    VM_code_print_instruction(instr);
    fputc('\n', stdout);

    printf("\tsp-regs {ip: %" PRIx64 ", sp: %" PRIx64 ", bp: %" PRIx64 ", flg: %08" PRIx64 "}\n",
           REG(vm, ip), REG(vm, sp), REG(vm, bp), REG(vm, flg));
    printf("\tgp-regs {%08" PRIx64 ", %08" PRIx64 ", %08" PRIx64 ", %08" PRIx64 ", %08" PRIx64 ", %08" PRIx64 "}\n",
           REG(vm, r0), REG(vm, r1), REG(vm, r2), REG(vm, r3), REG(vm, r4), REG(vm, r5));
    printf("\tstack {");
    for (int start = REG(vm, sp); start < REG(vm, bp); start += CYN_VM_ALIGNMENT) {
        printf(" %08" PRIx64 "", ((Value *)MEM(vm, start))->i);
    }
    printf(" }\n");
}
#endif

void VM_abort(VM *vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fputs("\nerror: ", stderr);
    vfprintf(stderr, fmt, args);
    va_end(args);

#ifdef CYN_VM_DEBUG_TRACE
    fprintf(stderr, "\n---- registers -----\n");
    fprintf(stderr, "\tsp  = %" PRIx64 "\n",   REG(vm, sp));
    fprintf(stderr, "\tip  = %" PRIx64 "\n",   REG(vm, ip));
    fprintf(stderr, "\tbp  = %" PRIx64 "\n",   REG(vm, bp));
    fprintf(stderr, "\tflg = %" PRIx64 "\n", REG(vm, flg));
    fprintf(stderr, "\n");
    for (int i = r0; i < sp; i++) {
        fprintf(stderr, "\tr%d  = %" PRIx64 "\n", i, REG(vm, i));
    }

    fprintf(stderr, "\n ----- stack frame ---- ");
    int start = REG(vm, sp), i = 0;
    for (; start >= REG(vm, bp); start--) {
        if (i++ % 8 == 0)
            fprintf(stderr, "\n\t%08x:", start);
        fprintf(stderr, " %02x", *MEM(vm, start));
    }
    fprintf(stderr, "\n");
#endif

    abort();
}

void vmThrowError(VM *vm, i32 code)
{}

attr(always_inline)
static u64 VM_fetch(VM *vm, Instruction *instr)
{
    u64 ret = REG(vm, ip);
    if (REG(vm, ip) + 1 > Vector_len(vm->code))
        VM_abort(vm, "execution goes beyond code space");

    instr->b1 =  *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);
    if (instr->osz == 1) return ret;

    if (REG(vm, ip) + (instr->osz-1) > Vector_len(vm->code))
        VM_abort(vm, "execution goes beyond code space");
    instr->b2 = *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);
    if (instr->osz == 3) {
        instr->b3 = *Vector_at(vm->code, REG(vm, ip));
        ++REG(vm, ip);
    }

    if (instr->rmd || instr->iea) {
        if (instr->osz == 2) {
            // fix instruction
            instr->ims = instr->ra;
            instr->ra = 0;
        }

        instr->ii = VM_read(Vector_at(vm->code, REG(vm, ip)), instr->ims);
        REG(vm, ip) += vmSizeTbl[instr->ims];
    }

    if (REG(vm, ip) > Vector_len(vm->code))
        VM_abort(vm, "execution goes beyond code space");

    return  ret;
}

attr(always_inline)
static void VM_execute(VM *vm, Instruction *instr, u64 iip)
{
    void *rA = NULL, *rB = NULL;
    u16 op = instr->opc << 1;

    VM_dbg_trace(vm, trcEXEC, VM_trace(vm, iip, instr));

    switch (instr->osz) {
        case 1: break;
        case 2:
            if (instr->rmd == amReg) {
                op |= 1;
                rA = instr->iam ? (void *) MEM(vm, REG(vm, instr->ra)) : (void *) &REG(vm, instr->ra);
            } else {
                rA = instr->iam ? (void *) MEM(vm, instr->iu) : (void *)&instr->iu;
            }
            break;
        case 3:
            rA = instr->iam ? (void *) MEM(vm, REG(vm, instr->ra)) : (void *) &REG(vm, instr->ra);
            if (instr->rmd == amReg) {
                op |= 1;
                if (instr->ibm) {
                    rB = ((MEM(vm, REG(vm, instr->rb))) + (instr->iea? instr->ii : 0));
                }
                else {
                    rB = (void *) &REG(vm, instr->rb);
                }
            } else {
                rB = instr->ibm ? (void *) MEM(vm, instr->iu) : (void *)&instr->iu;
            }
            break;
        default:
            unreachable();
    }

#define OP_CASES(OP, Apply, ...)                                              \
    case ((OP << 1) | 0b1) : { Apply((instr->imd), (instr->imd), ##__VA_ARGS__); break; }  \
    case ((OP << 1) | 0b0) : { Apply((instr->imd), (instr->ims), ##__VA_ARGS__); break; } \

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

#define Apply(TA, TB, OP)   VM_write(rA, (VM_read(rA, TA) OP VM_read(rB, TB)), TA)
#define XX(N, O) OP_CASES(op##N, Apply, O)
        BINARY_OPS(XX)
#undef XX
#undef Apply

#define ApplyMov(TA, TB) VM_write(rA, VM_read(rB, TB), TA)
        OP_CASES(opMov, ApplyMov)
#undef ApplyMov

#define ApplyRmem(TA, TB) VM_write(rA, (uptr)MEM(vm, VM_read(rB, TB)), TA)
        OP_CASES(opRmem, ApplyRmem)
#undef ApplyRmem

#define ApplyNot(TA, TB) VM_write(rA, !VM_read(rA, TB), TA)
        OP_CASES(opNot, ApplyNot)
#undef ApplyNot

#define ApplyBNot(TA, TB) VM_write(rA, ~VM_read(rA, TB), TA)
        OP_CASES(opBNot, ApplyBNot)
#undef ApplyBNot

#define ApplyInc(TA, TB) VM_write(rA, VM_read(rA, TB) + 1, TA)
        OP_CASES(opInc, ApplyInc)
#undef ApplyInc

#define ApplyDec(TA, TB) VM_write(rA, VM_read(rA, TB) - 1, TA)
        OP_CASES(opDec, ApplyDec)
#undef ApplyDec

#define ApplyPush(TA, TB) VM_push(vm, VM_read(rA, TB))
        OP_CASES(opPush, ApplyPush)
#undef ApplyPush

#define ApplyAlloca(TA, TB)                           \
        u32 count = VM_read(rB, TB) >> (szQuad - TA);  \
        VM_write(rA, REG(vm, sp)-8, szQuad);           \
        VM_pushn(vm, NULL, count);
        OP_CASES(opAlloca, ApplyAlloca)
#undef ApplyAlloca

#define ApplyPop(TA, TB)  VM_write(rA, VM_pop(vm, i64), TB)
        OP_CASES(opPop, ApplyPop)
#undef ApplyPop

#define ApplyPopn(TA, TB) VM_popn(vm, NULL, VM_read(rA, TB))
        OP_CASES(opPopn, ApplyPopn)
#undef ApplyPopn

#define ApplyJmp(TA, TB)  REG(vm, ip) = iip + VM_read(rA, TB);
        OP_CASES(opJmp, ApplyJmp)
#undef ApplyJmp

#define ApplyJmpz(TA, TB)  if (REG(vm, flg) & flgZero) REG(vm, ip) = iip + VM_read(rA, TB);
        OP_CASES(opJmpz, ApplyJmpz)
#undef ApplyJmpz

#define ApplyJmpnz(TA, TB)  if (!(REG(vm, flg) & flgZero)) REG(vm, ip) = iip + VM_read(rA, TB);
        OP_CASES(opJmpnz, ApplyJmpnz)
#undef ApplyJmpnz

#define ApplyJmpg(TA, TB)  if (REG(vm, flg) & flgGreater) REG(vm, ip) = iip + VM_read(rA, TB);
        OP_CASES(opJmpg, ApplyJmpg)
#undef ApplyJmpg

#define ApplyJmps(TA, TB)  if (REG(vm, flg) & flgLess) REG(vm, ip) = iip + VM_read(rA, TB);
        OP_CASES(opJmps, ApplyJmps)
#undef ApplyJmps

#define ApplyCmp(TA, TB)                            \
        i64 a = VM_read(rA, TA), b = VM_read(rB, TB); \
        if (a == b)                                 \
            REG(vm, flg) = flgZero;                 \
        else if (a < b)                             \
            REG(vm, flg) = flgLess;                 \
        else                                        \
            REG(vm, flg) = flgGreater;

        OP_CASES(opCmp, ApplyCmp)
#undef ApplyCmp

#define ApplyCall(TA, TB)               \
            VM_push(vm, REG(vm, ip));    \
            VM_push(vm, REG(vm, bp));    \
            REG(vm, bp) = REG(vm, sp);  \
            REG(vm, ip) = iip + VM_read(rA, TB)
        OP_CASES(opCall, ApplyCall)
#undef ApplyCall

#define ApplyRet(TA, TB)                            \
            u32 nret =  VM_read(rA, TB), nargs = 0;  \
            Value *ret = NULL;                      \
            if (nret) ret = VM_popn(vm, NULL, nret); \
            REG(vm, sp) = REG(vm, bp);              \
            REG(vm, bp) = VM_pop(vm, u64);           \
            REG(vm, ip) = VM_pop(vm, u64);           \
            nargs = VM_pop(vm, u32);                 \
            if (nargs) VM_popn(vm, NULL, nargs);     \
            if (nret)  VM_pushn(vm, ret, nret);      \
            VM_push(vm, nret);
        OP_CASES(opRet, ApplyRet)
#undef ApplyRet

#define ApplyNcall(TA, TB)                                          \
            uptr id = (uptr)VM_read(rA, TB);                         \
            NativeCall fn = (id < bncCOUNT)?                        \
                    vmNativeBuiltinCallTbl[id] : (NativeCall)id;    \
            Value *nargs = (Value*) MEM(vm, REG(vm, sp));           \
            Value *argv = (nargs->i == 0)? NULL :                   \
                        ((Value *)MEM(vm, (REG(vm, sp) + (nargs->i << 3)))); \
            VM_push(vm, REG(vm, ip));                                \
            VM_push(vm, REG(vm, bp));                                \
            REG(vm, bp) = REG(vm, sp);                              \
            fn(vm, argv, nargs->i);
        OP_CASES(opNcall, ApplyNcall)
#undef ApplyNcall

#define ApplyPutc(TA, TB)  VM_put_utf8_chr_(vm, VM_read(rA, TB), stdout);
        OP_CASES(opPutc, ApplyPutc)
#undef ApplyPutc

#define ApplyPuti(TA, TB)  printf("%" PRId64 "", VM_read(rA, TB))
        OP_CASES(opPuti, ApplyPuti)
#undef ApplyPuti

#define ApplyPuts(TA, TB)                                   \
            if (instr->iam)                                 \
                fputs(rA, stdout);                          \
            else                                            \
                fputs((void *) VM_read(rA, TB), stdout);
        OP_CASES(opPuts, ApplyPuts)
#undef ApplyPuts

#define ApplyAlloc(TA, TB)  VM_write(rA, VM_alloc(vm, VM_read(rB, TB)), TA)
        OP_CASES(opAlloc, ApplyAlloc)
#undef ApplyAlloc

#define ApplyDlloc(TA, TB)  VM_free(vm, VM_read(rA, TB))
        OP_CASES(opDlloc, ApplyDlloc)
#undef ApplyDlloc

        case (opHalt << 1):
        case (opHalt << 1) | 0b1:
            vm->flags = eflHalt;
            break;
        case (opDbg << 1):
        case (opDbg << 1) | 0b1:
            break;
        default:
            VM_abort(vm, "Unknown instruction {%0x|%0x|%0x -> %04x}",
                    instr->opc, instr->imd, instr->ims, op);
    }
}

void VM_returnx(VM *vm, Value *vals, u32 count)
{
    u32 nargs;
    REG(vm, sp) = REG(vm, bp);
    REG(vm, bp) = VM_pop(vm, u64);
    REG(vm, ip) = VM_pop(vm, u64);
    nargs = VM_pop(vm, u32);
    if (nargs) VM_popn(vm, NULL, nargs);
    if (count)  VM_pushn(vm, vals, count);
    VM_push(vm, count);
}

static void VM_memory_init(Memory *mem, u64 size, u32 bk, u32 ss, u32 db)
{
    mem->ptr = malloc(size);
    mem->base = mem->ptr + bk;
    mem->size = size - bk;
    mem->sb = mem->size - ss;
    mem->hb = db;
    mem->hlm = (mem->sb - CYN_VM_ALIGNMENT);
}

void VM_init_(VM *vm, Code *code, u64 mem, u32 nhbs, u32 ss)
{
    u32 bk;
    CodeHeader *header = (CodeHeader *) Vector_at(code, 0);

    memset(vm, 0, sizeof(*vm));

    bk = CynAlign((sizeof(Heap) + sizeof(HeapBlock) * nhbs), CYN_VM_ALIGNMENT);
    mem += header->db + bk;

    mem = CynAlign(mem, CYN_VM_ALIGNMENT);
    ss  = CynAlign(ss + CYN_VM_ALIGNMENT, CYN_VM_ALIGNMENT);

    VM_memory_init(&vm->ram, mem, bk, ss, header->db);
    VM_heap_init(vm, nhbs);

    // Copy over the code header and constants to ram
    vm->code = code;
    memcpy(vm->ram.base, header, header->db);
    // first stack word
    for (int i = 0; i < 8; i++)
        *MEM(vm, ss-i) = 0xA3;

    vm->flags = 0;
}

void VM_deinit(VM *vm)
{
    if (vm->ram.base) {
        free(vm->ram.ptr);
    }
    memset(vm, 0, sizeof(*vm));
}

void VM_run(VM *vm, int argc, char *argv[])
{
    CodeHeader *header = (CodeHeader *) Vector_at(vm->code, 0);
    memset(vm->regs, 0, sizeof(vm->regs));

    REG(vm, sp) = vm->ram.size;
    REG(vm, bp) = vm->ram.size;
    REG(vm, ip) = header->db;

    // and call into command line arguments
    REG(vm, r0) = argc;
    for (int i = 0; i < argc; i++)
        VM_push(vm, VM_cstring_dup(vm, argv[i]));
    VM_push(vm, argc);
    VM_push(vm, Vector_len(vm->code));
    VM_push(vm, REG(vm, bp));
    REG(vm, bp) = REG(vm, sp);

    while (REG(vm, ip) < Vector_len(vm->code))
    {
        Instruction instr = {0};
        u64 iip  = VM_fetch(vm, &instr);

#if defined(CYN_VM_DEBUGGER)
        if (instr.opc == opDbg || vm->flags & eflDbgBreak) {
            if (vm->debugger)
                vm->debugger(vm, iip, &instr);
        }
        VM_execute(vm, &instr, iip);
#else
        VM_execute(vm, &instr, iip);
#endif
        if (vm->flags & eflHalt)
            break;
    }
}
