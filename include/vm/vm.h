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

#include <vm/value.h>

#define VM_INSTRUCTIONS(FF, XX, YY, ZZ) \
    FF(Regular, 0b00000000)         \
    FF(Immediate, 0b01000000)      \
    FF(Extended,  0b10000000)       \
                                    \
    ZZ(opgRegular = ofgRegular)     \
    XX(Halt)                        \
    XX(Load)                        \
    XX(Store)                       \
    XX(Alloc)                       \
    XX(Dealloc)                     \
    XX(Pshsp)                       \
    XX(Pshfp)                       \
    XX(Popsp)                       \
    XX(Popfp)                       \
                                    \
    XX(Binary)                      \
    YY(Add, Binary)                 \
    XX(Sub)                         \
    XX(Mult)                        \
    XX(Div)                         \
    XX(Lt)                          \
    XX(Gt)                          \
    XX(Lte)                         \
    XX(Gte)                         \
    XX(Equal)                       \
    XX(NotEqual)                    \
    XX(LAnd)                        \
    XX(Lor)                         \
    XX(Shl)                         \
    XX(Shr)                         \
    XX(Xor)                         \
    XX(Band)                        \
    XX(Bor)                         \
                                    \
    XX(Unary)                       \
    YY(LNot, Unary)                 \
    XX(BNot)                        \
    XX(EUnary)                      \
                                    \
    ZZ(opgImmediate = ofgImmediate) \
    XX(Pshimm)                      \
    XX(Pshfpimm)                    \
    XX(Pshspimm)                    \
    XX(Popfpimm)                    \
    XX(Popspimm)                    \
    XX(Allocimm)                    \
    XX(Get)                         \
    XX(Set)                         \
    XX(Jump)                        \
    XX(Jumpz)                       \
    XX(Jumpnz)                      \
    XX(Call)                        \
    XX(Return)                      \
                                    \
    ZZ(opgExtended = ofgExtended)   \
                                    \
    ZZ(opgImmExt   = ofgExtended|ofgImmediate) \
    XX(Strimm)                      \


enum {
#define FF(N, V) ofg##N = V,
#define XX(N) op##N,
#define YY(N, V) op##N = op##V,
#define ZZ(...) __VA_ARGS__,
    VM_INSTRUCTIONS(FF, XX, YY, ZZ)
#undef YY
#undef XX
#undef FF
};


#define vmInstrIsExtended(I) (((I) & ofgExtended) == ofgExtended)
#define vmInstrNeedsImm(I)   (((I) & ofgImmediate) == ofgImmediate)
#define vmInstrIsBinary(I)   (((I) >= opBinary) && ((I) < opUnary))
#define vmInstrIsUnary(I)    (((I) >= opUnary) && ((I) <= opEUnary))


typedef struct VmStack {
    Value *base;
    Value *top;
} VmStack;

typedef Vector(u8) Code;

typedef struct VirtualMachine {
    Value *sp;
    Value *bp;
    u32    ip;
    Code   *code;
    VmStack stack;
} VM;


#define vmCheckOverflowX(V, X) ({                       \
    VM *__vm = (V);                                     \
    Value *__sp = __vm->sp;                            \
    do {                                                \
        __vm->sp -= (X);                                \
        if (__vm->sp <= __vm->stack.base)               \
            vmAbort(__vm, "stack overflow");            \
    } while (0);                                        \
    __sp;                                               \
})

#define vmCheckUnderflowX(V, X) ({                      \
    VM *__vm = (V);                                     \
    do {                                                \
        __vm->sp += (X);                                \
        if (__vm->sp >= __vm->stack.top)                \
            vmAbort(__vm, "stack underflow");           \
    } while (0);                                        \
    __vm->sp;                                           \
})

attr(noreturn)
attr(format, printf, 2, 3)
void vmAbort(VM *vm, const char* fmt, ...);

void vmStackTrace(VM *vm);

void vmInit(VM *vm, u32 stackSize);
void vmDeInit(VM *vm);

attr(always_inline)
static Value *vmPushX(VM *vm, u32 x)
{
    return vmCheckOverflowX(vm, x);
}

attr(always_inline)
static void vmPush(VM *vm, Value v)
{
    *vmPushX(vm, 1) = v;
}

attr(always_inline)
static Value *vmPopX(VM *vm, u32 x)
{
    return vmCheckUnderflowX(vm, x);
}
attr(always_inline)
static Value vmPop(VM *vm)
{
    return *vmPopX(vm, 1);
}

attr(always_inline)
static Value *vmStackX(VM *vm, i32 x)
{
    return vm->sp + x;
}

attr(always_inline)
static Value vmStack(VM *vm)
{
    return *vmStackX(vm, 1);
}

attr(always_inline)
static Value *vmBaseX(VM *vm, i32 x)
{
    return vm->bp - x;
}

Value vmAlloc(VM *vm, u32 size);
void vmDealloc(VM *vm, Value obj);
cstring vmInstructionStr(u8 op);


void vmRun(VM *vm, Code *code);

attr(always_inline)
void vmCodeAppendImm_(Code *code, uint8_t op, const f64 *imms, u64 count)
{
    Vector_push(code, op);
    for (int i = 0; i < count; i++) {
        Value val = newVal(imms[i]);
        Vector_pushArr(code, val.b64, sizeof(f64));
    }
}

#define vmCodeAppend(code, op, ...) \
({ f64 LineVAR(imms)[] = { 0, ##__VA_ARGS__, 0}; vmCodeAppendImm_((code), OP_##op, LineVAR(imms)+1, sizeof__(LineVAR(imms))-2); })

#ifdef __cplusplus
}
#endif
