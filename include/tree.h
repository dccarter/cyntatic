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

typedef int (*RbTreeCompare)(const void *lhs, u32 len, const void *rhs);
typedef void (*RbTreeElementDctor)(void *elem);
typedef void (*RbTreeDumpValue)(const void *value, char color);

typedef Pair(bool, void*) FindOrAdd;

typedef struct RbTreeNode {
    struct RbTreeNode *left;
    struct RbTreeNode *right;
    uptr               parent;
    u8                *data[0];
} __attribute__((aligned(sizeof(uptr)))) RbTreeNode;

typedef struct RbTreeBase {
    RbTreeNode *root;
    Allocator  *Alloc;
    RbTreeCompare compare;
    u32     size;
} RbTreeBase;

#define RbTree(T) struct { RbTreeBase base; T tmp; }

#define RbTree_init(self, cmp)                          \
    ( memset((self), 0, sizeof(*(self))),               \
      (self)->base.size = sizeof((self)->tmp),          \
      (self)->base.Alloc = DefaultAllocator,            \
      (self)->base.compare = (cmp) )

#define RbTree_deinit0(self, eDCTOR)                      \
    ( RbTree_deinit_(&(self)->base, (eDCTOR)),              \
      memset((self), 0, sizeof(*(self))) )

#define RbTree_deinit(self) RbTree_deinit0((self), NULL)

#define RbTree_initWith(self, cmp, A)               \
    ( memset((self), 0, sizeof(*(self))),           \
      (self)->base.size = sizeof((self)->tmp),      \
      (self)->base.Alloc = (A),                     \
      (self)->base.compare = (cmp) )

#define RbTree_ref(rbT, node)  ((__typeof__((rbT)->tmp) *) (node)->data)
#define RbTree_ref0(rbT, data) ((__typeof__((rbT)->tmp) *) (data))
#define RbTree_get(rbT, node)  *(RbTree_ref((rbT), (node)))
#define RbTree_get0(rbT, data) *(RbTree_ref0((rbT), (data)))

#define RbTree_add(rbT, VAL)                                                                    \
    ({  (rbT)->tmp = (VAL);                                                                     \
       *((__typeof__((rbT)->tmp) *)RbTree_add_(&(rbT)->base, &(rbT)->tmp, 0)) = (rbT)->tmp; })

#define RbTree_add_str0(rbT, str, len, AA)                                  \
    ({ (rbT)->tmp = (str);                                                  \
       char **LineVAR(noD) = RbTree_add_(&(rbT)->base, &(rbT)->tmp, (len)); \
       *LineVAR(noD) = Allocator_strndup((AA), (str), (len));               \
    })

#define RbTree_add_str(rbT, str, len)   \
    RbTree_add_str0((rbT), (str), (len), (rbT)->base.Alloc)

#define RbTree_find(rbT, VAL) \
    ({ (rbT)->tmp = (VAL);    \
        RbTreeNode *LineVAR(rNd) = RbTree_find_(&(rbT)->base, &(rbT)->tmp, 0); \
        ((LineVAR(rNd) != NULL)? RbTree_ref((rbT), LineVAR(rNd)): NULL); })
#define RbTree_find_str(rbT, str, len) RbTree_find_(&(rbT).base, (str), (len))

#define RbTree_find_or_add(rbT, VAL)                                            \
    ({                                                                          \
        FindOrAdd LineVAR(fOD);                                                 \
        do {                                                                    \
            (rbT)->tmp = (VAL);                                                 \
            LineVAR(fOD) = RbTree_find_or_add_(&(rbT)->base, &(rbT)->tmp, 0);   \
            if (LineVAR(fOD).f)                                                 \
                *((__typeof__((rbT)->tmp) *)LineVAR(fOD).s) = (rbT)->tmp;       \
                                                                                \
        } while (0);                                                            \
        LineVAR(fOD);                                                           \
    })

#define RbTree_find_or_create(rbT, VAL)                                          \
    ({                                                                          \
        FindOrAdd LineVAR(fOD);                                                 \
        do {                                                                    \
            (rbT)->tmp = (VAL);                                                 \
            LineVAR(fOD) = RbTree_find_or_add_(&(rbT)->base, &(rbT)->tmp, 0);   \
        } while (0);                                                            \
        LineVAR(fOD);                                                           \
    })

#define RbTree_find_or_add_str0(rbT, str, len, AA)                          \
    ({                                                                      \
        FindOrAdd LineVAR(fOD);                                             \
        do {                                                                \
            (rbT)->tmp = (str);                                             \
            LineVAR(fOD) = RbTree_find_or_add_(&(rbT)->base, &(rbT)->tmp,   \
                                                (len));                     \
            if (LineVAR(fOD).f)                                             \
                *((__typeof__((rbT)->tmp) *)LineVAR(fOD).s) =               \
                    Allocator_strndup((AA), (str), (len));                  \
                                                                            \
        } while (0);                                                        \
        LineVAR(fOD);                                                       \
    })

#define RbTree_find_or_create_str0(rbT, str, len, AA)                       \
    ({                                                                      \
        FindOrAdd LineVAR(fOD);                                             \
        do {                                                                \
            (rbT)->tmp = (str);                                             \
            LineVAR(fOD) = RbTree_find_or_add_(&(rbT)->base, &(rbT)->tmp,   \
                                              (len));                       \
        } while (0);                                                        \
        LineVAR(fOD);                                                       \
    })

#define RbTree_find_or_add_str(rbT, str, len)   \
    RbTree_find_or_add_str0((rbT), (str), (len), (rbT)->base.Alloc)

#define RbTree_find_or_create_str(rbT, str, len)   \
    RbTree_find_or_create_str0((rbT), (str), (len), (rbT)->base.Alloc)

#define RbTree_for_each_match(rbT, var, key)                                            \
    (rbT)->tmp = (key);                                                                 \
	for (RbTreeNode *var = RbTree_find_first_(&(rbT)->base, &(rbT)->tmp, 0);            \
	     (var); (var) = RbTreeNode_next_match_((var), (rbT)->base.cmp, &(rbT)->tmp, 0))

#define RbTree_for_each_match_str(rbT, var, str, len)                                       \
    (rbT)->tmp = (str);                                                                     \
	for (RbTreeNode *var = RbTree_find_first_(&(rbT)->base, &(rbT)->tmp, len);              \
	     (var); (var) = RbTreeNode_next_match_((var), (rbT)->base.cmp, &(rbT)->tmp, len))

#define RbTree_for_each(rbT, var)                                                           \
    RbTreeNode *LineVAR(iAr) = RbTree_first(&(rbT)->base);                                  \
	for (__typeof__((rbT)->tmp) * var = NULL;                                               \
	     (LineVAR(iAr) != NULL && ((var) = RbTree_ref((rbT), LineVAR(iAr))));                 \
         LineVAR(iAr) = RbTreeNode_next( LineVAR(iAr)))

void* RbTree_add_(RbTreeBase *rbt, const void *value, u32 len);
FindOrAdd RbTree_find_or_add_(RbTreeBase *rbt, const void *value, u32 len);
RbTreeNode *RbTree_find_(RbTreeBase *rbt, const void *value, u32 len);
RbTreeNode *RbTree_find_first_(RbTreeBase *rbt, const void *value, u32 len);
void RbTree_deinit_(RbTreeBase *rbt, RbTreeElementDctor dctor);
RbTreeNode *RbTreeNode_next(RbTreeNode *node);
RbTreeNode *RbTreeNode_prev(RbTreeNode *node);
RbTreeNode *RbTree_first(RbTreeBase *rbt);

void RbTree_dump_(RbTreeBase *rbt, RbTreeDumpValue dumpValue);

attr(always_inline)
static RbTreeNode *RbTreeNode_next_match_(RbTreeNode *node, RbTreeCompare cmp, const void *value, u32 len)
{
    csAssert0(cmp != NULL);

    node = RbTreeNode_next(node);
    if (node != NULL && cmp(value, len, node->data) == 0)
        return node;
    return NULL;
}

int RbTree_strncmp(const char *s1, u32 len, const char *s2);
int RbTree_strncasecmp(const char *s1, u32 len, const char *s2);

static inline
int RbTree_cmp_string(const void *lhs, u32 len, const void *rhs)
{
    const char *s1 = *((const char **)lhs), *s2 = *((const char **)rhs);
    if (len == 0)
        return strcmp(s1, s2);

    return RbTree_strncmp(s1, len, s2);
}

static inline
int RbTree_case_cmp_string(const void *lhs, u32 len, const void *rhs)
{
    const char *s1 = *((char **)lhs), *s2 = *((char **)rhs);
    if (len == 0)
        return strcasecmp(s1, s2);

    return RbTree_strncasecmp(s1, len, s2);
}

#define DECLARE_RbTree_cmp_num_(T)                                \
static inline                                                     \
int RbTree_cmp_##T(const void *lhs, u32 len, const void *rhs)     \
{                                                                 \
    T v1 = *((const T *)lhs), v2 = *((const T *)rhs);             \
    return v1 < v2? -1 : (v1 > v2? 1 : 0);                         \
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