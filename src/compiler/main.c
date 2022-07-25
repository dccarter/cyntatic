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

#include <stdio.h>

int main(int argc, char *argv[])
{
    Buffer buffer;
    cynArenaAllocatorInit(CYN_PAGE_SIZE);
    cynPoolAllocatorInit();

    {
        Buffer_initWith(&buffer, ArenaAllocator);
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
    return 0;
}
