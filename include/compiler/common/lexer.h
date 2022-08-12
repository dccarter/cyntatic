/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-26
 */

#pragma once

#include "token.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    lxfInStrExpr = BIT(0)
} LexerFlags;

typedef struct Lexer {
    struct Log_t *L;
    Source        *src;
    char          *conv;
    Token          next;
    u32            idx;
    LineColumn     pos;
    LexerFlags     flags;
} Lexer;

void Lexer_init(Lexer *lX, struct Log_t *log, Source *src);
void Lexer_reset(Lexer *lX, Source *src);
TokenKind Lexer_next(Lexer *lX, Token *out);

#ifdef __cplusplus
}
#endif