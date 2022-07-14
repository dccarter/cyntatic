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

#ifndef CYN_VM_NUM_REGISTERS
#define CYN_VM_NUM_REGISTERS 10
#endif

typedef struct VirtualMachineInstruction {
    u8  opcode: 5;
    u8  dstReg: 3;
    u8  srcType: 1;
    u8  srcSize: 2;
    u8  isDstMem: 1;
    u8  isSrcMem: 1;
    u8  srcReg: 3;
} attr(packed) Instruction;

typedef enum VirtualMachineRegister {
    Reg0,
    Reg1,
    Reg2,
    Reg3,
    Reg4,
    Reg5,
    RegBP, // base pointer for current stack frame
    RegESP,
    RegIp,
    RegFlags
} Register;

typedef enum VirtualMachineSize {
    szByte  = 0b00,
    szShort = 0b01,
    szDWord = 0b10,
    szQWord = 0b11
} Size;


typedef Vector(u8) Code;

typedef struct VirtualMachineStack {
    u8 *base;
    u8 *top;
    u32 size;
} Stack;

typedef struct VirtualMachine {
    u64   regs[CYN_VM_NUM_REGISTERS];
    Code  code;
    Stack stack;
} VM;

void vmAbort(VM *vm, const char *fmt, ...);

attr(always_inline)
void vmFetch(VM *vm, Instruction *instr, u64 *imm)
{
    if (vm->regs[RegIp] + sizeof(Instruction) >= Vector_len(&vm->code))
        vmAbort(vm, "missing code to execute");

    instr = (Instruction *) Vector_at(&vm->code, vm->regs[vm->regs[RegIp]]);
    vm->regs[RegIp] += 2;
    if (instr->srcType) {
        switch (instr->srcSize) {
            case szByte:
                *imm = *Vector_at(&vm->code, vm->regs[vm->regs[RegIp]]);
                ++vm->regs[RegIp];
                break;
            case szShort:
                *imm = *((u16 *) Vector_at(&vm->code, vm->regs[vm->regs[RegIp]]));
                vm->regs[RegIp] += 2;
                break;
            case szDWord:
                *imm = *((u32 *) Vector_at(&vm->code, vm->regs[vm->regs[RegIp]]));
                vm->regs[RegIp] += 4;
                break;
            case szQWord:
                *imm = *((u64 *) Vector_at(&vm->code, vm->regs[vm->regs[RegIp]]));
                vm->regs[RegIp] += 8;
                break;
        }
    }
    else {

    }
}

attr(always_inline)
void vmPushX(VM *vm, const u8 *data, u8 size)
{
    if ((size > sizeof(u64)) || ((size + vm->regs[RegESP]) >= vm->stack.size)) {
        vmAbort(vm, "VM memory error");
    }
    memcpy(&vm->stack.base[vm->regs[RegESP]], data, size);
    vm->regs[RegESP] += size;
}

#define vmPush(vm, val) vmPush((vm), (u8 *)&(val), sizeof(val)))

attr(always_inline)
void vmPopX(VM *vm, u8 *data, u8 size)
{
    if ((size > sizeof(u64)) || ((vm->regs[RegESP] - size) < 0)) {
        vmAbort(vm, "VM memory error");
    }
    memcpy(data, &vm->stack.base[vm->regs[RegESP]], size);
    vm->regs[RegESP] -= size;
}
#define vmPop(vm, T) ({ T LineVAR(p); vmPopX((vm), (u8 *)&LineVAR(p), sizeof(T)); LineVAR(p); })


#ifdef __cplusplus
}
#endif
