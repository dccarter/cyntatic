/**
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

/**
 * Copyright (c) 2021 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2021-10-20
 */

#pragma once

#include <allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Map_Node_t;
typedef struct Map_Node_t Map_Node;

typedef struct {
    Ptr(Map_Node) *buckets;
    Allocator     *Alloc;
    Allocator     *eAlloc;
    unsigned bucketCount;
    unsigned nodeCount;
} Map_Base;

typedef struct CynEmptyStruct {
} EmptyStruct;

typedef struct {
    Ptr(Map_Base) map;
    Ptr(Map_Node) node;
    unsigned bucketIndex;
    const char* key;
    Ptr(void) val;
} Map_Iter;

#define Map(T) \
    struct { Map_Base base; T *ref; T tmp; }

#define Map_init(this)                                                          \
    ( memset((this), 0, sizeof(*(this))),                                       \
      Map_init_(&(this)->base, DefaultAllocator, DefaultAllocator, (sz)))

#define Map_initWith(this, AA)                                                   \
    ( memset((this), 0, sizeof(*(this))),                                        \
     (this)->base.Alloc = (AA),                                                  \
     (this)->base.eAlloc = (AA), 0 )

#define Map_init0(this, sz)                          \
    ( memset((this), 0, sizeof(*(this))),            \
      Map_init_(&(this)->base, DefaultAllocator, DefaultAllocator, (sz)) )

#define Map_init0With(this, A, sz)                          \
    ( memset((this), 0, sizeof(*(this))),                   \
     Map_init_(&(this)->base, (A), (A), (sz)) )

#define Map_init1(this, AA, EA)                                  \
    ( memset((this), 0, sizeof(*(this))),                        \
      Map_init_(&(this)->base, (AA), (EA), 0) )

#define Map_init2(this, AA, EA, sz)                              \
    ( memset((this), 0, sizeof(*(this))),                        \
      Map_init_(&(this)->base, (AA), (EA), (sz)) )

#define Map_deinit(this)        \
 ( Map_deinit_(&(this)->base), memset((this), 0, sizeof(*(this))) )

#define Map_get0(this, key, kLen) \
    ( ((this)->ref = (__typeof((this)->tmp) *) Map_get_(&(this)->base, (key), (kLen))), \
    (assert((this)->ref != NULL)), \
      *((this)->ref)           \
    )

#define Map_get(this, key) Map_get0((this), (key), strlen(key))

#define Map_key(this, key, kLen) Map_key_(&(this)->base, (key), (kLen))

#define Map_ref0(this, key, kLen) \
    ( (__typeof((this)->tmp) *) Map_get_(&(this)->base, (key), (kLen)) )

#define Map_ref(this, key) Map_ref0((this), (key), strlen(key))

#define Map_set0(this, key, kLen, value) \
    ( (this)->tmp = (value), \
    Map_set_(&(this)->base, (key), (kLen), &(this)->tmp, sizeof((this)->tmp)) )

#define Map_set(this, key, value) Map_set0((this), (key), strlen(key), (value))

#define Map_remove0(this, key, kLen) Map_remove_(&(this)->base, (key), (kLen))
#define Map_remove(this, key) Map_remove0((this), (key) strlen(key))

#define Map_contains(this, key) \
    ( Map_get_(&(this)->base, (key)) != NULL )

#define Map_begin(this) \
    Map_begin_(&(this)->base)

#define Map_iter_next(this) \
    Map_iter_next_(this)

#define Map_foreach(this, kEy, vAl)     \
    const char* kEy;                    \
    __typeof__((this)->tmp) vAl;        \
    Map_Iter LineVAR(i) = Map_begin(this); for (kEy = LineVAR(i).key; (kEy != NULL) && ((vAl = *((__typeof__((this)->tmp)*) LineVAR(i).val)), 1); kEy = Map_iter_next(&LineVAR(i)))

#define Map_foreach_ptr(this, kEy, vAl)     \
    const char* kEy;                        \
    __typeof__((this)->tmp) *vAl;           \
    Map_Iter LineVAR(i) = Map_begin(this); for (kEy = LineVAR(i).key; (kEy != NULL) && ((vAl = (__typeof__((this)->tmp)*) LineVAR(i).val), 1); kEy = Map_iter_next(&LineVAR(i)))

int         Map_init_(Ptr(Map_Base) m, Allocator *A, Allocator *EA, unsigned initSize);
void        Map_deinit_(Ptr(Map_Base) m);
Ptr(void)   Map_get_(Ptr(Map_Base) m, const char* key, u32 kLen);
char       *Map_key_(Ptr(Map_Base) m, const char* key, u32 kLen);
char       *Map_set_(Ptr(Map_Base) m, const char* key, u32 kLen, void *val, unsigned vSize);
void        Map_remove_(Ptr(Map_Base) m, const char* key, u32 kLen);
Map_Iter    Map_begin_(Ptr(Map_Base) m);
const char* Map_iter_next_(Ptr(Map_Iter) ite);

#ifdef __cplusplus
}
#endif

