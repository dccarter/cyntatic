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

#define Vector(T) struct {Ptr(T) data; int len, capacity; }

#define Vector_unpack_(this) \
    (char **)&(this)->data, &(this)->len, &(this)->capacity, sizeof((this)->data[0])

#define Vector_init_(this) memset((this), 0, sizeof(*(this)))

#define Vector_deinit(this) (free((this)->data), Vector_init_(this))

#define Vector_init(this) Vector_init_(this)

#define Vector_init0(this, n)                                               \
    ( Vector_init(this), Vector_reserve_(Vector_unpack_(this), (n)))        \

#define Vector_push(this, v)                     \
    ( Vector_expand_(Vector_unpack_(this))? -1:  \
      ((this)->data[(this)->len++] = (v), 0),    \
      0                                          \
    )

#define Vector_pop(this) (this)->data[--(this)->len]

#define Vector_splice(this, start, count)                       \
    ( Vector_splice_(Vector_unpack_(this), (start), (count)),   \
      (this)->len -= (count)                                    \
    )

#define Vector_swapSplice(this, start, count)                       \
    ( Vector_swapSplice_(Vector_unpack_(this), (start), (count)),   \
      (this)->len -= (count)                                        \
    )

#define Vector_insert(this, i, val)                     \
    ( Vector_insert_(Vector_unpack_(this), (i))? -1 :   \
      ((this)->data[(i)] = (val), 0),                   \
      (this)->len++,                                    \
      0                                                 \
    )

#define Vector_sort(this, fn) \
    qsort((this)->data, (this)->len, sizeof(*(this)->data), fn)

#define Vector_swap(this, i1, i2) \
    Vector_swap_(Vector_unpack_(this), (i1), (i2))

#define Vector_truncate(this, l) \
    ( (this)->len = ((l) < (this)->len? (l) : (this)->len ) )

#define Vector_clear(this) \
    ( (this)->len = 0 )

#define Vector_front(this) \
    ( (this)->data[0] )

#define Vector_back(this)   \
    ( (this)->data[(this)->len-1])

#define Vector_at(this, idx)  \
    ({ __typeof__(idx) __idx = (idx); (((__idx) < (this)->len)? &(this)->data[(__idx)]: NULL); })

#define Vector_reserve(this, n) \
    Vector_reserve_(Vector_unpack_(this), n)

#define Vector_compact(this) \
    Vector_compact_(Vector_unpack_(this))

#define Vector_begin(this) \
    (this)->data

#define Vector_end(this) \
    ((this)->data? ((this)->data + (this)->len) : NULL)

#define Vector_len(this) (this)->len
#define Vector_empty(this) (Vector_len(this) == 0)

#define Vector_pushArr(this, arr, count) \
    do {                                 \
        int LineVAR(i), LineVAR(n) = (count); if (Vector_reserve_po2_(Vector_unpack_(this), ((this)->len + LineVAR(n))) !=0 ) { break; } for (LineVAR(i) = 0; LineVAR(i) < LineVAR(n); LineVAR(i)++) { (this)->data[(this)->len++] = (arr)[LineVAR(i)]; }                                                                           \
    } while (0)

#define Vector_extend(this, v) \
    Vector_pushArr((this), (v)->data, (v)->len)

#define Vector_find(this, val) \
    ({ int LineVAR(i); do { for (LineVAR(i) = 0; LineVAR(i) < (this)->len; LineVAR(i)++) { if ((this)->data[LineVAR(i)] == (val)) { break; } } if (LineVAR(i) == (this)->len) { LineVAR(i) = -1; } } while (0); LineVAR(i); })

#define Vector_remove(this, val)                    \
    do { int idx__ = Vector_find((this), (val));    \
        if (idx__ != -1)                            \
            Vector_splice(this, idx__, 1);          \
    } while (0)


#define Vector_reverse(this)                                    \
    do {                                                        \
        int i__ = (this)->len / 2;                              \
        while (i__--) {                                         \
            Vector_swap((this), i__, (this)->len - (i__ + 1));  \
        }                                                       \
    } while (0)

#define Vector_foreach(this, var) \
    __typeof__((this)->data[0]) var; int LineVAR(i); if ((this)->len > 0) for (LineVAR(i) = 0; LineVAR(i) < (this)->len && ((var = (this)->data[LineVAR(i)]), 1); ++LineVAR(i))

#define Vector_foreach_rev(this, var) \
    __typeof__((this)->data[0]) var; int LineVAR(i); if ((this)->len > 0) for (LineVAR(i) = ((this)->len - 1); LineVAR(i) >= 0 && ((var = (this)->data[LineVAR(i)]), 1); --LineVAR(i))

#define Vector_foreach_ptr(this, var)  \
    __typeof__((this)->data[0])* var; int LineVAR(i); if ((this)->len > 0) for (LineVAR(i) = 0; LineVAR(i) < (this)->len && ((var = &(this)->data[LineVAR(i)]), 1); ++LineVAR(i))

#define Vector_foreach_ptr_rev(this, var) \
    __typeof__((this)->data[0])* var; int LineVAR(i); if ((this)->len > 0) for (LineVAR(i) = ((this)->len - 1); LineVAR(i) >= 0 && ((var = &(this)->data[LineVAR(i)]), 1); -- LineVAR(i))

int Vector_expand_(char **data, const int *len, int *cap, int entrySize);
int Vector_reserve_(char **data, const int *len, int *cap, int entrySize, int n);
int Vector_reserve_po2_(char **data, int *len, int *cap, int entrySize, int n);
int Vector_compact_(char **data, const int *len, int *cap, int entrySize);
int Vector_insert_(char **data, int *len, int *cap, int entrySize, int idx);
void Vector_splice_(char **data, const int *len, const int *cap, int entrySize, int start, int count);
void Vector_swapSplice_(char **data, const int *len, const int *cap, int entrySize, int start, int count);
void Vector_swap_(char **data, const int *len, const int *cap, int entrySize, int idx1, int idx2);

#ifdef __cplusplus
}
#endif
