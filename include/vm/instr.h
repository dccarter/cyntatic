/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-18
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vm/vm.h>

#define B0_(OP, SZ) .osz = (SZ), .opc = (op##OP)
#define BA_(RA, IAM) .ra = (RA), .iam = (IAM)
#define BB_(RB, IBM) .ibm = (IBM), .rb = (RB)
#define BX_(T) .rmd = am##T
#define IMM_(T, N, M) BX_(T)

/**
 * Macro used to encode argument A as a memory reference for a value
 * stored in given register \param N for instructions that take
 * a single argument
 *
 * @example `mRa(r0)`
 */
#define mRa(N)   BA_((N), 1)

/**
 * Macro used to encode argument A as a register for instructions
 * that take a single argument
 *
 * @example `rRa(r0)`
 */
#define rRa(N)   BA_((N), 0)

/**
 * Macro used to encode argument A as a memory reference for a value
 * stored in given register \param N for instructions that takes
 * 2 arguments
 *
 * @example `mRaX(r0)`
 */
#define mRaX(N)  BA_((N), 1), BX_(Reg)

/**
 * Macro used to encode argument A as a register for instructions
 * that takes 2 arguments
 *
 * @example `rRaX(r0)`
 */
#define rRaX(N)  BA_((N), 0), BX_(Reg)

/**
 * Macro used to encode argument B as a memory reference for a value
 * stored in given register \param N
 *
 * @example `mRb(r0)`
 */
#define mRb(N)  BB_(N, 1), BX_(Reg)

/**
 * Macro used to encode argument B as a register
 *
 * @example `rRb(r0)`
 */
#define rRb(N)  BB_(N, 0), BX_(Reg)

/**
 * Macro used to encode an immediate value as a memory reference
 * for argument A
 */
#define mIMa(T, N) .iam = 1, .rmd = amImm, .ra = SZ_(T), .ii = (N)

/**
 * Macro used to encode an immediate value for argument A
 */
#define xIMa(T, N) .iam = 0, .rmd = amImm, .ra = SZ_(T), .ii = (N)

/**
 * Macro used to encode an immediate value as a memory reference
 * for argument B
 */
#define mIMb(T, N) .ibm = 1, .rmd = amImm, .ims = SZ_(T), .ii = (N)

/**
 * Macro used to encode argument 2 as an immediate address
 */
#define rbEA(R, T, N) mRb(R), .ims = SZ_(T), .iea = 1, .ii = (N)

/**
 * Macro used to encode an immediate value for argument B
 */
#define xIMb(T, N) .ibm = 0, .rmd = amImm, .ims = SZ_(T), .ii = (N)

/**
 * Used to change instruction's mode to `.b`
 */
#define dB .imd = szByte

/**
 * Used to change instruction's mode to `.s`
 */
#define dS .imd = szShort

/**
 * Used to change instruction's mode to `.w`
 */
#define dW .imd = szWord

/**
 * Used to change instruction's mode to `.q`
 */
#define dQ .imd = szQuad

/**
 * Instruction to used halt virtual machine
 *
 * @example `halt`
 */
#define cHALT()      ((Instruction) { B0_(Halt, 1)  })

/**
 * `ret` instruction used to return from function calls. This
 * instruction takes a single argument which is the number of
 * arguments returned.
 *
 * @param A the instruction offset to jump to, can be immediate, register or memory reference
 *
 * @example
 * ```
 * cRET(rRa(r0))        // ret r0
 * cRET(mRa(r0))        // ret [r0]
 * cRET(xIMA(u8, 2))    // ret 2
 * ```
 */
#define cRET(A, ...)      ((Instruction) { B0_(Ret,   2),  A, ##__VA_ARGS__})

/**
 * `jmp` instruction used to unconditionally jump to an instruction offset. Takes
 * the offset to jump to, which can be a negative number to jump backwards
 *
 * @param A the instruction offset to jump to, can be immediate, register or memory reference
 *
 * @example
 * ```
 * cJMP(xIMA(i8, -5))   // jmp -5
 * cJMP(xIMA(u8, 5))    // jmp 5
 * cJMP(rRa(ro))        // jmp r0
 * cJMP(mRa(r0))        // jmp [r0]
 * ```
 */
#define cJMP(A, ...)      ((Instruction) { B0_(Jmp,   2),  A, ##__VA_ARGS__})

/**
 * `jmpz` Jump to the given instruction offset if the zero flag (\see `flgZero`) is set.
 * Can be a negative number to jump backwards
 *
 * @param A the instruction offset to jump to, can be immediate, register or memory reference
 *
 * @example
 * ```
 * cJMPZ(xIMA(i8, -5))   // jmpz -5
 * cJMPZ(xIMA(u8, 5))    // jmpz 5
 * cJMPZ(rRa(ro))        // jmpz r0
 * cJMPZ(mRa(r0))        // jmpz [r0]
 * ```
 */
#define cJMPZ(A, ...)     ((Instruction) { B0_(Jmpz,  2),  A, ##__VA_ARGS__})

/**
 * `jmpnz` Jump to the given instruction offset if the zero flag (\see `flgZero`) is
 * not set .Can be a negative number to jump backwards
 *
 * @param A the instruction offset to jump to, can be immediate, register or memory reference
 *
 * @example
 * ```
 * cJMPNZ(xIMA(i8, -5))   // jmpnz -5
 * cJMPNZ(xIMA(u8, 5))    // jmpnz 5
 * cJMPNZ(rRa(ro))        // jmpnz r0
 * cJMPNZ(mRa(r0))        // jmpnz [r0]
 * ```
 */
#define cJMPNZ(A, ...)    ((Instruction) { B0_(Jmpnz, 2),  A, ##__VA_ARGS__})

/**
 * `jmpg` Jump to the given instruction offset if the greater than flag (\see `flgGreater`) is set.
 * Can be a negative number to jump backwards
 *
 * @param A the instruction offset to jump to, can be immediate, register or memory reference
 *
 * @example
 * ```
 * cJMPG(xIMA(i8, -5))   // jmpz -5
 * cJMPG(xIMA(u8, 5))    // jmpz 5
 * cJMPG(rRa(ro))        // jmpz r0
 * cJMPG(mRa(r0))        // jmpz [r0]
 * ```
 */
#define cJMPG(A, ...)     ((Instruction) { B0_(Jmpg,  2),  A, ##__VA_ARGS__})

/**
 * `jmps` Jump to the given instruction offset if the less than flag (\see `flgLess`) is set.
 * Can be a negative number to jump backwards
 *
 * @param A the instruction offset to jump to, can be immediate, register or memory reference
 *
 * @example
 * ```
 * cJMPS(xIMA(i8, -5))   // jmpz -5
 * cJMPS(xIMA(u8, 5))    // jmpz 5
 * cJMPS(rRa(ro))        // jmpz r0
 * cJMPS(mRa(r0))        // jmpz [r0]
 * ```
 */
#define cJMPS(A, ...)     ((Instruction) { B0_(Jmps,  2),  A, ##__VA_ARGS__})

/**
 * `not` instruction operates a logical not `!` operation on the given register
 * or memory reference argument. Note that it does not make sense to apply this
 * to immediate values as the result will be discarded
 *
 * @param A this is the value to perform the not operation on, only register
 * and memory references make sense here
 */
#define cNOT(A, ...)      ((Instruction) { B0_(Not,   2),  A, ##__VA_ARGS__})

/**
 * `bnot` is the binary complement operator `~` which operates on the given
 * value
 *
 * @param A the value whose binary complement will be computed, only register
 * and memory reference arguments make sense here
 */
#define cBNOT(A, ...)     ((Instruction) { B0_(BNot,  2),  A, ##__VA_ARGS__})

/**
 * `inc` increment instruction increases the argument value by 1
 *
 * @param A the value to increase by 1, only register
 * and memory reference arguments make sense here
 */
#define cINC(A, ...)      ((Instruction) { B0_(Inc,   2),  A, ##__VA_ARGS__})

/**
 * `dec` increment instruction decreases the argument value by 1
 *
 * @param A the value to decrease by 1, only register
 * and memory reference arguments make sense here
 */
#define cDEC(A, ...)      ((Instruction) { B0_(Dec,   2),  A, ##__VA_ARGS__})


#define cPUSH(A, ...)     ((Instruction) { B0_(Push,  2),  A, ##__VA_ARGS__})
#define cPOP(A, ...)      ((Instruction) { B0_(Pop,   2),  A, ##__VA_ARGS__})
#define cPOPN(A, ...)     ((Instruction) { B0_(Popn,  2),  A, ##__VA_ARGS__})
#define cPUTI(A, ...)     ((Instruction) { B0_(Puti,  2),  A, ##__VA_ARGS__})
#define cPUTS(A, ...)     ((Instruction) { B0_(Puts,  2),  A, ##__VA_ARGS__})
#define cPUTC(A, ...)     ((Instruction) { B0_(Putc,  2),  A, ##__VA_ARGS__})
#define cNCALL(A, ...)    ((Instruction) { B0_(Ncall, 2),  A, ##__VA_ARGS__})
#define cDLLOC(A, ...)    ((Instruction) { B0_(Dlloc, 2),  A, ##__VA_ARGS__})


#define cALLOCA(A, B, ...)  ((Instruction) { B0_(Alloca,3),  A, B, ##__VA_ARGS__})
#define cRMEM(A, B, ...)    ((Instruction) { B0_(Rmem,  3),  A, B, ##__VA_ARGS__})
#define cMOV(A, B, ...)     ((Instruction) { B0_(Mov,   3),  A, B, ##__VA_ARGS__})
#define cADD(A, B, ...)     ((Instruction) { B0_(Add,   3),  A, B, ##__VA_ARGS__})
#define cSUB(A, B, ...)     ((Instruction) { B0_(Sub,   3),  A, B, ##__VA_ARGS__})
#define cAND(A, B, ...)     ((Instruction) { B0_(And,   3),  A, B, ##__VA_ARGS__})
#define cOR(A, B, ...)      ((Instruction) { B0_(Or,    3),  A, B, ##__VA_ARGS__})
#define cSAR(A, B, ...)     ((Instruction) { B0_(Sar,   3),  A, B, ##__VA_ARGS__})
#define cSAL(A, B, ...)     ((Instruction) { B0_(Sal,   3),  A, B, ##__VA_ARGS__})
#define cXOR(A, B, ...)     ((Instruction) { B0_(Xor,   3),  A, B, ##__VA_ARGS__})
#define cBOR(A, B, ...)     ((Instruction) { B0_(Bor,   3),  A, B, ##__VA_ARGS__})
#define cBAND(A, B, ...)    ((Instruction) { B0_(Band,  3),  A, B, ##__VA_ARGS__})
#define cMUL(A, B, ...)     ((Instruction) { B0_(Mul,   3),  A, B, ##__VA_ARGS__})
#define cDIV(A, B, ...)     ((Instruction) { B0_(Div,   3),  A, B, ##__VA_ARGS__})
#define cMOD(A, B, ...)     ((Instruction) { B0_(Mod,   3),  A, B, ##__VA_ARGS__})
#define cCMP(A, B, ...)     ((Instruction) { B0_(Cmp,   3),  A, B, ##__VA_ARGS__})
#define cCALL(A, B, ...)    ((Instruction) { B0_(Call,  3),  A, B, ##__VA_ARGS__})
#define cALLOC(A, B, ...)   ((Instruction) { B0_(Alloc, 3),  A, B, ##__VA_ARGS__})

#ifdef __cplusplus
}
#endif