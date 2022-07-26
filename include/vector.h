/**
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

#include <common.h>
#include <allocator.h>

#define Vector(T) struct {Ptr(T) data; Allocator *Alloc; int len, capacity; }

#define Vector_unpack_(self) \
    (self)->Alloc, (char **)&(self)->data, &(self)->len, &(self)->capacity, sizeof((self)->data[0])

#define Vector_init_(self) memset((self), 0, sizeof(*(self)))

#define Vector_deinit(self) (Vector_dealloc_(Vector_unpack_(self)), Vector_init_(self))

#define Vector_init(self) (Vector_init_(self), (self)->Alloc = DefaultAllocator)

#define Vector_release(self) ({void *LineVAR(_rel) = (self)->data; Vector_init_(self); LineVAR(_rel); })

#define Vector_initWith(self, A) (Vector_init_(self), (self)->Alloc = (A))

#define Vector_init0(self, n)                                               \
    ( Vector_init(self), Vector_reserve_(Vector_unpack_(self), (n)) )        \

#define Vector_init0With(self, A, n)                                                   \
    ( Vector_initWith((self), (A)), Vector_reserve_(Vector_unpack_(self), (n)))        \

#define Vector_push(self, v)                     \
    ( Vector_expand_(Vector_unpack_(self))? -1:  \
      ((self)->data[(self)->len++] = (v), 0),    \
      0                                          \
    )

#define Vector_pop(self) (self)->data[--(self)->len]

#define Vector_splice(self, start, count)                       \
    ( Vector_splice_(Vector_unpack_(self), (start), (count)),   \
      (self)->len -= (count)                                    \
    )

#define Vector_swapSplice(self, start, count)                       \
    ( Vector_swapSplice_(Vector_unpack_(self), (start), (count)),   \
      (self)->len -= (count)                                        \
    )

#define Vector_insert(self, i, val)                     \
    ( Vector_insert_(Vector_unpack_(self), (i))? -1 :   \
      ((self)->data[(i)] = (val), 0),                   \
      (self)->len++,                                    \
      0                                                 \
    )

#define Vector_sort(self, fn) \
    qsort((self)->data, (self)->len, sizeof(*(self)->data), fn)

#define Vector_swap(self, i1, i2) \
    Vector_swap_(Vector_unpack_(self), (i1), (i2))

#define Vector_truncate(self, l) \
    ( (self)->len = ((l) < (self)->len? (l) : (self)->len ) )

#define Vector_clear(self) \
    ( (self)->len = 0 )

#define Vector_front(self) \
    ( (self)->data[0] )

#define Vector_back(self)   \
    ( (self)->data[(self)->len-1])

#define Vector_at(self, idx)  \
    ({ __typeof__(idx) __idx = (idx); (((__idx) < (self)->len)? &(self)->data[(__idx)]: NULL); })

#define Vector_reserve(self, n) \
    Vector_reserve_(Vector_unpack_(self), n)

#define Vector_reserve0(self, n) \
    Vector_reserve_po2_(Vector_unpack_(self), n)

#define Vector_compact(self) \
    Vector_compact_(Vector_unpack_(self))

#define Vector_begin(self) \
    (self)->data

#define Vector_end(self) \
    ((self)->data? ((self)->data + (self)->len) : NULL)

#define Vector_len(self) (self)->len
#define Vector_capacity(self) (self)->capacity
#define Vector_empty(self) (Vector_len(self) == 0)

#define Vector_pushArr(self, arr, count) \
    do {                                 \
        int LineVAR(i), LineVAR(n) = (count); if (Vector_reserve_po2_(Vector_unpack_(self), ((self)->len + LineVAR(n))) !=0 ) { break; } for (LineVAR(i) = 0; LineVAR(i) < LineVAR(n); LineVAR(i)++) { (self)->data[(self)->len++] = (arr)[LineVAR(i)]; }                                                                           \
    } while (0)

#define Vector_extend(self, v) \
    Vector_pushArr((self), (v)->data, (v)->len)

#define Vector_expand(self, N) \
    ({ u32 LineVAR(ss) = (self)->len; Vector_reserve((self), ((self)->len + (N))); (self)->len += (N); &(self)->data[LineVAR(ss)]; })


#define Vector_find(self, val) \
    ({ int LineVAR(i); do { for (LineVAR(i) = 0; LineVAR(i) < (self)->len; LineVAR(i)++) { if ((self)->data[LineVAR(i)] == (val)) { break; } } if (LineVAR(i) == (self)->len) { LineVAR(i) = -1; } } while (0); LineVAR(i); })

#define Vector_remove(self, val)                    \
    do { int idx__ = Vector_find((self), (val));    \
        if (idx__ != -1)                            \
            Vector_splice(self, idx__, 1);          \
    } while (0)


#define Vector_reverse(self)                                    \
    do {                                                        \
        int i__ = (self)->len / 2;                              \
        while (i__--) {                                         \
            Vector_swap((self), i__, (self)->len - (i__ + 1));  \
        }                                                       \
    } while (0)

#define Vector_foreach(self, var) \
    __typeof__((self)->data[0]) var; int LineVAR(i); if ((self)->len > 0) for (LineVAR(i) = 0; LineVAR(i) < (self)->len && ((var = (self)->data[LineVAR(i)]), 1); ++LineVAR(i))

#define Vector_foreach_rev(self, var) \
    __typeof__((self)->data[0]) var; int LineVAR(i); if ((self)->len > 0) for (LineVAR(i) = ((self)->len - 1); LineVAR(i) >= 0 && ((var = (self)->data[LineVAR(i)]), 1); --LineVAR(i))

#define Vector_foreach_ptr(self, var)  \
    __typeof__((self)->data[0])* var; int LineVAR(i); if ((self)->len > 0) for (LineVAR(i) = 0; LineVAR(i) < (self)->len && ((var = &(self)->data[LineVAR(i)]), 1); ++LineVAR(i))

#define Vector_foreach_ptr_rev(self, var) \
    __typeof__((self)->data[0])* var; int LineVAR(i); if ((self)->len > 0) for (LineVAR(i) = ((self)->len - 1); LineVAR(i) >= 0 && ((var = &(self)->data[LineVAR(i)]), 1); -- LineVAR(i))

int  Vector_expand_(Allocator *A, char **data, const int *len, int *cap, int entrySize);
int  Vector_reserve_(Allocator *A, char **data, const int *len, int *cap, int entrySize, int n);
int  Vector_reserve_po2_(Allocator *A, char **data, int *len, int *cap, int entrySize, int n);
int  Vector_compact_(Allocator *A, char **data, const int *len, int *cap, int entrySize);
int  Vector_insert_(Allocator *A, char **data, int *len, int *cap, int entrySize, int idx);
void Vector_splice_(Allocator *A, char **data, const int *len, const int *cap, int entrySize, int start, int count);
void Vector_swapSplice_(Allocator *A, char **data, const int *len, const int *cap, int entrySize, int start, int count);
void Vector_swap_(Allocator *A, char **data, const int *len, const int *cap, int entrySize, int idx1, int idx2);
void Vector_dealloc_(Allocator *A, char **data, const int *len, int *cap, int entrySize);

#ifdef __cplusplus
}
#endif
