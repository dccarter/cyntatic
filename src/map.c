/**
 * Copyright (c) 2021 suilteam, Carter 
 *
 * m library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2021-10-20
 */

#include "map.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAP_DEFAULT_BUCKET_SIZE (32)

struct Map_Node_t {
    unsigned hash;
    void    *value;
    struct  Map_Node_t *next;
    char    key[0];
};

static unsigned Map_hash_string_(const char *k)
{
#define FNV_PRIME_32 16777619
#define FNV_OFFSET_32 2166136261U
    uint32_t hash = FNV_OFFSET_32;
    for(; *k; k++)
    {
        hash ^= (*k); // xor next byte into the bottom of the hash
        hash *= FNV_PRIME_32; // Multiply by prime number found to work well
    }
    return hash;
#undef FNV_PRIME_32
#undef FNV_OFFSET_32
}
#define Map_key(n) ((n) + 1)

static Ptr(Map_Node) Map_new_node_(Allocator *A, const char *key, Ptr(void) value, unsigned vSize)
{
    Ptr(Map_Node) node;
    unsigned kSize = strlen(key) + 1;
    unsigned vOffset = kSize + (sizeof(Ptr(void)) - kSize) % sizeof(Ptr(void));

    node = (Ptr(Map_Node)) cynAlloc(A, sizeof(*node) + vOffset + vSize);
    if (node == NULL) {
        return NULL;
    }
    memcpy(Map_key(node), key, kSize);
    node->hash  = Map_hash_string_(key);
    node->value = ((char *)(Map_key(node))) + vOffset;
    memcpy(node->value, value, vSize);

    return node;
}

static inline unsigned Map_bucket_idx_(Ptr(Map_Base) m, unsigned hash)
{
    return hash & (m->bucketCount - 1);
}

static void Map_add_node_(Ptr(Map_Base) m, Ptr(Map_Node) node)
{
    unsigned n = Map_bucket_idx_(m, node->hash);
    node->next = m->buckets[n];
    m->buckets[n] = node;
}

static int Map_resize_(Ptr(Map_Base) m, unsigned numBuckets)
{
    Map_Node *node, *next, *nodes = NULL;
    Ptr(Map_Node) *buckets;
    unsigned i = m->bucketCount;
    // chain all nodes
    while (i--) {
        node = (m->buckets)[i];
        while (node) {
            next = node->next;
            node->next = nodes;
            nodes = node;
            node = next;
        }
    }

    /* reset buckets */
    buckets = (Ptr(Map_Node) *) cynReAlloc(m->A, m->buckets, sizeof(*m->buckets) * numBuckets);
    if (buckets != NULL) {
        m->buckets = buckets;
        m->bucketCount = numBuckets;
    }

    if (m->buckets != NULL) {
        memset(m->buckets, 0, sizeof(*m->buckets) * numBuckets);
        node = nodes;
        while (node) {
            next = node->next;
            Map_add_node_(m, node);
            node = next;
        }
    }

    return (buckets == NULL)? -1 : 0;
}

static Ptr(Map_Node)* Map_get_ref_(Ptr(Map_Base) m, const char *key)
{
    unsigned hash = Map_hash_string_(key);
    Ptr(Map_Node) *next;
    if (m->bucketCount > 0) {
        next = &m->buckets[Map_bucket_idx_(m, hash)];
        while (*next) {
            if ((*next)->hash == hash && (0 == strcmp((char *)Map_key(*next), key))) {
                return next;
            }
            next = &(*next)->next;
        }
    }
    return NULL;
}

int  Map_init_(Ptr(Map_Base) m, Allocator *A, unsigned initSize)
{
    cynAssert(A != NULL, "A valid allocator should be provided to a map");
    m->A = A;

    return Map_resize_(m, initSize);
}

void Map_deinit_(Ptr(Map_Base) m)
{
    Map_Node *next, *node;
    Allocator *A;
    cynAssert(m != NULL && m->A != NULL, "Undefined map");

    A = m->A;
    unsigned i = m->bucketCount;
    while (i--) {
        node = m->buckets[i];
        while (node) {
            next = node->next;
            cynDealloc(node);
            node = next;
        }
    }
    cynDealloc(m->buckets);
}

int Map_set_(Ptr(Map_Base) m, const char *key, void *value, unsigned size)
{
    unsigned n;
    Map_Node **next, *node;
    Allocator *A;
    cynAssert(m != NULL && m->A != NULL, "Undefined map");

    A = m->A;

    if ((next = Map_get_ref_(m, key))) {
        memcpy((*next)->value, value, size);
        return 0;
    }

    if ((node = Map_new_node_(A, key, value, size)) == NULL) {
        goto failed;
    }

    if (m->nodeCount >= m->bucketCount) {
        n = (m->bucketCount < MAP_DEFAULT_BUCKET_SIZE)? MAP_DEFAULT_BUCKET_SIZE : (m->bucketCount << 1);
        if (0 != Map_resize_(m, n)) {
            goto failed;
        }
    }

    Map_add_node_(m, node);
    m->nodeCount++;
    return 0;

failed:
    if (node) {
        cynDealloc(node);
    }
    return -1;
}

void Map_remove_(Ptr(Map_Base) m, const char *key)
{
    Ptr(Map_Node) node;
    Ptr(Map_Node) *next = Map_get_ref_(m, key);
    cynAssert(m != NULL && m->A != NULL, "Undefined map");

    if (next) {
        node = *next;
        *next = (*next)->next;
        cynDealloc(node);
        m->nodeCount--;
    }
}

Ptr(void) Map_get_(Ptr(Map_Base) m, const char *key)
{
    Ptr(Map_Node) *next = Map_get_ref_(m, key);
    return next? (*next)->value : NULL;
}

Map_Iter Map_begin_(Ptr(Map_Base) m)
{
    Map_Iter ite = {0};
    ite.map = m;
    if (m->bucketCount > 0) {
        Map_iter_next_(&ite);
    }

    return ite;
}

const char* Map_iter_next_(Ptr(Map_Iter) ite)
{
    Ptr(Map_Base) m = ite->map;
    if (m == NULL) {
        return NULL;
    }

    if (ite->node) {
        ite->node = ite->node->next;
        if (ite->node == NULL) goto nextBucket;
    }
    else {
    nextBucket:
        do {
            if (++ite->bucketIndex >= ite->map->bucketCount) {
                return NULL;
            }
            ite->node = m->buckets[ite->bucketIndex];
        } while (ite->node == NULL);
    }
    ite->key = (const char *) Map_key(ite->node);
    ite->val = ite->node->value;

    return ite->key;
}

#ifdef __cplusplus
}
#endif