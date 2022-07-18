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

        if (seq[i].rdt == dtImm) {
            switch (seq[i].ims) {
                case szByte:
                    vmCodeAppendImm(code, u8, seq[i].iu);
                    break;
                case szShort:
                    vmCodeAppendImm(code, u16, seq[i].iu);
                    break;
                case szWord:
                    vmCodeAppendImm(code, u32, seq[i].iu);
                    break;
                case szQuad:
                default:
                    vmCodeAppendImm(code, u32, seq[i].iu);
                    break;
            }
        }
    }
}

void vmCodeDisassemble(Code *code, FILE *fp)
{
    CodeHeader *header = (CodeHeader *) Vector_at(code, 0);
    u32 ip = header->db;
    while (ip < Vector_len(code)) {
        Instruction instr;
        u32 size;

        fprintf(fp, "%08d: ", ip);
        size = vmCodeInstructionAt(code, &instr, ip);
        vmPrintInstruction_(&instr, fp);

        ip += size;
        if (size == 0) ip++;

        fputc('\n', fp);
    }
}

u32 vmCodeInstructionAt(const Code *code, Instruction *instr, u32 iip) {
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

    if (instr->osz == 1)
        return 1;

    instr->b2 = *Vector_at(code, iip++);
    if (instr->osz == 2 && instr->rdt == dtImm) {
        instr->ims = instr->ra;
        instr->ra = 0;
    }
    if (instr->osz == 3) {
        instr->b3 = *Vector_at(code, iip++);
    }

    if (instr->rdt == dtImm) {
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
