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

        if (seq[i].type == dtImm) {
            switch (seq[i].size) {
                case szByte:
                    vmCodeAppendImm(code, u8, seq[i].iu);
                    break;
                case szShort:
                    vmCodeAppendImm(code, u16, seq[i].iu);
                    break;
                case szDWord:
                    vmCodeAppendImm(code, u32, seq[i].iu);
                    break;
                case szQWord:
                default:
                    vmCodeAppendImm(code, u32, seq[i].iu);
                    break;
            }
        }
    }
}
