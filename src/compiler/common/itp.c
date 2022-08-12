/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-08-12
 */

#include <compiler/common/itp.h>

E4C_DEFINE_EXCEPTION(ITPException, "Parsing error", RuntimeException);

bool ITokenParser_check_(ITokenParser *itp, const TokenKind *kinds, u32 count)
{
    if (ITP_eof(itp)) return false;

    for (int i = 0; i < count; i++) {
        if (kinds[i] == itp->curr.kind) return true;
    }
    return false;
}

bool ITokenParser_match_(ITokenParser *itp, const TokenKind *kinds, u32 count)
{
    if (ITokenParser_check_(itp, kinds, count)) {
        ITP_advance(itp);
        return true;
    }
    return false;
}

void ITokenParser_init(ITokenParser *itp, Lexer *lX)
{
    memset(itp, 0, sizeof(*itp));
    itp->L = lX->L;
    itp->lX = lX;

    itp->peek.kind = tokCOUNT;
    Lexer_next(itp->lX, &itp->curr);
}

Token* ITokenParser_advance(ITokenParser *itp)
{
    if (itp->curr.kind == tokEoF)
        return &itp->curr;

    itp->prev = itp->curr;
    if (itp->peek.kind != tokCOUNT) {
        itp->curr = itp->peek;
        itp->peek.kind = tokCOUNT;
    }
    else
        Lexer_next(itp->lX, &itp->curr);

    return &itp->prev;
}

Token* ITokenParser_peek(ITokenParser *itp)
{
    if (itp->curr.kind == tokEoF)
        return &itp->curr;

    if (itp->peek.kind == tokCOUNT)
        Lexer_next(itp->lX, &itp->peek);

    return &itp->peek;
}