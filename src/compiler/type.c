/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-08-12
 */

#include "compiler/type.h"

static Type s_voidType = {.id = tpVoid, .size = 0};
static Type s_boolType = {.id = tpBool, .size = 1};
static Type s_charType = {.id = tpChar, .size = sizeof(u32) };
static Type s_f32Type  = {.id = tpFloat, .size = sizeof(f32)};
static Type s_f64Type  = {.id = tpFloat, .size = sizeof(f64)};

static IntegerType s_i8Type  = {.id = tpInteger, .size = sizeof(u8),  .isSigned = true};
static IntegerType s_u8Type  = {.id = tpInteger, .size = sizeof(u8),  .isSigned = false};
static IntegerType s_i16Type = {.id = tpInteger, .size = sizeof(u16), .isSigned = true};
static IntegerType s_u16Type = {.id = tpInteger, .size = sizeof(u16), .isSigned = false};
static IntegerType s_i32Type = {.id = tpInteger, .size = sizeof(u32), .isSigned = true};
static IntegerType s_u32Type = {.id = tpInteger, .size = sizeof(u32), .isSigned = false};
static IntegerType s_i64Type = {.id = tpInteger, .size = sizeof(u64), .isSigned = true};
static IntegerType s_u64Type = {.id = tpInteger, .size = sizeof(u64), .isSigned = false};


const Type *voidType(void)
{
 return &s_voidType;
}

const Type *boolType(void)
{
    return &s_boolType;
}

const Type *charType(void)
{
    return &s_charType;
}

const Type *i8Type(void)
{
    return (Type *) &s_i8Type;
}

const Type *u8Type(void)
{
    return (Type *) &s_u8Type;
}

const Type *i16Type(void)
{
    return (Type *) &s_i16Type;
}

const Type *u16Type(void)
{
    return (Type *) &s_u16Type;
}

const Type *i32Type(void)
{
    return (Type *) &s_i32Type;
}

const Type *u32Type(void)
{
    return (Type *) &s_u32Type;
}

const Type *i64Type(void)
{
    return (Type *) &s_i64Type;
}

const Type *u64Type(void)
{
    return (Type *) &s_u64Type;
}

const Type *f32Type(void)
{
    return (Type *) &s_f32Type;
}

const Type *f64Type(void)
{
    return (Type *) &s_f64Type;
}

bool Type_is_same(const Type *lhs, const Type *rhs)
{
    if (lhs == rhs) return true;
    if (lhs->id != rhs->id) return false;

    switch (lhs->id) {
        case tpPointer:
            return Type_is_same(lhs->tPointer.pointee, rhs->tPointer.pointee);
        case tpArray:
            return lhs->tArray.len == rhs->tArray.len &&
                   Type_is_same(lhs->tArray.elem, rhs->tArray.elem);
        case tpStruct:
        case tpFunc:
            return false;
        default:
            return true;
    }
}