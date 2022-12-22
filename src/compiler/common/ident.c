/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */


#include "compiler/common/ident.h"
#include "compiler/common/heap.h"

#include "map.h"
#include "tree.h"
#include <stdio.h>

static RbTree(char *) sIdentCache;
static u32 sVariableCounts = 0;

void IdentCache_init(void)
{
    RbTree_initWith(&sIdentCache, RbTree_cmp_string, ArenaAllocator);

    // Add commonly used identifiers
    RbTree_add(&sIdentCache, "i");
    RbTree_add(&sIdentCache, "j");
    RbTree_add(&sIdentCache, "k");
}

Ident Ident_foa(const char *str, u32 size)
{
    FindOrAdd foa = RbTree_find_or_create_str(&sIdentCache, (char *)str, size);
    if (foa.f) {
        str = RbTree_get0(&sIdentCache, foa.s) = Allocator_strndup(ArenaAllocator, str, size);
        csAssert(str != NULL, "Adding to identifier cache failed, out of memory?");
    }
    return (Ident){.name = RbTree_get0(&sIdentCache, foa.s)};
}

Ident Ident_genVariable(const char *prefix)
{
    u32 len = strlen(prefix);
    char str[len + 16];
    strncpy(str, prefix, len);
    len += sprintf(&str[len], "%u", sVariableCounts++);
    return Ident_foa(str, len);
}
