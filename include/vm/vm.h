/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Carter
 * @date 2022-06-22
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common.h>
#include <vector.h>
#include <stdio.h>

#ifndef CYN_VM_NUM_REGISTERS
#define CYN_VM_NUM_REGISTERS 10
#endif

#ifndef CYN_VM_DEFAULT_SS
#define CYN_VM_DEFAULT_SS (sizeof(u64) * 1024)  // 8k stack by default
#endif

#ifndef CYN_VM_DEFAULT_MS
#define CYN_VM_DEFAULT_MS (1024 * 1024)         // default memory size 1MB
#endif

typedef struct VirtualMachineInstruction {
    union {
        struct attr(packed) {
            u8 osz:2;
            u8 opc:6;
        };
        u8 b1;
    };
    union {
        struct attr(packed) {
            u8 ra:4;
            u8 iam:1;
            u8 rdt:1;
            u8 dsz:2;
        };
        u8 b2;
    };
    union {
        struct attr(packed) {
            u8 ims:2;
            u8 rb:4;
            u8 ibm:1;
            u8 u0:1;
        };
        u8 b3;
    };

    union {
        u64 iu;
        i64 ii;
    };
} attr(packed) Instruction;

typedef enum VirtualMachineRegister {
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    sp,
    ip,
    bp,
    flg
} Register;
extern const char *vmRegisterNameTbl[];

typedef enum VirtualMachineSize {
    szByte  = 0b00,
    szShort = 0b01,
    szWord = 0b10,
    szQuad = 0b11
} Size;

#define szu64_ szQuad
#define szu32_ szWord
#define szu16_ szShort
#define szu8_  szByte

extern const u8 vmSizeTbl[];
extern const char* vmSizeNamesTbl[];

typedef union {
    f32 f;
    u32 u;
    u8  _b[4];
} FltU32;

typedef union {
    f64 f;
    u64 u;
    u8  _b[8];
} Flt64;

#define f2u64(V) ({ Flt64 LineVAR(f) = {.f = (V)}; LineVAR(f).u; })
#define u2f64(V) ({ Flt64 LineVAR(u) = {.u = (V)}; LineVAR(u).v; })

#define SZ_(T)  CynPST(CynPST(sz, T), _)

typedef enum VirtualMachineDataType {
    dtReg,
    dtImm
} DataType;

typedef enum VirtualMachineFlags {
    flgZero     = 0b10000000,
    flgSmaller  = 0b01000000,
    flgGreater  = 0b00100000
} Flags;

#define VM_OP_CODES(XX)             \
    XX(Halt,  halt, 0)                 \
                                       \
    XX(Ret,   ret, 1)                  \
    XX(Jmp,   jmp, 1)                  \
    XX(Jmpz,  jmpz, 1)                 \
    XX(Jmpnz, jmpnz, 1)                \
    XX(Jmpg,  jmpg, 1)                 \
    XX(Jmps,  jmps, 1)                 \
    XX(Not,   lnot, 1)                 \
    XX(BNot,  bnot, 1)                 \
    XX(Inc,   inc, 1)                  \
    XX(Dec,   dec, 1)                  \
                                       \
    XX(Call,  call, 1)                 \
    XX(Push,  push, 1)                 \
    XX(Pop,   pop, 1)                  \
    XX(Puti,  puti, 1)                 \
    XX(Puts,  puts, 1)                 \
    XX(Putc,  putc, 1)                 \
    XX(Mcpy,  mcpy, 1)                 \
    XX(scall, scall, 1)                \
                                        \
    XX(Mov,   mov, 2)                  \
    XX(Add,   add, 2)                  \
    XX(Sub,   sub, 2)                  \
    XX(And,   and, 2)                  \
    XX(Or,    or, 2)                   \
    XX(Sar,   sar, 2)                  \
    XX(Sal,   sal, 2)                  \
    XX(Xor,   xor, 2)                  \
    XX(Bor,   bor, 2)                  \
    XX(Band,  band, 2)                 \
    XX(Mul,   mul, 2)                  \
    XX(Div,   div, 2)                  \
    XX(Mod,   mod, 2)                  \
    XX(Cmp,   cmp, 2)                  \

typedef enum VirtualMachineOpCodes {
#define XX(N, ...) op##N,
    VM_OP_CODES(XX)
#undef XX
} OpCodes;

typedef Vector(u8) Code;

typedef struct VirtualMachineCodeHeader {
    u64 size;
    u32 db;
    u32 main;
    u8  code[0];
} attr(packed) CodeHeader;

typedef struct VirtualMachineMemory {
    u8 *base;
    u8 *top;
    u64 sb;
    u64 hb;
    u32 size;
} Memory;

typedef struct VirtualMachine {
    bool halt;
    u64   regs[CYN_VM_NUM_REGISTERS];
    Code *code;
    Memory ram;
} VM;

#define REG(V, R) (vm)->regs[(R)]
#define MEM(V, O) (vm)->ram.base[(O)]

void vmAbort(VM *vm, const char *fmt, ...);

attr(always_inline)
void* vmPushX(VM *vm, const u8 *data, u8 size)
{

    if ((size > sizeof(u64)) || ((REG(vm, sp) - size) <= vm->ram.sb)) {
        vmAbort(vm, "VM stack memory overflow - collides with heap boundary");
    }

    REG(vm, sp) -= size;
    if (data != NULL)
        memcpy(&MEM(vm, REG(vm, sp)), data, size);

    return &MEM(vm, REG(vm, sp));
}

#define vmPush(vm, val) ({ *((typeof(val) *)vmPushX((vm), NULL, sizeof(val))) = (val); })
#define vmPushS(vm, S) vmPushX((vm), NULL, vmSizeTbl[(S)])

attr(always_inline)
void* vmPopX(VM *vm, u8 *data, u8 size)
{
    void* ret;
    if ((size > sizeof(u64)) || ((REG(vm, sp) + size) > vm->ram.size)) {
        vmAbort(vm, "VM stack memory underflow - escapes virtual machine memory");
    }

    ret = &MEM(vm, REG(vm, sp));
    if (data != NULL)
        memcpy(data, ret, size);
    REG(vm, sp) += size;
    return ret;
}

#define vmPop(vm, T) ({ *((T *)vmPopX((vm), NULL, sizeof(T))); })
#define vmPopS(vm, S) vmPopX((vm), NULL, vmSizeTbl[(S)])

void vmInit_(VM *vm, u64 mem, u32 ss);
#define vmInit(V, S) vmInit_((V), (S), CYN_VM_DEFAULT_SS)
void vmRun(VM *vm, Code *code, int argc, char *argv[]);
void vmDeInit(VM *vm);

u32  vmAlloc(VM *vm, u32 size);
void vmFree(VM *vm, u32 mem);

attr(always_inline)
Size vmIntegerSize(u64 imm)
{
    if (imm <= 0xFF) return szByte;
    if (imm <= 0xFFFF) return szShort;
    if (imm <= 0xFFFFFFFF) return szWord;
    return szQuad;
}

attr(always_inline)
static i64 vmRead(const void *src, Size size)
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

attr(always_inline)
static void vmWrite(void *dst, i64 src, Size size)
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

#define B0_(OP, SZ) .osz = (SZ), .opc = (op##OP)
#define BA_(RA, IAM) .ra = (RA), .iam = (IAM)
#define BB_(RB, IBM) .ibm = (IBM), .rb = (RB)
#define BX_(T) .rdt = dt##T
#define IMM_(T, N, M) BX_(T)

#define mRa(N)   BA_((N), 1)
#define rRa(N)   BA_((N), 0)
#define mRaX(N)  BA_((N), 1), BX_(Reg)
#define rRaX(N)  BA_((N), 0), BX_(Reg)
#define mRb(N)  BB_(N, 1), BX_(Reg)
#define rRb(N)  BB_(N, 0), BX_(Reg)

#define mIM(T, N) .rdt = dtReg, .ims = SZ_(T), .ii = (N)
#define xIM(T, N) .rdt = dtImm, .ims = SZ_(T), .ii = (N)

#define cHALT()      ((Instruction) { B0_(Halt, 1)  })

#define dB .dsz = szByte
#define dS .dsz = szShort
#define dW .dsz = szWord
#define dQ .dsz = szQuard

#define cRET(A, ...)      ((Instruction) { B0_(Ret,   2),  A, ##__VA_ARGS__})
#define cJMP(A, ...)      ((Instruction) { B0_(Jmp,   2),  A, ##__VA_ARGS__})
#define cJMPZ(A, ...)     ((Instruction) { B0_(Jmpz,  2),  A, ##__VA_ARGS__})
#define cJMPNZ(A, ...)    ((Instruction) { B0_(Jmpnz, 2),  A, ##__VA_ARGS__})
#define cJMPG(A, ...)     ((Instruction) { B0_(Jmpg,  2),  A, ##__VA_ARGS__})
#define cJMPS(A, ...)     ((Instruction) { B0_(Jmps,  2),  A, ##__VA_ARGS__})
#define cNOT(A, ...)      ((Instruction) { B0_(Not,   2),  A, ##__VA_ARGS__})
#define cBNOT(A, ...)     ((Instruction) { B0_(BNot,  2),  A, ##__VA_ARGS__})
#define cINC(A, ...)      ((Instruction) { B0_(Inc,   2),  A, ##__VA_ARGS__})
#define cDEC(A, ...)      ((Instruction) { B0_(Dec,   2),  A, ##__VA_ARGS__})

#define cCALL(A)     ((Instruction) { B0_(Call,  2),  A})
#define cPUSH(A)     ((Instruction) { B0_(Push,  2),  A})
#define cPOP(A)      ((Instruction) { B0_(Pop,   2),  A})
#define cPUTI(A)     ((Instruction) { B0_(Puti,  2),  A})
#define cPUTS(A)     ((Instruction) { B0_(Puts,  2),  A})
#define cPUTC(A)     ((Instruction) { B0_(Putc,  2),  A})
#define cSCALL(Z)    ((Instruction) { B0_(scall, 2),  A})


#define cMOV(A, B, ...)   ((Instruction) { B0_(Mov,   3),  A, B, ##__VA_ARGS__})
#define cADD(A, B, ...)   ((Instruction) { B0_(Add,   3),  A, B, ##__VA_ARGS__})
#define cSUB(A, B, ...)   ((Instruction) { B0_(Sub,   3),  A, B, ##__VA_ARGS__})
#define cAND(A, B, ...)   ((Instruction) { B0_(And,   3),  A, B, ##__VA_ARGS__})
#define cOR(A, B, ...)    ((Instruction) { B0_(Or,    3),  A, B, ##__VA_ARGS__})
#define cSAR(A, B, ...)   ((Instruction) { B0_(Sar,   3),  A, B, ##__VA_ARGS__})
#define cSAL(A, B, ...)   ((Instruction) { B0_(Sal,   3),  A, B, ##__VA_ARGS__})
#define cXOR(A, B, ...)   ((Instruction) { B0_(Xor,   3),  A, B, ##__VA_ARGS__})
#define cBOR(A, B, ...)   ((Instruction) { B0_(Bor,   3),  A, B, ##__VA_ARGS__})
#define cBAND(A, B, ...)  ((Instruction) { B0_(Band,  3),  A, B, ##__VA_ARGS__})
#define cMUL(A, B, ...)   ((Instruction) { B0_(Mul,   3),  A, B, ##__VA_ARGS__})
#define cDIV(A, B, ...)   ((Instruction) { B0_(Div,   3),  A, B, ##__VA_ARGS__})
#define cMOD(A, B, ...)   ((Instruction) { B0_(Mod,   3),  A, B, ##__VA_ARGS__})
#define cCMP(A, B, ...)   ((Instruction) { B0_(Cmp,   3),  A, B, ##__VA_ARGS__})


void vmCodeAppend_(Code *code, const Instruction *seq, u32 sz);

#define vmCodeAppend(C, INS, ...) \
    ({Instruction LineVAR(cc)[] = {(INS), ##__VA_ARGS__}; vmCodeAppend_((C), LineVAR(cc), sizeof__(LineVAR(cc))); })

void vmCodeDisassemble(Code *code, FILE *fp);


void vmPutUtf8Chr_(VM *vm, u32 chr, FILE *fp);
#define putUtf8Chr(CH, FP) vmPutUtf8Chr_(NULL, (CH), FP)

#ifdef __cplusplus
}
#endif
