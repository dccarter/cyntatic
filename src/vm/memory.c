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

#define vmHEAP(vm) (Heap *)(vm)->ram.ptr

static void VM_heap_insert_heap_block(VM *vm, Heap *heap, HeapBlock *block)
{
#ifndef CYN_HA_DISABLE_COMPACT
    HeapBlock *ptr  = heap->free;
    HeapBlock *prev = NULL;
    while (ptr != NULL) {
        if (block->addr <= ptr->addr) {
            VM_dbg_trace(vm, trcHEAP,
                         fprintf(stdout, "heap: insert %u.\n", ptr->addr);
            );
            break;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    if (prev != NULL) {
        if (ptr == NULL) {
            VM_dbg_trace(vm, trcHEAP, fputs("heap: new tail.\n", stdout));
        }
        prev->next = block;
    } else {
        VM_dbg_trace(vm, trcHEAP, fputs("heap: new head.\n", stdout));
        heap->free = block;
    }
    block->next = ptr;
#else
    block->next = heap->free;
    heap->free  = block;
#endif
}

#ifndef CYN_HA_DISABLE_COMPACT
static void VM_heap_release_heap_blocks(VM *vm, Heap *heap, HeapBlock *scan, HeapBlock *to)
{
    HeapBlock *snext;
    while (scan != to) {
        VM_dbg_trace(vm, trcHEAP, printf("heap: release %u\n", scan->addr));
        snext   = scan->next;
        scan->next  = heap->fresh;
        heap->fresh = scan;
        scan->addr  = 0;
        scan->size  = 0;
        scan        = snext;
    }
}

static void VM_heap_compact(VM *vm, Heap *heap)
{
    HeapBlock *ptr = heap->free;
    HeapBlock *prev;
    HeapBlock *scan;
    while (ptr != NULL) {
        prev = ptr;
        scan = ptr->next;
        while (scan != NULL && prev->addr + prev->size == scan->addr) {
            VM_dbg_trace(vm, trcHEAP, printf("heap: merge %u\n", scan->addr));
            prev = scan;
            scan = scan->next;
        }
        if (prev != ptr) {
            u32 newSize = prev->addr - ptr->addr + prev->size;
            VM_dbg_trace(vm, trcHEAP, printf("heap: new size %u\n", newSize));

            ptr->size   = newSize;
            HeapBlock *next = prev->next;
            VM_heap_release_heap_blocks(vm, heap, ptr->next, prev->next);
            ptr->next = next;
        }
        ptr = ptr->next;
    }
}
#endif

static HeapBlock *VM_heap_alloc_heap_block(VM *vm, Heap *heap, u32 size)
{
    HeapBlock *ptr  = heap->free;
    HeapBlock *prev = NULL;
    u32 top  = heap->top;
    size  = CynAlign(size, heap->aln);

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
                VM_dbg_trace(vm, trcHEAP, printf("heap: resize top block\n"));
                ptr->size = size;
                heap->top = ptr->addr + size;
#ifndef CYN_HA_DISABLE_SPLIT
            } else if (heap->fresh != NULL) {
                u32 excess = ptr->size - size;
                if (excess >= heap->sth) {
                    ptr->size    = size;
                    HeapBlock *split = heap->fresh;
                    heap->fresh  = split->next;
                    split->addr  = ptr->addr + size;
                    VM_dbg_trace(vm, trcHEAP, printf("heap: split %u\n", split->addr));
                    split->size = excess;
                    VM_heap_insert_heap_block(vm, heap, split);
#ifndef CYN_HA_DISABLE_COMPACT
                    VM_heap_compact(vm, heap);
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

void VM_heap_init_(VM *vm, u32 blocks, u32 sth, u8 alignment)
{
    Heap *heap = vmHEAP(vm);
    heap->sth = sth;
    heap->aln = alignment;
    heap->lmt = vm->ram.hlm;

    heap->free   = NULL;
    heap->used   = NULL;
    heap->fresh  = (HeapBlock *) &heap->mem[0];
    heap->top    = vm->ram.hb;

    HeapBlock *block = heap->fresh;
    size_t i  = blocks - 1;
    while (i--) {
        block->next = block + 1;
        block++;
    }
    block->next = NULL;
}

u32 VM_alloc(VM *vm, u32 size)
{
    Heap *heap = vmHEAP(vm);
    HeapBlock *block = VM_heap_alloc_heap_block(vm, heap, size);
    if (block != NULL) {
        return block->addr;
    }
    return 0;
}

bool VM_free(VM *vm, u32 mem)
{
    if (mem == 0) return 0;

    Heap *heap = vmHEAP(vm);
    HeapBlock *block = heap->used;
    HeapBlock *prev  = NULL;
    while (block != NULL) {
        if (mem == block->addr) {
            if (prev) {
                prev->next = block->next;
            } else {
                heap->used = block->next;
            }
            VM_heap_insert_heap_block(vm, heap, block);
#ifndef TA_DISABLE_COMPACT
            VM_heap_compact(vm, heap);
#endif
            return true;
        }
        prev  = block;
        block = block->next;
    }
    return false;
}

u32 VM_cstring_dup_(VM *vm, const char *s, u32 len)
{
    u32 mem = VM_alloc(vm, len + 1);
    VM_assert(vm, mem != 0, "Out of heap memory, consider adjusting heap size");
    strncpy((char *)MEM(vm, mem), s, len);
    return mem;
}
