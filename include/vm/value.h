/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-24
 */

#pragma once

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union ValueT {
    f64 f;
    u64 u;
    i64 i;
    u8  b64[sizeof(f64)];
} Value;

#define f2u(V) ({ Value LineVAR(val) = {.f = (V)}; LineVAR(val).u; })
#define f2i(V) ({ Value LineVAR(val) = {.f = (V)}; LineVAR(val).i; })
#define i2u(V) ({ Value LineVAR(val) = {.i = (V)}; LineVAR(val).u; })
#define i2f(V) ({ Value LineVAR(val) = {.i = (V)}; LineVAR(val).f; })
#define u2f(V) ({ Value LineVAR(val) = {.u = (V)}; LineVAR(val).f; })
#define u2i(V) ({ Value LineVAR(val) = {.u = (V)}; LineVAR(val).i; })
#define u2v(U) (Value){.u = (U)}
#define i2v(I) (Value){.i = (I)}
#define f2v(F) (Value){.f = (F)}
#define v2u(V) (V).u
#define v2f(V) (V).f
#define v2i(V) (V).i
#define p2v(P) (Value){.u = (uptr)(P)}
#define v2p(V) (uptr) v2u(V)
#define i2uX(V, X) ({ union {i##X i; u##X u; } LineVAR(val) = {.i = (i##X)(V)}; LineVAR(val).u; })
#define u2iX(V, X) ({ union {i##X i; u##X u; } LineVAR(val) = {.u = (u##X)(V)}; LineVAR(val).i; })
    
#ifdef __cplusplus
}
#endif