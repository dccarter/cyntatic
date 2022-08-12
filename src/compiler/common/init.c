/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-29
 */

#include "compiler/init.h"
#include "compiler/common/heap.h"
#include "compiler/common/ident.h"
#include "compiler/common/timer.h"

#include "stream.h"

void Compiler_init_common(void)
{
    ArenaAllocator_Init(CYN_DEFAULT_ARENA_BLOCK_SIZE);
    PoolAllocator_Init();

    //ArenaAllocator = DefaultAllocator;
    //PoolAllocator = DefaultAllocator;

    IdentCache_init();
    Streams_init();
    Timer_init();
}