/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-08-12
 */

#include "compiler/parser.h"
#include "compiler/common/log.h"
#include "compiler/common/itp.h"

typedef struct {
    Log   *L;
    ITokenParser itp;
} Parser;

void Parser_init(Parser *P, Lexer *lX)
{
    ITP_init(&P, lX);
}

AstModule *parse(Lexer *lX)
{
    Parser P;
    Parser_init(&P, lX);
}