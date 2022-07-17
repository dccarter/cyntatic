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

const u8 vmSizeTbl[] = {1, 2, 4, 8};
const char* vmSizeNamesTbl[] = {".b", ".s", ".w", ".q"};

attr(always_inline)
static void printInstruction(const Instruction *instr)
{
    switch (instr->opc) {
#define XX(O, N, ...) case op##O: fputs(#N, stdout); fputs(vmSizeNamesTbl[instr->dsz], stdout); break;
        VM_OP_CODES(XX)
#undef XX
        default:
            printf("op-%u", instr->opc);
            break;
    }
    if (instr->osz > 1) {
        printf(", ra: %u, iam: %s, type: %s, dsz: %u",
               instr->ra,
               (instr->iam ? "true" : "false"),
               (instr->type? "immediate" : "register"),
               instr->dsz);
    }
    if (instr->osz > 2) {
        printf(", ibm: %s, rb %u, ims: %u", (instr->ibm ? "true" : "false"), instr->rb, instr->ims);
    }
    if (instr->type == dtImm)
        printf(", imm: %lld", instr->ii);
}

attr(always_inline)
static void vmTrace(VM *vm, const Instruction *instr)
{
#if CYN_DEBUG_TRACE
    printf("::instr: {");
    printInstruction(instr);
    printf("}\n");

    printf("::regs: {ip: %llu, sp: %llu, bp: %llu, flg: %08llx}\n",
           REG(vm, ip), REG(vm, sp), REG(vm, bp), REG(vm, flg));
    printf("::regs: {%08llx, %08llx, %08llx, %08llx, %08llx, %08llx}\n",
           REG(vm, r0), REG(vm, r1), REG(vm, r2), REG(vm, r3), REG(vm, r4), REG(vm, r5));
    printf("::stack: {");
    int i = 0;
    for (int start = REG(vm, sp); start >= REG(vm, bp); start--) {
        printf("%02x", MEM(vm, start));
        if (++i % 8 == 0) printf(" ");
    }
    printf("\b}\n");
#endif
}

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
void vmFetch(VM *vm, Instruction *instr)
{
    if (REG(vm, ip) + 1 > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");

    instr->b1 =  *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);
    if (instr->osz == 1) return;

    if (REG(vm, ip) + (instr->osz-1) > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");
    instr->b2 = *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);

    if (instr->osz == 3) {
        instr->b3 = *Vector_at(vm->code, REG(vm, ip));
        ++REG(vm, ip);
    }

    if (instr->type) {
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
}

attr(always_inline)
i64 vmRead(const void *src, Size size)
{
    switch (size) {
        case szByte:  return *((i8 *)src);
        case szShort: return *((i16 *)src);
        case szWord:  return *((i32 *)src);
        case szQuad:  return *((i64 *)src);
        default:
            unreachable("!!!!");
    }
}

void vmWrite(void *dst, i64 src, Size size)
{
    switch (size) {
        case szByte:
            *((i8 *)dst) = (i8)src;
            break;
        case szShort:
            *((i16 *)dst) = (i16)src;
            break;
        case szWord:
            *((i32 *)dst) = (i32)src;
            break;
        case szQuad:
            *((i64 *)dst) = (i64)src;
            break;
        default:
            unreachable("!!!!");
    }
}

attr(always_inline)
static void vmExecute(VM *vm, Instruction *instr)
{
    void *rA = NULL, *rB = NULL;
    u16 op = instr->opc << 1;

    vmTrace(vm, instr);

    switch (instr->osz) {
        case 1: break;
        case 2:
            if (instr->type == dtReg) {
                op |= 1;
                rA = instr->iam ? (void *) &MEM(vm, REG(vm, instr->ra)) : (void *) &REG(vm, instr->ra);
            } else {
                rA = instr->iam ? (void *) &MEM(vm, instr->iu) : (void *)&instr->iu;
            }
            break;
        case 3:
            rA = instr->iam ? (void *) &MEM(vm, REG(vm, instr->ra)) : (void *) &REG(vm, instr->ra);
            if (instr->type == dtReg) {
                op |= 1;
                rB = instr->ibm ? (void *) &MEM(vm, REG(vm, instr->rb)) : (void *) &REG(vm, instr->rb);
            } else {
                rB = instr->ibm ? (void *) &MEM(vm, instr->iu) : (void *)&instr->iu;
            }
            break;
        default:
            unreachable();
    }

//#define OP_CASES(op, Apply, ...)                                              \
//    case ((op << 4) | 0b0000) : Apply(u8,  i8,  u64, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0001) : Apply(u8,  i8,  u8,  ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0010) : Apply(u16, i64, u64, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0011) : Apply(u16, i16, u16, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0100) : Apply(u32, i32, u64, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0101) : Apply(u32, i32, u32, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0110) : Apply(u64, i64, u64, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b0111) : Apply(u64, i64, u64, ##__VA_ARGS__); break;   \
//    case ((op << 4) | 0b1000)...                                              \
//         ((op << 4) | 0b1111) : Apply(i64, i64, i64, ##__VA_ARGS__); break;

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

#define ApplyInc(TA, TB) vmWrite(rA, vmRead(rA, TA) + 1, TA)
        OP_CASES(opInc, ApplyInc)
#undef ApplyInc

#define ApplyDec(TA, TB) vmWrite(rA, vmRead(rA, TA) - 1, TA)
        OP_CASES(opDec, ApplyDec)
#undef ApplyDec

#define ApplyPush(TA, TB) vmPushX(vm, rA, vmSizeTbl[TA])
        OP_CASES(opPush, ApplyPush)
#undef ApplyPush

#define ApplyPop(TA, TB)  vmWrite(rA, vmRead(vmPopS(vm, TA), TA), TA)
        OP_CASES(opPop, ApplyPop)
#undef ApplyPop

#define ApplyJmp(TA, TB)  REG(vm, ip) += vmRead(rA, TB);
        OP_CASES(opJmp, ApplyJmp)
#undef ApplyJmp

#define ApplyJmpz(TA, TB)  if (REG(vm, flg) & flgZero) REG(vm, ip) += vmRead(rA, TB);
        OP_CASES(opJmpz, ApplyJmpz)
#undef ApplyJmpz

#define ApplyJmpnz(TA, TB)  if (!(REG(vm, flg) & flgZero)) REG(vm, ip) += vmRead(rA, TB);
        OP_CASES(opJmpnz, ApplyJmpnz)
#undef ApplyJmpnz

#define ApplyJmpg(TA, TB)  if (REG(vm, flg) & flgGreater) REG(vm, ip) += vmRead(rA, TB);
        OP_CASES(opJmpg, ApplyJmpg)
#undef ApplyJmpg

#define ApplyJmps(TA, TB)  if (REG(vm, flg) & flgSmaller) REG(vm, ip) += vmRead(rA, TB);
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

        case (opRet << 1):
            REG(vm, ip) = vmPop(vm, u64);
            break;

        case (opPuts << 1):
            fputs(rA, stdout);
            break;
        case (opHalt << 1):
            vm->halt = true;
            break;
        default:
            vmAbort(vm, "Unknown instruction {%0x|%0x|0x -> %04x}",
                        instr->opc, instr->dsz, instr->ims, op);
    }
}

static void vmMemoryInit(Memory *mem, u64 size, u32 ss)
{
    mem->base = malloc(size);
    mem->size = size;
    mem->top = mem->base + size;
    mem->sb = size - ss;
    mem->hb = mem->sb - 8;
}

void vmInit_(VM *vm, u64 mem, u32 ss)
{
    mem = CynAlign(mem, 8);
    ss  = CynAlign(ss + 8, 8);

    memset(vm->regs, 0, sizeof(vm->regs));
    vm->code = NULL;
    vmMemoryInit(&vm->ram, mem, ss);
    REG(vm, sp) = mem;
    REG(vm, bp) = mem;
    for (int i = 0; i < 8; i++)
        MEM(vm, ss-i) = 0xA3;
}

void vmPutUtf8Chr_(VM *vm, u32 chr, FILE *fp)
{
    if (chr < 0x80) {
        fputc((char)chr, fp);
    }
    else if (chr < 0x800) {
        char c[] = {(char)(0xC0|(chr >> 6)),  (char)(0x80|(chr &  0x3F)), '\0'};
        fputs(c, fp);
    }
    else if (chr < 0x10000) {
        char c[] = {
                (char)(0xE0 | (chr >> 12)),
                (char)(0x80 | ((chr >> 6) & 0x3F)),
                (char)(0x80 | (chr & 0x3F)),
                '\0'
        };
        fputs(c, fp);
    }
    else if (chr < 0x200000) {
        char c[] = {
                (char)(0xF0 | (chr >> 18)),
                (char)(0x80 | ((chr >> 12) & 0x3F)),
                (char)(0x80 | ((chr >> 6) & 0x3F)),
                (char)(0x80 | (chr & 0x3F)),
                '\0'
        };
        fputs(c, fp);
    }
    else if (vm) {
        vmAbort(vm, "invalid UCS character: \\U%08x", chr);
    }
    else {
        unreachable("!!!invalid UCS character: \\U%08x", chr);
    }
}

void vmDeInit(VM *vm)
{
    if (vm->ram.base) {
        free(vm->ram.base);
    }
    memset(vm, 0, sizeof(*vm));
}

void vmRun(VM *vm, Code *code, int argc, char *argv[])
{
    vm->code = code;
    memset(vm->regs, 0, sizeof(vm->regs));
    CodeHeader *header = (CodeHeader *) Vector_at(code, 0);
    memcpy(vm->ram.base, header, header->db);
    REG(vm, sp) = vm->ram.size;
    REG(vm, bp) = vm->ram.size;
    REG(vm, ip) = header->db;

    // and call into command line arguments
    for (int i = 0; i < argc; i++)
        vmPush(vm, argv[i]);
    vmPush(vm, (u64)argc);
    vmPush(vm, REG(vm, ip));
    vmPush(vm, REG(vm, bp));
    REG(vm, bp) = REG(vm, sp);

    while (!vm->halt && REG(vm, ip) < Vector_len(vm->code))
    {
        Instruction instr = {0};
        vmFetch(vm, &instr);
        vmExecute(vm, &instr);
    }
}

