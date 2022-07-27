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

#include <stdlib.h>
#include <unistd.h>

#define CYN_DEFAULT_ALLOCATOR_MAGIC CynAllocMagic(0x00)

// Used to assign ID's to allocators (there can only be a maximum of 255 allocators + Default)
static __thread u8 sAllocId = 1;

struct {
    u32 nAllocs;
    u32 nDeallocs;
    u32 inflight;
} sDAStats;

static void *defaultAlloc(u32 size)
{
    void *mem = malloc(size);

    if (mem) {
        sDAStats.inflight += size;
        sDAStats.nAllocs++;
    }

    return mem;
}

static void *defaultCAlloc(u32 num, u32 size)
{
    void *mem = calloc(num, size);
    if (mem) {
        sDAStats.inflight += size;
        sDAStats.nAllocs++;
    }
    return mem;
}

static void *defaultReAlloc(void *mem, attr(unused) u32 orig, u32 newSize)
{
    mem = realloc(mem, newSize);
    if (mem) {
        sDAStats.inflight += newSize;
        sDAStats.inflight -= orig;
        sDAStats.nAllocs++;
        sDAStats.nDeallocs++;
    }
    return mem;
}

static void defaultDealloc(void *mem, attr(unused) u32 size)
{
    if (mem) {
        sDAStats.inflight -= size;
        sDAStats.nDeallocs++;
        free(mem);
    }
}

static void defaultDump(void *to)
{
    __typeof__(sDAStats) stats = sDAStats;
    Buffer *B = to;

    Buffer_appendf(B, "Default Allocator: allocations %u, de-allocations: %u, inflight: %g Kb\n",
                   stats.nAllocs,
                   stats.nDeallocs,
                   stats.inflight / 1024.0);
}

static Allocator sDefaultAllocator  = {
    .alloc = defaultAlloc,
    .cAlloc = defaultCAlloc,
    .reAlloc = defaultReAlloc,
    .dealloc = defaultDealloc,
    .dump = defaultDump,
    .magic =  CYN_DEFAULT_ALLOCATOR_MAGIC
};

Ptr(Allocator) DefaultAllocator = &sDefaultAllocator;

u32 cynGetPageSize(void)
{
    static u32 sPageSize = 0;
    if (sPageSize == 0)
        sPageSize = sysconf(_SC_PAGESIZE);

    return sPageSize;
}

void Allocator_init(Allocator *A, Alloc alloc, CAlloc cAlloc, ReAlloc reAlloc, Dealloc dealloc)
{
    cynAssert(A != NULL, "Initializing an undefined allocator is undefined behaviour");
    cynAssert(alloc && cAlloc && reAlloc && dealloc, "Allocator API's cannot be NULL");

    A->alloc = alloc;
    A->reAlloc = reAlloc;
    A->cAlloc = cAlloc;
    A->dealloc = dealloc;

    A->magic = CynAllocMagic(sAllocId++);
}

void *Allocator_alloc(Allocator *A, u32 size)
{
    AllocatorMetadata *meta;
    if (size == 0) return NULL;
    if (A == NULL) A = DefaultAllocator;

    meta = (AllocatorMetadata *)A->alloc(size + sizeof(AllocatorMetadata));
    if (meta == NULL) return NULL;

    meta->A = A;
    meta->magic = A->magic;
    meta->size = size;

    return &meta->mem[0];
}

void *Allocator_cAlloc(Allocator *A, u32 size, u32 num)
{
    AllocatorMetadata *meta;
    if (size == 0 || num == 0) return NULL;
    if (A == NULL) A = DefaultAllocator;

    meta = (AllocatorMetadata *)A->cAlloc(1, ((size * num) + sizeof(AllocatorMetadata)));
    if (meta == NULL) return NULL;

    meta->A = A;
    meta->magic = A->magic;
    meta->size = size;

    return &meta->mem[0];
}

void *Allocator_reAlloc(Allocator *A, void *mem, u32 newSize)
{
    AllocatorMetadata *meta;
    if (mem == NULL) return Allocator_alloc(A, newSize);

    cynAssert(newSize > 0, "Reallocating with size 0 is undefined behaviour");

    meta = (AllocatorMetadata *) ((u8 *)mem - sizeof(AllocatorMetadata));
    cynAssert(meta->A == A && meta->magic == A->magic,
              "Reallocating memory allocated by a different allocator in undefined behaviour, %p", mem);

    meta = (AllocatorMetadata *)A->reAlloc(
            meta, (meta->size + sizeof(AllocatorMetadata)), (newSize + sizeof(AllocatorMetadata)));
    meta->size = newSize;

    return &meta->mem[0];
}

void Allocator_dealloc(void *mem)
{
    AllocatorMetadata *meta;
    Allocator *A;
    u32 header;
    if (mem == NULL) return;

    meta = (AllocatorMetadata *) ((u8 *)mem - sizeof(AllocatorMetadata));
    A = meta->A;
    header = CynAllocMagicHeader(meta->magic);

    cynAssert(A != NULL && header == CYN_ALLOCATOR_MAGIC0 && A->magic == meta->magic,
              "Undefined behaviour, was memory allocated by a CYN allocator %p", mem);

    A->dealloc(meta, meta->size + sizeof(AllocatorMetadata));
}

char *Allocator_strndup(Allocator *A, const char* str, u32 len)
{
    char *dst = Allocator_alloc(A, len+1);

    if (dst) {
        strncpy(dst, str, len);
        dst[len] = '\0';
    }

    return dst;
}



