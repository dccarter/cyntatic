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
    Vector_push(&code, opHalt);

    VM vm = {0};
    vmInit(&vm, CYN_VM_DEFAULT_MS);

    vmRun(&vm, &code, argc, argv);

    vmDeInit(&vm);
    return EXIT_SUCCESS;
}