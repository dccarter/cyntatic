/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-07-18
 */

#include "vm/vm.h"

const u8 vmSizeTbl[] = {1, 2, 4, 8};

const char* vmSizeNamesTbl[] = {".b", ".s", ".w", ".q"};

const char *vmRegisterNameTbl[] = {
        "r0", "r1", "r2", "r3", "r4", "r5",
        "sp", "ip", "bp", "flg"
};

const char* vmInstructionNamesTbl[] = {
#define XX(OP, N, ...) #N,
        VM_OP_CODES(XX)
#undef XX
};

void vmPrintInstruction_(const Instruction* instr, FILE *fp)
{
    if (instr->osz == 0) {
        fputs("...null...\n", fp);
        return;
    }

    if (instr->opc >= nOPS) {
        fprintf(fp, "not-sup.%u\n", instr->opc);
        return;
    }

    fputs(vmInstructionNamesTbl[instr->opc], fp);
    fputs(vmSizeNamesTbl[instr->dsz], fp);

    if (instr->osz == 1) {
        return;
    }

    fputs(" ", fp);

    switch (instr->osz) {
        case 2:
            if (instr->iam)
                fputc('[', fp);
            if (instr->rdt == dtReg)
                fputs(vmRegisterNameTbl[instr->ra], fp);
            else
                fprintf(fp, "%lld", instr->ii);
            if (instr->iam)
                fputc(']', fp);
            break;
        case 3:
            if (instr->iam)
                fputc('[', fp);
            fputs(vmRegisterNameTbl[instr->ra], fp);
            if (instr->iam)
                fputc(']', fp);

            fputc(' ', fp);

            if (instr->ibm)
                fputc('[', fp);
            if (instr->rdt == dtReg)
                fputs(vmRegisterNameTbl[instr->rb], fp);
            else
                fprintf(fp, "%lld", instr->ii);
            if (instr->ibm)
                fputc(']', fp);
            break;
        default:
            printf("Unsupported size of %u.%u\n", instr->opc, instr->osz);
            unreachable();
    }
}

void vmPutUtf8Chr_(VM *vm, u32 chr, FILE *fp)
{
    if (chr < 0x80) {
        fputc((char)chr, fp);
    }
    else if (chr < 0x800) {
        char c[] = {(char)(0xC0|(chr >> 6)),  (char)(0x80|(chr &  0x3F)), '\0'};
        fputs(c, fp);
    }
    else if (chr < 0x10000) {
        char c[] = {
                (char)(0xE0 | (chr >> 12)),
                (char)(0x80 | ((chr >> 6) & 0x3F)),
                (char)(0x80 | (chr & 0x3F)),
                '\0'
        };
        fputs(c, fp);
    }
    else if (chr < 0x200000) {
        char c[] = {
                (char)(0xF0 | (chr >> 18)),
                (char)(0x80 | ((chr >> 12) & 0x3F)),
                (char)(0x80 | ((chr >> 6) & 0x3F)),
                (char)(0x80 | (chr & 0x3F)),
                '\0'
        };
        fputs(c, fp);
    }
    else if (vm) {
        vmAbort(vm, "invalid UCS character: \\U%08x", chr);
    }
    else {
        unreachable("!!!invalid UCS character: \\U%08x", chr);
    }
}
