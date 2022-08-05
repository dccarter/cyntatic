/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#include "asm/asm.h"

#include "compiler/compile.h"
#include "compiler/log.h"
#include "compiler/lexer.h"

#define ASM_check_errors(L, msg)        \
    if ((L)->errors)                     \
        abortCompiler((L), (msg));

int main(int argc, char *argv[])
{
    Source src;
    Log  L;
    Lexer  lX;
    Assembler as;
    Code code;

    Compiler_init();
    Log_init(&L);

    Source_open(&src, &L, "../src/asm/examples/args.acyn");
    ASM_check_errors(&L, NULL);

    Lexer_init(&lX, &L, &src);
    Assembler_init(&as, &lX);
    Vector_init(&code);

    Assembler_assemble(&as, &code);
    ASM_check_errors(&L, NULL);

    vmCodeDisassemble_(&code, stdout, true);

    Assembler_deinit(&as);
    Source_deinit(&src);

    return 0;
}