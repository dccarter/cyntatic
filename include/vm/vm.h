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
#extern "C" {
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
            u8 osz: 2;
            u8 opc: 6;
        };
        u8 b1;
    };
    union {
        struct attr(packed) {
            u8 ra: 4;
            u8 iam: 1;
            u8 type: 1;
            u8 size: 2;
        };
        u8 b2;
    };
    union {
        struct attr(packed) {
            u8 ibm: 1;
            u8 rb: 4;
            u8 u0: 3;
        };
        u8 b3;
    };

    u64 imm;
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

typedef enum VirtualMachineSize {
    szByte  = 0b00,
    szShort = 0b01,
    szDWord = 0b10,
    szQWord = 0b11
} Size;

#define szu64_ szQWord
#define szu32_ szDWord
#define szu16_ szShort
#define szu8_  szDWord

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

#define VM_OP_CODES(XX)     \
    XX(Halt)                \
                            \
    XX(Ret)                 \
    XX(Jmp)                 \
    XX(Jmpz)                \
    XX(Jmpnz)               \
    XX(Jmpg)                \
    XX(Jmps)                \
    XX(Not)                 \
    XX(BNot)                \
    XX(Inc)                 \
    XX(Dec)                 \
                            \
    XX(Call)                \
    XX(Push)                \
    XX(Pop)                 \
    XX(Puti)                \
    XX(Puts)                \
    XX(Putc)                \
    XX(Memcpy)              \
    XX(Syscall)             \
                            \
    XX(Mov)                 \
    XX(Add)                 \
    XX(Sub)                 \
    XX(And)                 \
    XX(Or)                  \
    XX(Sar)                 \
    XX(Sal)                 \
    XX(Xor)                 \
    XX(Bor)                 \
    XX(Band)                \
    XX(Mul)                 \
    XX(Div)                 \
    XX(Mod)                 \
    XX(Cmp)                 \

typedef enum VirtualMachineOpCodes {
#define XX(N) op##N,
    VM_OP_CODES(XX)
#undef XX
} OpCodes;

typedef Vector(u8) Code;

typedef struct VirtualMachineMemory {
    u8 *base;
    u8 *top;
    u64 sb;
    u64 hb;
    u32 size;
} Memory;

typedef struct VirtualMachine {
    u64   regs[CYN_VM_NUM_REGISTERS];
    Code *code;
    Memory ram;
} VM;

#define REG(V, R) (vm)->regs[(R)]
#define MEM(V, O) (vm)->ram.base[(0)]

void vmAbort(VM *vm, const char *fmt, ...);

attr(always_inline)
void vmPushX(VM *vm, const u8 *data, u8 size)
{

    if ((size > sizeof(u64)) || ((REG(vm, sp) - size) <= vm->ram.sb)) {
        vmAbort(vm, "VM stack memory overflow - collides with heap boundary");
    }

    REG(vm, sp) -= size;
    memcpy(&MEM(vm, REG(vm, sp)), data, size);
    printf("%08llx: %08llx\n", REG(vm, sp), *(u64 *)&MEM(vm, REG(vm, sp)));
}

#define vmPush(vm, val) ({ typeof(val) LineVAR(v) = (val); vmPushX((vm), (u8 *)&(LineVAR(v)), sizeof(LineVAR(v))); })

attr(always_inline)
void vmPopX(VM *vm, u8 *data, u8 size)
{
    if ((size > sizeof(u64)) || ((REG(vm, sp) + size) > vm->ram.size)) {
        vmAbort(vm, "VM stack memory underflow - escapes virtual machine memory");
    }
    memcpy(data, &MEM(vm, REG(vm, sp)), size);
    REG(vm, sp) += size;
}

#define vmPop(vm, T) ({ T LineVAR(p); vmPopX((vm), (u8 *)&LineVAR(p), sizeof(T)); LineVAR(p); })

void vmInit_(VM *vm, u64 mem, u32 ss);
#define vmInit(V, S) vmInit_((V), (S), CYN_VM_DEFAULT_SS)
void vmRun(VM *vm, Code *code, int argc, char *argv[]);
void vmDeInit(VM *vm);


typedef struct {
    Instruction instr;
    u64 imm;
} Code_;

/*
 *  u8  ra: 4;
    u8  isRaMem: 1;
    u8  type: 1;
    u8  size: 2;
*/
#define B0_(OP, SZ) .osz = (SZ), .opc = (op##OP)
#define B1_(RA, IAM, T, SZ) .ra = (RA), .iam = (IAM), .type = dt##T, .size = (u8)(SZ)
#define BA_(RB, IAM) .ra = (RA), .iam = (IAM)
#define BB_(RB, IBM) .ibm = (RB), .rb = (rb)
#define BX_(T) .type = dt##T

#define mRa(N)   BA_((N), 1)
#define rRa(N)   BA_((N), 0)
#define mRaX(N)  BA_((N), 1), BX_(Reg)
#define rRaX(N)  BA_((N), 0), BX_(Reg)
#define mRb(N)   BB_(N, 1)
#define rRb(N)   BB_(N, 0)
#define mRbX(N)  BB_(N, 1), BX_(Reg)
#define rRbX(N)  BB_(N, 0), BX_(Reg)

#define mIM(T, N) B1_(0, 1, Reg, SZ_(T)), .imm = (N)
#define xIM(T, N) B1_(0, 0, Reg, SZ_(T)), .imm = (N)

#define HALT()      ((Instruction) { B0_(Halt, 1) })
#define RET(N)      ((Instruction) { B0_(Ret, 1), N})
#define PUSH(N)     ((Instruction) { B0_(Push, 1), N})

void vmCodeAppend_(Code *code, const Instruction *seq, u32 sz);

#define vmCodeAppend(C, INS, ...) \
    ({Instruction LineVAR(cc)[] = {(INS), ##__VA_ARGS__}; vmCodeAppend_((C), LineVAR(cc), sizeof__(LineVAR(cc))); })


#ifdef __cplusplus
}
#endif
