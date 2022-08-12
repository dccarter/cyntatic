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

#include "compiler/common/heap.h"
#include "compiler/common/ident.h"
#include "compiler/common/lexer.h"
#include "compiler/common/log.h"
#include "compiler/common/timer.h"
#include "tree.h"

#include <stdio.h>

void Dump_u32(const void *value, char color)
{
    if (value == NULL) {
        printf("\n");
        return;
    }

    char *val = *((char **)value);
    fprintf(stdout, "%c:%s ", color, val);
}

int main(int argc, char *argv[])
{
    Log L;
    Source src;
    Token tok;
    Lexer lX;

    ArenaAllocator_Init(CYN_DEFAULT_ARENA_BLOCK_SIZE);
    PoolAllocator_Init();

    IdentCache_init();
    Streams_init();
    Timer_init();

//    u64 comp = Timer_add(true);
//    Log_init(&L);
//    Source_open(&src, &L, "../src/compiler/lexer.c");
//    if (L.errors)
//        abortCompiler0(&L, Stderr, "Parser error");
//
//    Lexer_init(&lX, &L, &src);
//
//    while (Lexer_next(&lX, &tok) != tokEoF) {
//        Token_toString0(&tok, Stdout);
//        fputc('\n', stdout);
//        if (tok.kind == tokString && tok.value.kind == vkdString)
//            Allocator_dealloc(tok.value.s);
//    }
//
//    u64 elapsed = Timer_stop(comp);
//    fprintf(stderr, "Lexing %g Kb took %lu us!\n", src.contents.len/1024.0, elapsed);
//

    RbTree(char*) rbt;
    RbTree_initWith(&rbt, RbTree_cmp_string, PoolAllocator);
    RbTree_add(&rbt, "Hello");
    RbTree_add(&rbt, "Cello");
    RbTree_add(&rbt, "Iello");
    RbTree_add(&rbt, "Bello");
    RbTree_add(&rbt, "Jello");
    RbTree_add(&rbt, "Aello");
    RbTree_add_str(&rbt, "Kello", 5);

    RbTree_dump_(&rbt.base, Dump_u32);

    char  **aello = RbTree_find(&rbt, "Aell");
    if (aello)
        printf("%s\n", *aello);
    else
        printf("not found\n");

    RbTree_deinit(&rbt);

    Allocator_dumpStats(ArenaAllocator, Stdout);
    Allocator_dumpStats(PoolAllocator, Stdout);
    Allocator_dumpStats(DefaultAllocator, Stdout);

    if (L.errors)
        abortCompiler0(&L, Stderr, "Parser error");

    return EXIT_SUCCESS;
}
