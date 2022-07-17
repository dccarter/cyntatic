/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-24
 */


#include "vm/vm.h"

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
    Vector_init(&code);
    vmCodeAppend(&code,
             cHALT(),
             cPUSH(xIM(u8, 78)),
             cMOV(rRa(r5), xIM(u8, 10)),
             cMOV(rRa(r1), xIM(u8, 24)),
             cMOV(rRa(r2), rRb(r1)),
             cADD(rRa(r2), rRb(r5)),
             cPUTI(rRa(r2)));
    printf("Code length: %u\n", Vector_len(&code));

    VM vm = {0};
    vmInit(&vm, CYN_VM_DEFAULT_MS);

    vmRun(&vm, &code, argc, argv);

    vmDeInit(&vm);
    return EXIT_SUCCESS;
}