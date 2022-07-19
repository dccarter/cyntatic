/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-24
 */


#include "vm/instr.h"

/*
 * push argv
 * push argc
 * call main
 * exit:
 *      halt
 *
 *
 */
int main(int argc, char *argv[])
{
    Code code;
    CodeHeader *header;
    Vector_init(&code);
    header = (CodeHeader *)Vector_expand(&code, sizeof(CodeHeader));
    header->db = Vector_len(&code);
    header->main = header->db;

    vmCodeAppend(&code,
             cPUSH(xIMa(u64, 78), dW),
             cPUSH(xIMa(u8, 78)),
             cPOP(rRa(r0)),
             cPUSH(xIMa(u32, 78), dW),
             cMOV(rRa(r5), xIMb(u8, 10)),
             cMOV(rRa(r1), xIMb(u8, 24)),
             cMOV(rRa(r2), rRb(r1)),
             cADD(rRa(r2), rRb(r5)),
             cBNOT(rRa(r2)),
             cPUTI(rRa(r2)),
             // move uint32 from stack
             cMOV(rRa(r1), mRb(sp), dW),
             cHALT());

    header->size = Vector_len(&code);

    printf("Code length: %u\n", Vector_len(&code));

    VM vm = {0};

    vmInit(&vm, &code, CYN_VM_DEFAULT_MS);
    vmRun(&vm, argc, argv);

    vmDeInit(&vm);

    return EXIT_SUCCESS;
}