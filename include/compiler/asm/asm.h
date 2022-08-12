/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#pragma once

#include "compiler/common/lexer.h"
#include "vm/instr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AssemblerCtx AssemblerCtx;

typedef struct Assembler {
    AssemblerCtx *ctx;
} Assembler;

void Assembler_init(Assembler *as, Lexer *lX);
void Assembler_deinit(Assembler *as);
u32  Assembler_assemble(Assembler *as, Code *into);

#ifdef __cplusplus
}
#endif