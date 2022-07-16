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

attr(always_inline)
static void printInstruction(const Instruction *instr, u64 imm)
{
    switch (instr->opc) {
#define XX(O) case op##O: fputs(#O, stdout); break;
        VM_OP_CODES(XX)
#undef XX
        default:
            printf("op-%u", instr->opc);
            break;
    }
    if (instr->osz > 1) {
        printf(", ra: %u, raIsMem: %s, type: %s, size: %u",
               instr->ra,
               (instr->iam ? "true" : "false"),
               (instr->type? "immediate" : "register"),
               instr->size);
    }
    if (instr->osz > 2) {
        printf(", rbIsMem: %s, rb %u", (instr->ibm ? "true" : "false"), instr->rb);
    }
    if (instr->type == dtImm)
        printf(", imm: %llu", imm);
}

attr(always_inline)
static void vmTrace(VM *vm, const Instruction *instr, u64 imm)
{
#ifdef CYN_DEBUG_TRACE
    printf("::instr: {");
    printInstruction(instr, imm);
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
    vfprintf(stderr, fmt, args);
    va_end(args);

#ifdef CYN_DEBUG_TRACE
    printf("\n---- registers -----\n");
    printf("\tsp  = %llu\n",   REG(vm, sp));
    printf("\tip  = %llu\n",   REG(vm, ip));
    printf("\tbp  = %llu\n",   REG(vm, bp));
    printf("\tflg = %08llx\n", REG(vm, flg));
    printf("\n");
    for (int i = r0; i < sp; i++) {
        printf("\tr%d  = %08llx\n", i, REG(vm, i));
    }

    printf("\n ----- stack frame ---- ");
    int start = REG(vm, sp), i = 0;
    for (; start >= REG(vm, bp); start--) {
        if (i++ % 8 == 0)
            printf("\n\t%08x: ", start);
        printf("%02x", MEM(vm, start));
    }
#endif



    abort();
}

void vmExit(VM *vm, i32 code)
{

}

attr(always_inline)
u64 vmFetch(VM *vm, Instruction *instr)
{
    u64 imm = 0;
    if (REG(vm, ip) + 1 > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");

    instr->b1 =  *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);
    if (instr->osz == 1) return imm;

    if (REG(vm, ip) + instr->osz > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");
    instr->b2 = *Vector_at(vm->code, REG(vm, ip));
    ++REG(vm, ip);

    if (instr->osz == 3) {
        instr->b3 = *Vector_at(vm->code, REG(vm, ip));
        ++REG(vm, ip);
    }

    if (instr->type) {
        switch (instr->size) {
            case szByte:
                imm = *Vector_at(vm->code, REG(vm, ip));
                ++REG(vm, ip);
                break;
            case szShort:
                imm = *((u16 *) Vector_at(vm->code, REG(vm, ip)));
                REG(vm, ip) += 2;
                break;
            case szDWord:
                imm = *((u32 *) Vector_at(vm->code, REG(vm, ip)));
                REG(vm, ip) += 4;
                break;
            case szQWord:
                imm = *((u64 *) Vector_at(vm->code, REG(vm, ip)));
                REG(vm, ip) += 8;
                break;
            default:
                vmAbort(vm, "unreachable");
        }
    }

    if (REG(vm, ip) > Vector_len(vm->code))
        vmAbort(vm, "execution goes beyond code space");

    return imm;
}

attr(always_inline)
static void vmExecute(VM *vm, Instruction *instr, u64 imm)
{
    bool isDstReg = false;
    void *rA = NULL, *rB = NULL;

    vmTrace(vm, instr, imm);

#define OP_CASES(op, Apply, ...)                         \
    case ((op << 3) | 0b000) : Apply(u8, i8, u64, ##__VA_ARGS__);   break;  \
    case ((op << 3) | 0b001) : Apply(u8, i8, u8, ##__VA_ARGS__);    break;  \
    case ((op << 3) | 0b010) : Apply(u16, i16, u64, ##__VA_ARGS__); break;  \
    case ((op << 3) | 0b011) : Apply(u16, i16, u16, ##__VA_ARGS__); break;  \
    case ((op << 3) | 0b100) : Apply(u32, i32, u64, ##__VA_ARGS__); break;  \
    case ((op << 3) | 0b101) : Apply(u32, i32, u32, ##__VA_ARGS__); break;  \
    case ((op << 3) | 0b110) : Apply(u64, i64, u64, ##__VA_ARGS__); break;  \
    case ((op << 3) | 0b111) : Apply(u64, i64, u64, ##__VA_ARGS__); break;  \

    switch ((instr->opc << 3) | (instr->size << 1) | isDstReg) {
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

#define Apply(TA, TB, TD, OP)   *((TD *)rA) = *((TA *)rA) OP *((TB *)rB)
#define XX(N, O) OP_CASES(op##N, Apply, O)
        BINARY_OPS(XX)
#undef XX
#undef Apply

#define ApplyNot(TA, TB, TD) *((TA *)rA) = !*((TA *)rA)
        OP_CASES(opNot, ApplyNot)
#undef ApplyNot

#define ApplyBNot(TA, TB, TD) *((TA *)rA) = ~*((TA *)rA)
        OP_CASES(opBNot, ApplyBNot)
#undef ApplyBNot

#define ApplyInc(TA, TB, TD) *((TA *)rA) += 1
        OP_CASES(opInc, ApplyInc)
#undef ApplyInc

#define ApplyDec(TA, TB, TD) *((TA *)rA) -= 1
        OP_CASES(opDec, ApplyDec)
#undef ApplyDec

#define ApplyPush(TA, TB, TD) vmPush(vm, *((TA *)rA))
        OP_CASES(opPush, ApplyPush)
#undef ApplyPush

#define ApplyPop(TA, TB, TD)  *((TA *)rA) = vmPop(vm, TA);
        OP_CASES(opPop, ApplyPop)
#undef ApplyPop

#define ApplyJmp(TA, TB, TD)  REG(vm, ip) += *((TB *)rA)
        OP_CASES(opJmp, ApplyJmp)
#undef ApplyJmp

#define ApplyJmpz(TA, TB, TD)  if (REG(vm, flg) & flgZero) REG(vm, ip) += *((TB *)rA)
        OP_CASES(opJmpz, ApplyJmpz)
#undef ApplyJmpz

#define ApplyJmpnz(TA, TB, TD)  if (!(REG(vm, flg) & flgZero)) REG(vm, ip) += *((TB *)rA)
        OP_CASES(opJmpnz, ApplyJmpnz)
#undef ApplyJmpnz

#define ApplyJmpg(TA, TB, TD)  if (REG(vm, flg) & flgGreater) REG(vm, ip) += *((TB *)rA)
        OP_CASES(opJmpg, ApplyJmpg)
#undef ApplyJmpg

#define ApplyJmps(TA, TB, TD)  if (REG(vm, flg) & flgSmaller) REG(vm, ip) += *((TB *)rA)
        OP_CASES(opJmps, ApplyJmps)
#undef ApplyJmps

#define ApplyCmp(TA, TB, TD)                \
        if (*((TA *)rA) == *((TA *)rB))     \
            REG(vm, flg) = flgZero;            \
        else if (*((TA *)rA) < *((TA *)rB)) \
            REG(vm, flg) = flgSmaller;         \
        else                                \
            REG(vm, flg) = flgGreater;

        OP_CASES(opCmp, ApplyCmp)
#undef ApplyCmp

#define ApplyCall(TA, TB, TD)           \
            vmPush(vm, REG(vm, ip));    \
            vmPush(vm, REG(vm, bp));    \
            REG(vm, ip) += *((TB *)rA)

        OP_CASES(opCall, ApplyCall)
#undef ApplyCall

#define ApplyPutc(TA, TB, TD)  fputc(*((TA *)rA), stdout)
        OP_CASES(opPutc, ApplyPutc)
#undef ApplyPutc

#define ApplyPuti(TA, TB, TD)  printf("%" PRIu64 "", (i64)*((TB *)rA))
        OP_CASES(opPuti, ApplyPuti)
#undef ApplyPuti

        case (opRet << 3):
            REG(vm, ip) = vmPop(vm, u64);
            break;

        case (opPuts << 3):
            fputs(rA, stdout);
            break;
        case (opHalt << 3):
            vmExit(vm, vmPop(vm, i32));
            break;
        default:
            vmAbort(vm, "Unknown instruction {%d-%d-%d}", instr->opc, instr->size, isDstReg);
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

    REG(vm, sp) = vm->ram.size;
    REG(vm, bp) = vm->ram.size;

    // and call into command line arguments
    for (int i = 0; i < argc; i++)
        vmPush(vm, argv[i]);
    vmPush(vm, (u64)argc);
    vmPush(vm, REG(vm, ip));
    vmPush(vm, REG(vm, bp));
    REG(vm, bp) = REG(vm, sp);

    while (REG(vm, ip) < Vector_len(vm->code))
    {
        Instruction instr = {0};
        u64 imm = vmFetch(vm, &instr);
        vmExecute(vm, &instr, imm);
    }
}

#define vmCodeAppendImm(C, T, I) \
    ({                                                          \
        T LineVAR(imm) = (T)(I);                                \
        u32 LineVAR(tag) = Vector_len(code);                    \
        Vector_pushArr((C), (u8*)&LineVAR(imm), sizeof(T));     \
        *((T*)Vector_at((C), LineVAR(tag))) = LineVAR(imm);     \
    })

void vmCodeAppend_(Code *code, const Instruction *seq, u32 sz)
{
    for (int i  = 0; i < sz; i++) {
        const Instruction *ins = &seq[i];
        Vector_push(code, ins->b1);
        if (ins->osz > 1)
            Vector_push(code, ins->b2);
        if (ins->osz > 2)
            Vector_push(code, ins->b3);

        if (seq[i].type == dtImm) {
            switch (seq[i].size) {
                case szByte:
                    vmCodeAppendImm(code, u8, seq[i].imm);
                    break;
                case szShort:
                    vmCodeAppendImm(code, u16, seq[i].imm);
                    break;
                case szDWord:
                    vmCodeAppendImm(code, u32, seq[i].imm);
                    break;
                case szQWord:
                default:
                    vmCodeAppendImm(code, u32, seq[i].imm);
                    break;
            }
        }
    }
}
