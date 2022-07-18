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
        printf("%08d: ", ip);
        instr.b1 = *Vector_at(code, ip++);
        switch (instr.opc) {
#define XX(O, N, ...) case op##O: fputs(#N, fp); fputs(vmSizeNamesTbl[instr.dsz], fp); break;
            VM_OP_CODES(XX)
#undef XX
            default:
                fprintf(fp, "unknown-%u", instr.opc);
                break;
        }

        if (instr.osz == 1)
            continue;

        instr.b2 = *Vector_at(code, ip++);
        if (instr.osz == 2 && instr.rdt == dtImm) {
            instr.ims = instr.ra;
            instr.ra = 0;
        }
        if (instr.osz == 3) {
            instr.b3 = *Vector_at(code, ip++);
        }

        if (instr.rdt == dtImm) {
            switch (instr.ims) {
                case szByte:
                    instr.ii = (i64)*((i8 *)Vector_at(code, ip));
                    ++ip;
                    break;
                case szShort:
                    instr.ii = *((i16 *) Vector_at(code, ip));
                    ip += 2;
                    break;
                case szWord:
                    instr.ii = *((i32 *) Vector_at(code, ip));
                    ip += 4;
                    break;
                case szQuad:
                    instr.ii = *((i64 *) Vector_at(code, ip));
                    ip += 8;
                    break;
                default:
                    unreachable();
            }
        }

        fputs(" ", fp);

        switch (instr.osz) {
            case 2:
                if (instr.iam)
                    fputc('[', fp);
                if (instr.rdt == dtReg)
                    fputs(vmRegisterNameTbl[instr.ra], fp);
                else
                    fprintf(fp, "%lld", instr.ii);
                if (instr.iam)
                    fputc(']', fp);
                break;
            case 3:
                if (instr.iam)
                    fputc('[', fp);
                fputs(vmRegisterNameTbl[instr.ra], fp);
                if (instr.iam)
                    fputc(']', fp);

                fputc(' ', fp);

                if (instr.ibm)
                    fputc('[', fp);
                if (instr.rdt == dtReg)
                    fputs(vmRegisterNameTbl[instr.rb], fp);
                else
                    fprintf(fp, "%lld", instr.ii);
                if (instr.ibm)
                    fputc(']', fp);
                break;
            default:
                unreachable();
        }

        fputc('\n', fp);
    }
}
