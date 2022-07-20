/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-24
 */


#include "vm/builtins.h"
#include <unistd.h>

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

    u32 hello = vmCodeAppendString(&code, "Hello World!\n");    // Add "Hello World!" to data section of the code

    header->db = Vector_len(&code);
    header->main = header->db;

    // Program code to write "Hello World" to STDOUT_FILENO
    vmCodeAppend(&code,
             cRMEM(rRa(r1), xIMb(u32, hello), dQ),              // get the real memory address of hello
             bncWRITE(xIMa(u32, STDOUT_FILENO),                 // file descriptor to write to
                      rRa(r1),                                  // the real memory address of the string was put in r2
                      xIMa(u32, 13),                            // the size of the string is 13
                      rRa(r2)),                                 // return the number of bytes written to 32
            cHALT());

    header->size = Vector_len(&code);

    {
        VM vm = {0};
        vmInit(&vm, &code, CYN_VM_DEFAULT_MS);
        vmRun(&vm, argc, argv);
        vmDeInit(&vm);
    }

    return EXIT_SUCCESS;
}