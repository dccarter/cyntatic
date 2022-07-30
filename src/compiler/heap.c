/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "compiler/heap.h"

#include "buffer.h"
#include "vector.h"
#include "stream.h"

static void *ArenaAllocator_alloc(u32 size);
static void *ArenaAllocator_cAlloc(u32 num, u32 size);
static void *ArenaAllocator_reAlloc(void *mem, u32 orig, u32 size);
static void  ArenaAllocator_dealloc(void *mem, u32 size);
static void  ArenaAllocator_Dump(void *);

static void *PoolAllocator_alloc(u32 size);
static void *PoolAllocator_cAlloc(u32 num, u32 size);
static void *PoolAllocator_reAlloc(void *mem, u32 orig, u32 size);
static void  PoolAllocator_dealloc(void *mem, u32 size);
static void  PoolAllocator_dump(void *);

static struct ArenaAllocatorRegion {
    u8   *data;
    u32   size;
    u32   allocs;
    u32   current;
    struct ArenaAllocatorRegion *next;
} sArena;

static struct {
    u32 regions;
    u64 size;
    u64 used;
} sArenaStats;

#define NUMBER_OF_POOLS 8
#define POOL_BLOCK_SIZE 32768

typedef struct PoolAllocatorData {
    struct PoolAllocatorData *next;
    u8 buf[0];
} PoolAllocatorData;

typedef struct PoolAllocatorBlock {
    u32         size;
    u32         segments;
    u32         allocs;
    u32         deallocs;
    Vector(u8*) free;
    PoolAllocatorData *data;
} PoolAllocatorBlock;

static struct PoolAllocatorPools {
    PoolAllocatorBlock blocks[NUMBER_OF_POOLS];
} sPoolAllocatorBlocks;

Allocator sArenaAllocator;
Ptr(Allocator) ArenaAllocator = NULL;

Allocator sPoolAllocator;
Ptr(Allocator) PoolAllocator = NULL;

attr(always_inline)
static u32 np2(u32 v)
{
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

attr(always_inline)
static u32 n21(u32 value)
{
    if (value == 0) return 0;

#if defined(__GNUC__) || defined(__GNUG__)
    return 31 - __builtin_clz(value);
#else
    u32 i = 0;
    while (value >>= 1) ++i;
    return i;
#endif
}

static void ArenaAllocator_Init_(struct ArenaAllocatorRegion *arena, u32 size)
{
    arena->size = size;
    arena->current = 0;
    arena->next = NULL;
    arena->data = Allocator_alloc(DefaultAllocator, arena->size);
    sArenaStats.regions++;
    sArenaStats.size += arena->size;
}

static void PoolAllocatorBlock_Init(PoolAllocatorBlock *block, u32 size)
{
    size += sizeof(AllocatorMetadata);
    Vector_init0(&block->free, (POOL_BLOCK_SIZE/size)+1);
    block->size = size;
    block->data = Allocator_alloc(DefaultAllocator, POOL_BLOCK_SIZE - sizeof(AllocatorMetadata));
    cynAssert(block->data, "!!!Out of Memory!!!");

    for (u32  i = 0; i < POOL_BLOCK_SIZE; i += size) {
        // populate the free buffer list
        block->segments++;
        Vector_push(&block->free, &block->data->buf[i]);
    }
}

void ArenaAllocator_Init(u32 size)
{
    cynAssert(ArenaAllocator == NULL, "Arena allocator already initialized");

    ArenaAllocator = &sArenaAllocator;
    Allocator_init(&sArenaAllocator,
                   ArenaAllocator_alloc,
                   ArenaAllocator_cAlloc,
                   ArenaAllocator_reAlloc,
                   ArenaAllocator_dealloc);
    sArenaAllocator.dump = ArenaAllocator_Dump;

    ArenaAllocator_Init_(&sArena, size);
}

void PoolAllocator_Init(void)
{
    cynAssert(PoolAllocator == NULL, "Pool allocator already initialized");

    PoolAllocator = &sPoolAllocator;

    Allocator_init(&sPoolAllocator,
                   PoolAllocator_alloc,
                   PoolAllocator_cAlloc,
                   PoolAllocator_reAlloc,
                   PoolAllocator_dealloc);
    sPoolAllocator.dump = PoolAllocator_dump;

    for (int i = 0; i < NUMBER_OF_POOLS; i++) {
        PoolAllocatorBlock_Init(&sPoolAllocatorBlocks.blocks[i],
                                (1 << (i + 4))); // i + 4 because minimum size is 16
    }
}

void *ArenaAllocator_alloc(u32 size)
{
    struct ArenaAllocatorRegion *arena = &sArena, *last = &sArena;
    while (arena) {
        if (arena->size - arena->current > size) {
            u32 i = arena->current;
            arena->current += size;
            arena->allocs++;

            sArenaStats.used += size;

            return &arena->data[i];
        }
        last = arena;
        arena = arena->next;
    }

    arena = Allocator_cAlloc(DefaultAllocator, 1, sizeof(*arena));
    last->next = arena;
    ArenaAllocator_Init_(arena, MAX(sArena.size, size));
    arena->current += size;
    arena->allocs++;

    sArenaStats.used += size;

    return arena->data;
}

void *ArenaAllocator_cAlloc(u32 num, u32 size)
{
    void *mem = ArenaAllocator_alloc(num * size);
    if (mem)
        bzero(mem, num * size);
    return mem;
}

void *ArenaAllocator_reAlloc(void *mem, u32 orig, u32 size)
{
    void *ret;
    // WARNING!!! Unwise to re-alloc in an arena
    if (size == 0) return NULL;

    ret = ArenaAllocator_alloc(size);
    if (ret == NULL) return NULL;

    memmove(ret, mem, MIN(orig, size));

    return ret;
}


void  ArenaAllocator_dealloc(attr(unused) void *mem, attr(unused) u32 size)
{
    // noop
}

void  ArenaAllocator_Dump(void *to)
{
    Stream *os = to;
    Stream_printf(os,
                  "Arena Allocator: regions: %u, size: %g Kb, used: %g Kb\n",
                  sArenaStats.regions, (double)sArenaStats.size/1024.0, (double)sArenaStats.used/1024.0);
    int i = 0;
    for (struct ArenaAllocatorRegion *region = &sArena; region != NULL; region = region->next) {
        Stream_printf(os,
                      "\tregion-%d: allocs: %4u, usage: %.2f %\n",
                      i, region->allocs, (((double)region->current/region->size) * 100));
        i++;
    }
}

void *PoolAllocator_alloc(u32 size)
{
    u32 i;
    PoolAllocatorBlock *block;

    if (size == 0) return NULL;
    if (size >= 4096) return NULL;

    size = np2(MAX(16, size));
    i = n21(size) - 4;

    block = &sPoolAllocatorBlocks.blocks[i];
    if (Vector_empty(&block->free)) {
        // Can we increase number of blocks?
        return NULL;
    }
    block->allocs++;
    // remove a block from the vector
    return Vector_pop(&block->free);
}

static void *PoolAllocator_cAlloc(u32 num, u32 size)
{
    void *mem;
    size = num * size;

    mem = PoolAllocator_alloc(size);
    if (mem == NULL) return NULL;

    bzero(mem, size);
    return mem;
}

void *PoolAllocator_reAlloc(void *mem, u32 orig, u32 size)
{
    void *ret;
    u32 pog = np2(MAX(16, orig));
    u32 psz = np2(MAX(16, size));

    // if the new size can be allocated in current memory, return current
    if (psz == pog) return mem;

    ret = PoolAllocator_alloc(size);
    if (!ret) return NULL;
    memmove(ret, mem, MIN(size, orig));

    // memory we are re-allocating from
    PoolAllocator_dealloc(mem, orig);

    return ret;
}

void  PoolAllocator_dealloc(void *mem, u32 size)
{
    u32 i;
    PoolAllocatorBlock *block;

    size = np2(MAX(16, size));
    i = n21(size) - 4;

    block = &sPoolAllocatorBlocks.blocks[i];
    block->deallocs++;
    Vector_push(&block->free, (u8 *)mem);
}

void  PoolAllocator_dump(void *to)
{
    Stream *os = to;
    Stream_puts(os, "Poll allocator:\n");
    for (int i = 0; i < NUMBER_OF_POOLS; i++) {
        double usage;
        PoolAllocatorBlock *block = &sPoolAllocatorBlocks.blocks[i];
        usage = (double)(block->segments - Vector_len(&block->free))/block->segments * 100;
        Stream_printf(os, "\tblock-%-6u: allocs: %4u, deallocs: %4u, usage: %.2f %\n",
                            block->size, block->allocs, block->deallocs, usage);
    }
}
