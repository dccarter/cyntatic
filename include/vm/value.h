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

#define f2u(V) ({ Value LineVAR(v) = {.f = (V)}; LineVAR(v).u; })
#define f2i(V) ({ Value LineVAR(v) = {.f = (V)}; LineVAR(v).i; })
#define i2u(V) ({ Value LineVAR(v) = {.i = (V)}; LineVAR(v).u; })
#define i2f(V) ({ Value LineVAR(v) = {.i = (V)}; LineVAR(v).f; })
#define u2f(V) ({ Value LineVAR(v) = {.u = (V)}; LineVAR(v).f; })
#define u2i(V) ({ Value LineVAR(v) = {.u = (V)}; LineVAR(v).i; })

typedef struct Field {
    cstring name;
} Field;

typedef struct Function {
    cstring name;

} Function;

typedef struct ObjectMeta {
    cstring name;
    u32 nFields;
    Field fields[0];
} ObjectMeta;

typedef struct Object {
    Ptr(ObjectMeta) meta;
    Value data[0];
} Object;

#define newVal(V)    (Value){.val = (f64)(V)}
#define cast(N, T)  ((T)(uptr)(N).val)

Value objectGetField(Value obj, u16 field);
void objectSetField(Value obj, u16 field, Value value);
#ifdef __cplusplus
}
#endif