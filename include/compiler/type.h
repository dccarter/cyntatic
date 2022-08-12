/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-08-12
 */

#pragma once

#include <common.h>
#include <map.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CYN_TYPE_LIST(XX)       \
    XX(Void)                    \
    XX(Bool)                    \
    XX(Char)                    \
    XX(Integer)                 \
    XX(Float)                   \
    XX(String)                  \
    XX(Pointer)                 \
    XX(Array)                   \
    XX(Struct)                  \
    XX(Func)                    \

enum {
#define XX(N) tp##N,
    CYN_TYPE_LIST(XX)
#undef XX
};

#define IType struct {     \
    u8  id;                \
    u32 size;              \
    u32 align;             \
}

typedef IType TypeInf;

typedef union Type Type;

typedef TypeInf VoidType;
typedef TypeInf BoolType;
typedef TypeInf FloatType;
typedef TypeInf CharType;
typedef TypeInf StringType;

typedef struct {
    IType;
    bool isSigned;
} IntegerType;

typedef struct {
    IType;
    Type *pointee;
} PointerType;

typedef struct {
    IType;
    Type *elem;
    u32   len;
} ArrayType;

typedef struct {
    IType;
    Map(Type*) members;
    int offset;
} StructType;

typedef struct {
    IType;
    Type *returns;
} FuncType;

union Type {
    IType;
#define XX(N) N##Type t##N;
    CYN_TYPE_LIST(XX)
#undef XX
};

const Type *voidType(void);
const Type *boolType(void);
const Type *charType(void);
const Type *i8Type(void);
const Type *u8Type(void);
const Type *i16Type(void);
const Type *u16Type(void);
const Type *i32Type(void);
const Type *u32Type(void);
const Type *i64Type(void);
const Type *u64Type(void);
const Type *f32Type(void);
const Type *f64Type(void);

attr(always_inline)
static Type *Type_func(Allocator *A, Type *returns)
{
    return (Type *) New(A, FuncType, .id = tpFloat, .size = 8, .returns = returns);
}

attr(always_inline)
static Type *Type_pointer(Allocator *A, Type *target)
{
    return (Type *) New(A, PointerType,
                        .id = tpPointer,
                        .size = 8,
                        .align = 8,
                        .pointee = target);
}

attr(always_inline)
static Type *Type_array(Allocator *A, Type *elem, u32 len)
{
    return (Type *) New(A, ArrayType,
                         .id = tpArray,
                         .size  = elem->size * len,
                         .align = elem->align,
                         .elem  = elem,
                         .len   = len);
}

attr(always_inline)
static Type *Type_string(Allocator *A, u32 len)
{
    return (Type *) New(A, StringType, .id = tpString, .size = len, .align = 8);
}

bool Type_is_same(const Type *lhs, const Type *rhs);

#ifdef __cplusplus
}
#endif
