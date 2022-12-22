/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-16
 */

#include "vm/vm.h"

void VM_code_append_(Code *code, const Instruction *seq, u32 sz)
{
    for (int i  = 0; i < sz; i++) {
        const Instruction *ins = &seq[i];
        Vector_push(code, ins->b1);
        if (ins->osz > 1)
            Vector_push(code, ins->b2);
        if (ins->osz > 2)
            Vector_push(code, ins->b3);

        Mode ims = ins->osz == 2? ins->ra : ins->ims;
        if (ins->osz > 1 && (ins->rmd == amImm || ins->iea)) {
            void *dst = Vector_expand(code, vmSizeTbl[ims]);
            VM_write(dst, ins->ii, ims);
        }
    }
}

void* VM_code_append_data_(Code *code, const void *data, u32 sz)
{
    u32 ret = Vector_len(code);
    if (data) Vector_pushArr(code, (u8 *)data, sz);
    else Vector_expand(code, sz);

    return Vector_at(code, ret);
}

void VM_code_disassemble_(Code *code, FILE *fp, bool showAddr)
{
    CodeHeader *header = (CodeHeader *) Vector_at(code, 0);
    u32 ip = header->db;
    while (ip < Vector_len(code)) {
        Instruction instr = {0};
        u32 size;

        if (showAddr) fprintf(fp, "%08d: ", ip);
        size = VM_code_instruction_at(code, &instr, ip);
        VM_code_print_instruction_(&instr, fp);

        ip += size;
        if (size == 0) ip++;

        fputc('\n', fp);
    }
}

u32 VM_code_instruction_at(const Code *code, Instruction *instr, u32 iip) {
    u32 start = iip;
    if (iip + 1 > Vector_len(code)) {
        instr->osz = 0;
        return 0;
    }

    instr->b1 = *Vector_at(code, iip++);

    if ((start + instr->osz) > Vector_len(code)) {
        instr->osz = 0;
        return 0;
    }

    if (instr->osz <= 1)
        return instr->osz;

    instr->b2 = *Vector_at(code, iip++);
    if (instr->osz == 2 && instr->rmd == amImm) {
        instr->ims = instr->ra;
        instr->ra = 0;
    }

    if (instr->osz == 3) {
        instr->b3 = *Vector_at(code, iip++);
    }

    if (instr->rmd == amImm || instr->iea) {
        switch (instr->ims) {
            case szByte:
                instr->ii = (i64) *((i8 *) Vector_at(code, iip));
                ++iip;
                break;
            case szShort:
                instr->ii = *((i16 *) Vector_at(code, iip));
                iip += 2;
                break;
            case szWord:
                instr->ii = *((i32 *) Vector_at(code, iip));
                iip += 4;
                break;
            case szQuad:
                instr->ii = *((i64 *) Vector_at(code, iip));
                iip += 8;
                break;
            default:
                unreachable();
        }
    }
    return iip - start;
}

/*
add r1, 2
cmp r1, 0
jmpeq Exit

Exit:

jmpexitBlock
*/