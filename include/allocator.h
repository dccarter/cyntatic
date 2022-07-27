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

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CYN_ALLOCATOR_MAGIC0 (u32)0x43796e
#define CynAllocMagic(X) ((CYN_ALLOCATOR_MAGIC0 << 8) | (X))
#define CynAllocMagicHeader(X) ((X) >> 8)
#define CynAllocMagicId(X)     ((X) & 0xff)

/**
 * An allocator is a memory manager, can be used by some
 * types to allocate memory
 */
typedef struct Allocator {
    void *(*alloc)(u32);
    void *(*cAlloc)(u32, u32);
    void *(*reAlloc)(void *, u32, u32);
    void  (*dealloc)(void *, u32);
    void  (*dump)(void *);
    u32   magic;
} Allocator;

typedef void *(*Alloc)(u32);
typedef void *(*CAlloc)(u32, u32);
typedef void *(*ReAlloc)(void *, u32, u32);
typedef void  (*Dealloc)(void *, u32);
typedef void  (*dump)(void *);

typedef struct AllocatorMetadata {
    Allocator *A;
    u32        size;
    u32        magic;
    void      *mem[0];
} attr(packed) AllocatorMetadata;

/**
 * Default allocator just allocates memory from system memory
 */
extern Ptr(Allocator) DefaultAllocator;

u32 cynGetPageSize(void);

#ifndef PAGE_SIZE
#define CYN_PAGE_SIZE cynGetPageSize()
#else
#define CYN_PAGE_SIZE PAGE_SIZE
#endif

/**
 * This function is used to initialize an allocator. Every allocator
 * must be initialized with this API
 *
 * @param A
 * @param alloc
 * @param cAlloc
 * @param reAlloc
 * @param dealloc
 */
void Allocator_init(Allocator *A, Alloc alloc, CAlloc cAlloc, ReAlloc reAlloc, Dealloc dealloc);

attr(always_inline)
static void Allocator_dumpStats(Allocator *A, void *to)
{
    if (A && A->dump) A->dump(to);
}

void *Allocator_alloc(Allocator *A, u32 size);

void *Allocator_cAlloc(Allocator *A, u32 size, u32 num);

void *Allocator_reAlloc(Allocator *A, void *mem, u32 newSize);



/**
 * Deallocate memory that was originally allocated
 * by a CYN allocator
 * @param mem
 */
void Allocator_dealloc(void *mem);

void *Allocator_relocate(void *mem, Allocator *to, u32 size);

char *Allocator_strndup(Allocator *A, const char* str, u32 len);

attr(always_inline)
char *Allocator_strdup(Allocator *A, const char* str)
{
    return Allocator_strndup(A, str, strlen(str));
}

#ifdef __cplusplus
}
#endif