/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#pragma once

#include <allocator.h>
#include <view.h>

extern Ptr(Allocator) ArenaAllocator;
extern Ptr(Allocator) PoolAllocator;

void cynArenaAllocatorInit(u32 size);
void cynPoolAllocatorInit(void);
