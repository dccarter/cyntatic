/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-29
 */

#include "compiler/compile.h"
#include "compiler/heap.h"
#include "compiler/ident.h"
#include "compiler/timer.h"

#include "stream.h"

void Compiler_init(void)
{
    ArenaAllocator_Init(CYN_DEFAULT_ARENA_BLOCK_SIZE);
    PoolAllocator_Init();

    IdentCache_init();
    Streams_init();
    Timer_init();
}