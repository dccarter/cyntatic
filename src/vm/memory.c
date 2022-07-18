/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-07-17
 */

#include "vm/vm.h"

typedef struct MemoryBlock {
    struct MemoryBlock *next;
    u32 size;
    u32 addr;
} attr(packed) Block;

typedef struct MemoryHeap {
    Block *free;
    Block *used;
    Block *fresh;
    u32   top;
    u32   sth;
    u32   lmt;
    u32   mbk;
    u8    aln;
    u8    mem[0];
} attr(packed) Heap;

#define vmHEAP(vm) (Heap *)&MEM((vm), (vm)->ram.hlm);

static void vmHeapInsertBlock(Heap *heap, Block *block)
{
#ifndef CYN_HA_DISABLE_COMPACT
    Block *ptr  = heap->free;
    Block *prev = NULL;
    while (ptr != NULL) {
        if (block->addr <= ptr->addr) {
            vmDbgTrace(HEAP,
                fprintf(stdout, "heap: insert %u.\n", ptr->addr);
            );
            break;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    if (prev != NULL) {
        if (ptr == NULL) {
            vmDbgTrace(HEAP, fputs("heap: new tail.\n", stdout));
        }
        prev->next = block;
    } else {
        vmDbgTrace(HEAP, fputs("heap: new head.\n", stdout));
        heap->free = block;
    }
    block->next = ptr;
#else
    block->next = heap->free;
    heap->free  = block;
#endif
}

#ifndef CYN_HA_DISABLE_COMPACT
static void vmHeapReleaseBlocks(Heap *heap, Block *scan, Block *to)
{
    Block *snext;
    while (scan != to) {
        vmDbgTrace(HEAP, printf("heap: release %u\n", scan->addr));
        snext   = scan->next;
        scan->next  = heap->fresh;
        heap->fresh = scan;
        scan->addr  = 0;
        scan->size  = 0;
        scan        = snext;
    }
}

static void vmHeapCompact(Heap *heap)
{
    Block *ptr = heap->free;
    Block *prev;
    Block *scan;
    while (ptr != NULL) {
        prev = ptr;
        scan = ptr->next;
        while (scan != NULL && prev->addr + prev->size == scan->addr) {
            vmDbgTrace(HEAP, printf("heap: merge %u\n", scan->addr));
            prev = scan;
            scan = scan->next;
        }
        if (prev != ptr) {
            u32 newSize = prev->addr - ptr->addr + prev->size;
            vmDbgTrace(HEAP, printf("heap: new size %u\n", newSize));

            ptr->size   = newSize;
            Block *next = prev->next;
            vmHeapReleaseBlocks(heap, ptr->next, prev->next);
            ptr->next = next;
        }
        ptr = ptr->next;
    }
}
#endif

static Block *vmHeapAllocBlock(Heap *heap, u32 size)
{
    Block *ptr  = heap->free;
    Block *prev = NULL;
    u32 top  = heap->top;
    size  = (size + heap->aln - 1) & -heap->aln;

    while (ptr != NULL) {
        bool isTop = (ptr->addr + ptr->size >= top) && (ptr->addr + size <= heap->lmt);
        if (isTop || ptr->size >= size) {
            if (prev != NULL) {
                prev->next = ptr->next;
            } else {
                heap->free = ptr->next;
            }
            ptr->next  = heap->used;
            heap->used = ptr;
            if (isTop) {
                vmDbgTrace(HEAP, printf("heap: resize top block\n"));
                ptr->size = size;
                heap->top = ptr->addr + size;
#ifndef CYN_HA_DISABLE_SPLIT
            } else if (heap->fresh != NULL) {
                u32 excess = ptr->size - size;
                if (excess >= heap->sth) {
                    ptr->size    = size;
                    Block *split = heap->fresh;
                    heap->fresh  = split->next;
                    split->addr  = ptr->addr + size;
                    vmDbgTrace(HEAP, printf("heap: split %u\n", split->addr));
                    split->size = excess;
                    vmHeapInsertBlock(heap, split);
#ifndef CYN_HA_DISABLE_COMPACT
                    vmHeapCompact(heap);
#endif
                }
#endif
            }
            return ptr;
        }
        prev = ptr;
        ptr  = ptr->next;
    }

    u32 newTop = top + size;
    if (heap->fresh != NULL && newTop <= heap->lmt) {
        ptr         = heap->fresh;
        heap->fresh = ptr->next;
        ptr->addr   = top;
        ptr->next   = heap->used;
        ptr->size   = size;
        heap->used  = ptr;
        heap->top   = newTop;
        return ptr;
    }
    return NULL;
}

void vmHeapInit_(VM *vm, u32 blocks, u32 sth, u8 alignment)
{
    Heap *heap = vmHEAP(vm);
    heap->sth = sth;
    heap->aln = alignment;
    heap->mbk = blocks;
    heap->lmt = vm->ram.sb;

    heap->free   = NULL;
    heap->used   = NULL;
    heap->fresh  = (Block *) &heap->mem[0];
    heap->top    = vm->ram.hlm + (blocks * sizeof(Block));

    Block *block = heap->fresh;
    size_t i  = blocks - 1;
    while (i--) {
        block->next = block + 1;
        block++;
    }
    block->next = NULL;
}

u32 vmAlloc(VM *vm, u32 size)
{
    Heap *heap = vmHEAP(vm);
    Block *block = vmHeapAllocBlock(heap, size);
    if (block != NULL) {
        return block->addr;
    }
    return 0;
}

bool vmFree(VM *vm, u32 mem)
{
    if (mem == 0) return 0;

    Heap *heap = vmHEAP(vm);
    Block *block = heap->used;
    Block *prev  = NULL;
    while (block != NULL) {
        if (mem == block->addr) {
            if (prev) {
                prev->next = block->next;
            } else {
                heap->used = block->next;
            }
            vmHeapInsertBlock(heap, block);
#ifndef TA_DISABLE_COMPACT
            vmHeapCompact(heap);
#endif
            return true;
        }
        prev  = block;
        block = block->next;
    }
    return false;
}
