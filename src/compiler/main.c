/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "allocator.h"
#include "buffer.h"

#include "compiler/heap.h"
#include "compiler/ident.h"
#include "compiler/lexer.h"
#include "compiler/log.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    Log L;
    Source src;
    Lexer lX;
    Token tok;

    ArenaAllocator_Init(CYN_PAGE_SIZE);
    PoolAllocator_Init();
    IdentCache_init();

    Streams_init();

    Log_init(&L);
    Source_load(&src, "code", R"(
int main(int argc, char *argv[])
{
    return EXIT_SUCCESS "
}
)");

    Lexer_init(&lX, &L, &src);
    while (Lexer_next(&lX, &tok) != tokEoF) {
        Token_toString0(&tok, Stdout);
        fputc('\n', stdout);
    }

    if (L.errors)
        abortCompiler0(&L, Stderr, "Parser error");

    return EXIT_SUCCESS;
}
