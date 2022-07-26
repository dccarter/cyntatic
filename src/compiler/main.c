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

#include <stdio.h>

int main(int argc, char *argv[])
{
    Buffer buffer;
    ArenaAllocator_Init(CYN_PAGE_SIZE);
    PoolAllocator_Init();
    IdentCache_init();

    {
        Buffer_initWith(&buffer, PoolAllocator);
        Allocator_dumpStats(DefaultAllocator, &buffer);
        fputs(Buffer_cstr(&buffer), stdout);
        Vector_deinit(&buffer);
    }
    {
        Buffer_initWith(&buffer, ArenaAllocator);
        Allocator_dumpStats(DefaultAllocator, &buffer);
        fputs(Buffer_cstr(&buffer), stdout);
        Vector_deinit(&buffer);
    }
}
