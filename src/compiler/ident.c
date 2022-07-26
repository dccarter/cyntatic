/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */


#include "compiler/ident.h"
#include "compiler/heap.h"
#include "compiler/token.h"

#include "map.h"

Map(EmptyStruct) sIdentCache;

void IdentCache_init(void)
{
    Map_init0With(&sIdentCache, ArenaAllocator, 1024);

    // Add commonly used identifiers
    Map_set0(&sIdentCache, "i", 1, (EmptyStruct){});
    Map_set0(&sIdentCache, "j", 1, (EmptyStruct){});
    Map_set0(&sIdentCache, "k", 1, (EmptyStruct){});
}

Ident Ident_foa(const char *str, u32 size)
{
    char *key = Map_key(&sIdentCache, str, size);
    if (key == NULL)
        key = Map_set0(&sIdentCache, str, size, (EmptyStruct){});

    csAssert(key != NULL, "Adding to identifier cache failed, out of memory?");

    return (Ident){.name = key};
}
