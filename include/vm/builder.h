/**
 * Copyright (c) 2022 suilteam, Carter
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Mpho Mbotho
 * @date 2022-12-15
 */

#pragma once

#include <compiler/common/log.h>
#include <vm/vm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Builder Builder;

enum {
    sytLabel,
    sytVar,
    sytDefine
};

typedef u32 SymbolTag;

typedef struct {
    u64 id;
    u32 size;
    SymbolTag tag;
    Ident name;
    Range range;
} Symbol;

Builder *Builder_create(Log *L);
u32 Builder_link(Builder *builder, Code *code);
Symbol *Builder_addSymbol(
    Builder *builder,
    u32 pos,
    SymbolTag tag,
    Ident name,
    const Range *range);

Symbol* Builder_findSymbol(Builder *builder, Ident name);

u32 Builder_addSymbolReference(Builder *builder,
                               u32 pos,
                               Ident name,
                               const Range *range,
                               bool addToPatchWork);
u32 Builder_appendIntegralData(Builder *builder, i64 value, Mode mode);
void Builder_appendInstruction_(Builder *builder, Instruction *instr);
#define Builder_appendInstruction(builder, instr) \
    Builder_appendInstruction_((builder), &(instr))

u32 Builder_appendString_(Builder *builder, const char *str, u32 len);
#define Builder_appendString(builder, str) \
    Builder_appendString_((builder), (str), strlen(str))

u32 Builder_pos(Builder *builder);

void Builder_deinit(Builder *builder);

#ifdef __cplusplus
}
#endif