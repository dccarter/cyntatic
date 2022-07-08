
/**
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include "vector.h"

#define MINIMUM_SIZE 8

static int round2pow2(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;

}

int Vector_expand_(char **data, const int *len, int *cap, int entrySize)
{
    if (*len + 1 > *cap) {
        void *ptr;
        int n = (*cap < MINIMUM_SIZE)? MINIMUM_SIZE : (*cap << 1);
        ptr = realloc(*data, n * entrySize);
        if (ptr == NULL) {
            return -1;
        }
        *data = (char *) ptr;
        *cap = n;
    }
    return 0;
}

int Vector_reserve_(char **data, attr(unused) const int *len, int *cap, int entrySize, int n)
{
    if (n > *cap) {
        void *ptr = realloc(*data, n * entrySize);
        if (ptr == NULL) {
            return -1;
        }
        *data = (char *) ptr;
        *cap = n;
    }
    return 0;
}

int Vector_reserve_po2_(char **data, int *len, int *cap, int entrySize, int n)
{
    n = round2pow2(n);
    return Vector_reserve_(data, len, cap, entrySize, n);
}

int Vector_compact_(char **data, const int *len, int *cap, int entrySize)
{
    if (*len == 0) {
        free(*data);
        *data = NULL;
        *cap = 0;
        return 0;
    }
    else {
        void *ptr = realloc(*data, *len * entrySize);
        if (ptr == NULL) {
            return -1;
        }
        *cap = *len;
        *data = (char *) ptr;
    }
    return 0;
}

int Vector_insert_(char **data, int *len, int *cap, int entrySize, int idx)
{
    if (-1 == Vector_expand_(data, len, cap, entrySize)) {
        return -1;
    }
    memmove(Ptr_off(*data, ((idx + 1) * entrySize)),
            Ptr_off(*data, (idx * entrySize)),
            (*len - idx) * entrySize);
    return 0;
}

void Vector_splice_(char **data, const int *len, attr(unused) const int *cap, int entrySize, int start, int count)
{
    memmove(Ptr_off0(*data, start, entrySize),
            Ptr_off0(*data, (start + count), entrySize),
            ((*len - start - count) * entrySize));
}

void Vector_swapSplice_(char **data, const int *len, attr(unused) const int *cap, int entrySize, int start, int count)
{
    memmove(Ptr_off0(*data, start, entrySize),
            Ptr_off0(*data, (*len - count), entrySize),
            count * entrySize);
}

void Vector_swap_(char **data, attr(unused) const int *len, const int *cap, int entrySize, int idx1, int idx2)
{
    unsigned char *a, *b;
    int count;
    if (idx1 == idx2) {
        return;
    }
    a = Ptr_off(*data, idx1 * entrySize);
    b = Ptr_off(*data, idx2 * entrySize);
    count = entrySize;
    while (count--) {
        unsigned char tmp = *a;
        *a = *b;
        *b = tmp;
        a++, b++;
    }
}
