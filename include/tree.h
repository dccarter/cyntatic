/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#pragma once

#include <allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*RbTree_compare)(const void *lhs, u32 len, const void *rhs);
typedef Pair(bool, void*) FindOrAdd;

typedef struct RbTreeNode {
    struct RbTreeNode *left;
    struct RbTreeNode *right;
    uptr               parent;
    u8                *data[0];
} __attribute__((aligned(sizeof(uptr)))) RbTreeNode;

typedef struct RbTree {
    RbTreeNode *root;
    Allocator  *Alloc;
    RbTree_compare compare;
    u32     size;
} RbTree;

#define RbTree(T) struct { RbTree base; T tmp; }

#define RbTree_init(self, cmp)                          \
    ( memset(&(self), 0, sizeof(*(self))),              \
      (self)->size = sizeof((self)->tmp),               \
      (self)->base.Alloc = DefaultAllocator,            \
      (self)->base.compare = (cmp) )

#define RbTree_with(self, cmp, A)                   \
    ( memset(&(self), 0, sizeof(*(self))),          \
      (self)->size = sizeof((self)->tmp),           \
      (self)->base.Alloc = (A),                     \
      (self)->base.compare = (cmp) )

#define RbTree_ref(rbT, node) (__typeof__((rbT)->tmp) *) node->data
#define RbTree_get(rbT, node) *(RbTree_ref((rbT), (node))

#define RbTree_add(rbT, VAL)                                                            \
    ({  (rbT)->tmp = (VAL);                                                             \
       *((__typeof__((rBT)->tmp) *)RbTree_add_((rbT), (rbT)->tmp, 0)) = (rbT)->tmp; })

#define RbTree_add_str0(rbT, str, len, AA)                          \
    ({ char **LineVAR(noD) = RbTree_add_((rbT), &(str), (len));     \
       *LineVAR(noD) = Allocator_strndup((AA), (str), (len));       \
    })

#define RbTree_add_str(rbT, str, len) RbTree_add_str0((rbT), (str), (len), rbT->Alloc)

#define RbTree_find(rbT, VAL) ({ (rbT)->tmp = (VAL); RbTree_find_((rbT), &(rbT)->tmp, 0); })
#define RbTree_find_str(rbT, str, len) RbTree_find_((rbT), (str), (len))s

#define RbTree_find_or_add(rbT, VAL)                                        \
    ({                                                                      \
        FindOrAdd LineVAR(fOD);                                             \
        do {                                                                \
            (rbT)->tmp = (VAL);                                             \
            LineVAR(fOD) = RbTree_find_or_add_((rbT), &(rbT)->tmp, 0);      \
            if (LineVAR(fOD).f)                                             \
                *((__typeof__((rBT)->tmp) *)LineVAR(fOD).s) = (rbT)->tmp;   \
                                                                            \
        } while (0);                                                        \
        LineVAR(fOD);                                                       \
    })

#define RbTree_find_or_add_str0(rbT, str, len, AA)                          \
    ({                                                                      \
        FindOrAdd LineVAR(fOD);                                             \
        do {                                                                \
            (rbT)->tmp = (VAL);                                             \
            LineVAR(fOD) = RbTree_find_or_add_((rbT), &(str), (len));       \
            if (LineVAR(fOD).f)                                             \
                *((__typeof__((rBT)->tmp) *)LineVAR(fOD).s) =               \
                    Allocator_strndup((AA), (str), (len));                  \
                                                                            \
        } while (0);                                                        \
        LineVAR(fOD);                                                       \
    })

#define RbTree_find_or_add_str(rbT, str, len)   \
    RbTree_find_or_add_str0((rbT), (str), (len), (rbT)->Alloc)

void* RbTree_add_(RbTree *rbt, const void *value, u32 len);
FindOrAdd RbTree_find_or_add_(RbTree *rbt, const void *value, u32 len);
RbTreeNode *RbTree_find_(RbTree *rbt, const void *value, u32 len);

static inline
int RbTree_cmp_string(const void *lhs, u32 len, const void **rhs)
{
    const char *s1 = lhs, *s2 = *rhs;
    len = len != 0? : strlen(lhs);
    return strncmp(s1, s2, len);
}

static inline
int RbTree_case_cmp_string(const void *lhs, u32 len, const void *rhs)
{
    const char *s1 = *((char **)lhs), *s2 = *((char **)rhs);
    len = len != 0? : strlen(lhs);
    return strncasecmp(s1, s2, len);
}

#define DECLARE_RbTree_cmp_num_(T)                                \
static inline                                                     \
int RbTree_cmp_##T(const void *lhs, u32 len, const void *rhs)     \
{                                                                 \
    T v1 = *((const T *)lhs), v2 = *((const T *)rhs);             \
    return v1 < v2? 1 : (v1 > v2? 1 : 0);                         \
}

DECLARE_RbTree_cmp_num_(u64);
DECLARE_RbTree_cmp_num_(i64);
DECLARE_RbTree_cmp_num_(u32);
DECLARE_RbTree_cmp_num_(i32);
DECLARE_RbTree_cmp_num_(u16);
DECLARE_RbTree_cmp_num_(i16);
DECLARE_RbTree_cmp_num_(u8);
DECLARE_RbTree_cmp_num_(i8);
DECLARE_RbTree_cmp_num_(f64);
DECLARE_RbTree_cmp_num_(f32);

#ifdef __cplusplus
}
#endif