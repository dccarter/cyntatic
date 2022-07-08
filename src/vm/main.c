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

int main(int argc, char *argv[])
{
    Code code;
    Vector_init(&code);

    VM vm;
    vmInit(&vm, 1024);
    vmRun(&vm, &code);

    return EXIT_SUCCESS;
}