/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */

#pragma once

#include <view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CynIdent {
    char *name;
} Ident;

void IdentCache_init(void);

Ident Ident_foa(const char *str, u32 size);

#define Ident_equals(LHS, RHS) ((LHS)->name == (RHS)->name)

#ifdef __cplusplus
}
#endif