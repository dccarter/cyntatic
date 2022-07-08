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


#ifndef CYN_VM_MAX_IMM_COUNT
#define CYN_VM_MAX_IMM_COUNT 8
#endif


#define VM_EXEC_BINARY_INSTR(XX, BB) \
    XX(Add,     +)          \
    XX(Sub,     -)          \
    XX(Mult,    *)          \
    XX(Div,     /)          \
    XX(Lt,      <)          \
    XX(Gt,      >)          \
    XX(Lte,     <=)         \
    XX(Gte,     >=)         \
    XX(Equal,   ==)         \
    XX(NotEqual,!=)         \
    XX(LAnd,    &&)         \
    XX(Lor,     ||)         \
    BB(Shl,     <<)         \
    BB(Shr,     >>)         \
    BB(Xor,     ^)          \
    BB(Band,    &)          \
    BB(Bor,     |)          \


void vmAbort(VM *vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    vmStackTrace(vm);

    abort();
}

attr(noreturn)
void vmExit(VM *vm)
{
    i32 code = (i32) vmPop(vm).val;
    printf("// @TODO hood bye!! %d\n", code);
    exit(code);
}

void vmStackTrace(VM *vm)
{
    fputs("// @TODO implement stack trace!", stderr);
}


void vmInit(VM *vm, u32 stackSize)
{
    vm->stack.base = malloc(sizeof(Value) * stackSize);
    vm->stack.top  = &vm->stack.base[stackSize-1];
    vm->sp = vm->stack.top;
    vmPush(vm, newVal(0xAFAFAFAFAFAFAFAF));
}

void vmDeInit(VM *vm)
{
    if (vm->stack.base != NULL) {
        free(vm->stack.base);
        memset(vm, 0, sizeof(*vm));
    }
}

void vmRun(VM *vm, Code *code)
{
    vm->ip = 0;
#define readByte() Vector_at(code, vm->ip++)
#define readWord()   ({                                 \
    Value __imm = newVal(0);                           \
    do {                                                \
        __imm._bytes[0] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[1] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[2] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[3] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[4] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[5] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[6] = *Vector_at(code, vm->ip++);   \
        __imm._bytes[7] = *Vector_at(code, vm->ip++);   \
    } while (0);                                        \
    __imm;                                              \
})

    while (vm->ip < Vector_len(code)) {
        uint8_t instr = *readByte();
        uint8_t ext = 0, count = 0;
        if (vmInstrIsExtended(instr))
            ext = *readByte();

        Value imm[CYN_VM_MAX_IMM_COUNT] = {newVal(0)};
        if (vmInstrNeedsImm(instr)) {
            count = ext & 0x0F;
            count = MAX(count, 1);
            if (count  > CYN_VM_MAX_IMM_COUNT)
                vmAbort(vm, "to many (%u/%d) immediate values passed to instruction (%x:%u)",
                            count, CYN_VM_MAX_IMM_COUNT, instr, ext);

            for (int i = 0; i < CYN_VM_MAX_IMM_COUNT; i++) {
                imm[i] = readWord();
            }
        }

        if (vmInstrIsBinary(instr)) {
            Value b = vmPop(vm);
            Value a = vmPop(vm);
#define XX(N, OP) case op##N : vmPush(vm, newVal(a.val OP b.val)); break;
#define BB(N, OP) case op##N : vmPush(vm, newVal((uint64_t)a.val OP (uint64_t)b.val)); break;
            switch (instr) {
                VM_EXEC_BINARY_INSTR(XX, BB)
                default:
                    vmAbort(vm, "unsupported binary instruction %x", instr);
            }
            continue;
        }
        else if (vmInstrIsUnary(instr)) {
            Value a = vmPop(vm);
            switch (instr) {
                case opLNot:
                    vmPush(vm, newVal(!a.val));
                    break;
                case opBNot:
                    vmPush(vm, newVal(~((uint64_t)a.val)));
                    break;

                default:
                    vmAbort(vm, "unsupported binary instruction %x", instr);
            }
            continue;
        }

        switch(instr) {
            case opAlloc:
                vmPush(vm, vmAlloc(vm, (u32)vmPop(vm).val));
                break;
            case opDealloc:
                vmDealloc(vm, vmPop(vm));
                break;
            case opStore: {
                u32 size = (u32)vmPop(vm).val;
                Value *val = vmPopX(vm, size);
                memcpy((void *)(uptr)vmPop(vm).val, val, size * sizeof(*val));
                break;
            }
            case opLoad: {
                u32 size = cast(vmPop(vm), u32);
                Value src = vmPop(vm);
                memcpy(vmPushX(vm, size)->b64, src.b64, size * sizeof(src));
                break;
            }
            case opPshimm:
                for (int i = 0; i < count; i++)
                    vmPush(vm, imm[i]);
                break;
            case opStrimm: {
                memcpy(cast(imm[0], void *), imm, count * sizeof (*imm));
                break;
            }
            case opPshspimm:
                vmPush(vm, *vmStackX(vm, cast(imm[0], i32)));
                break;
            case opPopspimm:
                *vmStackX(vm, cast(imm[0], i32)) = vmPop(vm);
                break;
            case opPshfpimm:
                vmPush(vm, *vmBaseX(vm, cast(imm[0], i32)));
                break;
            case opPopfpimm:
                *vmBaseX(vm, cast(imm[0], i32)) = vmPop(vm);
                break;
            case opAllocimm:
                vmPush(vm, vmAlloc(vm, cast(imm[0], u32)));
                break;

            case opGet:
                vmPush(vm, objectGetField(vmPop(vm), cast(imm[0], u32)));
                break;

            case opSet: {
                Value value = vmPop(vm);
                objectSetField(vmPop(vm), cast(imm[0], u32), value);
                break;
            }
        }
    }
}