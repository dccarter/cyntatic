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

static void *arenaAllocatorAlloc(u32 size);
static void *arenaAllocatorCAlloc(u32 num, u32 size);
static void *arenaAllocatorReAlloc(void *mem, u32 orig, u32 size);
static void  arenaAllocatorDealloc(void *mem, u32 size);
static void  arenaAllocatorDump(void *);

static void *poolAllocatorAlloc(u32 size);
static void *poolAllocatorCAlloc(u32 num, u32 size);
static void *poolAllocatorReAlloc(void *mem, u32 orig, u32 size);
static void  poolAllocatorDealloc(void *mem, u32 size);

static struct ArenaAllocatorRegion {
    u8   *data;
    u32   size;
    u32   current;
    struct ArenaAllocatorRegion *next;
} sArena;

#define NUMBER_OF_POOLS 8
#define POOL_BLOCK_SIZE 32768

typedef struct PoolAllocatorData {
    struct PoolAllocatorData *next;
    u8 buf[0];
} PoolAllocatorData;

typedef struct PoolAllocatorBlock {
    u32         size;
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

static void cynArenaAllocatorInit_(struct ArenaAllocatorRegion *arena, u32 size)
{
    arena->size = CynAlign(size, CYN_PAGE_SIZE);
    arena->current = 0;
    arena->next = NULL;
    arena->data = cynAlloc(DefaultAllocator, size);
}

static void cynPoolAllocatorBlockInit(PoolAllocatorBlock *block, u32 size)
{
    size += sizeof(AllocatorMetadata);
    Vector_init0(&block->free, (POOL_BLOCK_SIZE/size)+1);
    block->size = size;
    block->data = cynAlloc(DefaultAllocator, POOL_BLOCK_SIZE - sizeof(AllocatorMetadata));
    cynAssert(block->data, "!!!Out of Memory!!!");

    for (u32  i = 0; i < POOL_BLOCK_SIZE; i += size) {
        // populate the free buffer list
        Vector_push(&block->free, &block->data->buf[i]);
    }
}

void cynArenaAllocatorInit(u32 size)
{
    cynAssert(ArenaAllocator == NULL, "Arena allocator already initialized");

    ArenaAllocator = &sArenaAllocator;
    Allocator_init(&sArenaAllocator,
                   arenaAllocatorAlloc,
                   arenaAllocatorCAlloc,
                   arenaAllocatorReAlloc,
                   arenaAllocatorDealloc);

    cynArenaAllocatorInit_(&sArena, size);
}

void cynPoolAllocatorInit(void)
{
    cynAssert(PoolAllocator == NULL, "Pool allocator already initialized");

    PoolAllocator = &sPoolAllocator;

    Allocator_init(&sPoolAllocator,
                   poolAllocatorAlloc,
                   poolAllocatorCAlloc,
                   poolAllocatorReAlloc,
                   poolAllocatorDealloc);
    for (int i = 0; i < NUMBER_OF_POOLS; i++) {
        cynPoolAllocatorBlockInit(&sPoolAllocatorBlocks.blocks[i],
                                  (1 << (i + 4))); // i + 4 because minimum size is 16
    }
}

void *arenaAllocatorAlloc(u32 size)
{
    struct ArenaAllocatorRegion *arena = &sArena, *last = &sArena;
    while (arena) {
        if (arena->size - arena->current > size) {
            u32 i = arena->current;
            arena->current += size;
            return &arena->data[i];
        }
        arena = arena->next;
        last = arena;
    }

    arena = cynCAlloc(DefaultAllocator, 1, sizeof(*arena));
    last->next = arena;
    cynArenaAllocatorInit_(arena, size);
    arena->current += size;

    return arena->data;
}

void *arenaAllocatorCAlloc(u32 num, u32 size)
{
    void *mem = arenaAllocatorAlloc(num * size);
    if (mem)
        bzero(mem, num * size);
    return mem;
}

void *arenaAllocatorReAlloc(void *mem, u32 orig, u32 size)
{
    void *ret;
    // WARNING!!! Unwise to re-alloc in an arena
    if (size == 0) return NULL;

    ret = arenaAllocatorAlloc(size);
    if (ret == NULL) return NULL;

    memmove(ret, mem, MIN(orig, size));

    return ret;
}


void  arenaAllocatorDealloc(attr(unused) void *mem, attr(unused) u32 size)
{
    // noop
}

void  arenaAllocatorDump(void *to)
{
    Buffer *B = to;
}

void *poolAllocatorAlloc(u32 size)
{
    u32 i;
    PoolAllocatorBlock *block;

    if (size == 0) return NULL;
    if (size >= 4096) return cynAlloc(DefaultAllocator, size);

    size = np2(MAX(16, size));
    i = n21(size) - 4;

    block = &sPoolAllocatorBlocks.blocks[i];
    if (Vector_empty(&block->free)) {
        // Can we increase number of blocks?
        return NULL;
    }

    // remove a block from the vector
    return Vector_pop(&block->free);
}

static void *poolAllocatorCAlloc(u32 num, u32 size)
{
    void *mem;
    size = num * size;

    mem = poolAllocatorAlloc(size);
    if (mem == NULL) return NULL;

    bzero(mem, size);
    return mem;
}

void *poolAllocatorReAlloc(void *mem, u32 orig, u32 size)
{
    void *ret;
    u32 pog = np2(MAX(16, orig));
    u32 psz = np2(MAX(16, size));

    // if the new size can be allocated in current memory, return current
    if (psz == pog) return mem;

    ret = poolAllocatorAlloc(size);
    if (!ret) return NULL;
    memmove(ret, mem, MIN(size, orig));

    // memory we are re-allocating from
    poolAllocatorDealloc(mem, orig);

    return ret;
}

void  poolAllocatorDealloc(void *mem, u32 size)
{
    u32 i;
    PoolAllocatorBlock *block;

    size = np2(MAX(16, size));
    i = n21(size) + 4;

    block = &sPoolAllocatorBlocks.blocks[i];
    Vector_push(&block->free, (u8 *)mem);
}
