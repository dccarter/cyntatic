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

#define CYN_DEFAULT_ARENA_BLOCK_SIZE (64 * CYN_PAGE_SIZE)

extern Ptr(Allocator) ArenaAllocator;
extern Ptr(Allocator) PoolAllocator;

void ArenaAllocator_Init(u32 size);
void PoolAllocator_Init(void);
