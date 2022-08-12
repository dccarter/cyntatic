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

const char* vmModeNamesTbl[] = {".b", ".s", ".w", ".q"};

const char *vmRegisterNameTbl[] = {
        "r0", "r1", "r2", "r3", "r4", "r5",
        "sp", "ip", "bp", "flg"
};

const char* vmInstructionNamesTbl[] = {
#define XX(OP, N, ...) #N,
        VM_OP_CODES(XX)
#undef XX
};

typedef Pair(const char*, OpCodeInfo) VmOpCodeNamePair;
typedef RbTree(VmOpCodeNamePair)   VmOpCodeNamePairList;

static inline
int VmOpCodeNamePairList_cmp(const void *lhs, u32 len, const void *rhs)
{
    VmOpCodeNamePair *aa = (VmOpCodeNamePair *) lhs, *bb = (VmOpCodeNamePair *) rhs;
    return RbTree_case_cmp_string(&aa->f, len, &bb->f);
}

static VmOpCodeNamePairList sVmOpCodeNamePairList;
static bool sVmOpCodeNamePairListInitialized = false;

static void VM_initCodeNamePairs(void)
{

    if (sVmOpCodeNamePairListInitialized == false) {
        RbTree_init(&sVmOpCodeNamePairList, VmOpCodeNamePairList_cmp);
#define XX(OP, N, SZ) RbTree_add(&sVmOpCodeNamePairList, make(VmOpCodeNamePair, #N, make(OpCodeInfo, op##OP, SZ)));
        VM_OP_CODES(XX)
#undef XX
        sVmOpCodeNamePairListInitialized = true;
    }
}

OpCodeInfo VM_get_opcode_for_instr_(const char *instr, u32 len)
{
    RbTreeNode *it;
    VmOpCodeNamePair pair = {.f = instr};

    VM_initCodeNamePairs();

    it = RbTree_find_(&sVmOpCodeNamePairList.base, &pair, len);
    if (it == NULL)
        return make(OpCodeInfo, opcCOUNT, 0);

    return RbTree_ref(&sVmOpCodeNamePairList, it)->s;
}

Register Vm_get_register_from_str_(const char *str, u32 len)
{
    if (len < 2 || len > 3) return regCOUNT;
    switch(str[0]) {
        case 'b':
            return (str[1] == 'p' && len == 2)? bp : regCOUNT;
        case 'i':
            return (str[1] == 'p' && len == 2)? ip : regCOUNT;
        case 's':
            return (str[1] == 'p' && len == 2)? sp : regCOUNT;
        case 'f':
            return (str[1] == 'l' && len == 3 && str[2] == 'g') ? flg : regCOUNT;
        case 'r':
            if (str[1] < '0' || str[1] > '5' || len != 2) return regCOUNT;
            return r0 + str[1] - '0';
        default:
            return regCOUNT;
    }
}

void VM_code_print_instruction_(const Instruction* instr, FILE *fp)
{
    if (instr->osz == 0) {
        fputs("...null...\n", fp);
        return;
    }

    if (instr->opc >= opcCOUNT) {
        fprintf(fp, "not-sup.%u\n", instr->opc);
        return;
    }

    fputs(vmInstructionNamesTbl[instr->opc], fp);
    fputs(vmModeNamesTbl[instr->imd], fp);

    if (instr->osz == 1) {
        return;
    }

    fputs(" ", fp);

    switch (instr->osz) {
        case 2:
            if (instr->iam)
                fputc('[', fp);
            if (instr->rmd == amReg)
                fputs(vmRegisterNameTbl[instr->ra], fp);
            else
                fprintf(fp, "%" PRIi64, instr->ii);
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
            if (instr->rmd == amReg) {
                fputs(vmRegisterNameTbl[instr->rb], fp);
                if (instr->iea)
                    fprintf(fp, ", %" PRIi64, instr->ii);
            }
            else
                fprintf(fp, "%" PRIi64, instr->ii);
            if (instr->ibm)
                fputc(']', fp);
            break;
        default:
            printf("Unsupported size of %u.%u\n", instr->opc, instr->osz);
            unreachable();
    }
}

void VM_put_utf8_chr_(VM *vm, u32 chr, FILE *fp)
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
        VM_abort(vm, "invalid UCS character: \\U%08x", chr);
    }
    else {
        unreachable("!!!invalid UCS character: \\U%08x", chr);
    }
}
