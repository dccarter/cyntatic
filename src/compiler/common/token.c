/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */

#include "compiler/common/token.h"

#include <inttypes.h>

StringView TokenKind_toString(TokenKind kind)
{
    switch(kind) {
#define XX(NAME, STR)       case tok##NAME: return (StringView){STR, sizeof(STR)-1};
#define YY(NAME, STR)       case tok##NAME: return (StringView){STR, sizeof(STR)-1};
#define ZZ(NAME, STR, _)    case tok##NAME: return (StringView){STR, sizeof(STR)-1};
#define BB(NAME, STR)       case tok##NAME: return (StringView){ "@" STR, sizeof(STR)};
        TOKEN_LIST(XX, YY, ZZ, BB)
#undef BB
#undef ZZ
#undef YY
#undef XX
        default:
            return (StringView) {"<unknown>", 9};
    }
}

bool Token_isBinaryOp(const Token* tok)
{
    csAssert0(tok != NULL);
    switch(tok->kind) {
        case tokPlus:
        case tokMinus:
        case tokMult:
        case tokDiv:
        case tokMod:
        case tokEqual:
        case tokNeq:
        case tokLt:
        case tokGt:
        case tokLte:
        case tokGte:
        case tokLAnd:
        case tokLOr:
        case tokBitAnd:
        case tokBitOr:
        case tokBitXor:
        case tokSal:
        case tokSar:
        case tokAssign:
        case tokPlusAssign:
        case tokMinisAssign:
        case tokMultAssign:
        case tokDivAssign:
        case tokModAssign:
        case tokBitAndAssign:
        case tokBitOrAssign:
        case tokBitXorAssign:
        case tokSalAssign:
        case tokSarAssign:
            return true;
        default:
            return false;
    }
}

bool Token_isAssignOp(const Token* tok)
{
    csAssert0(tok != NULL);
    switch(tok->kind) {
        case tokPlusAssign:
        case tokMinisAssign:
        case tokMultAssign:
        case tokDivAssign:
        case tokModAssign:
        case tokBitAndAssign:
        case tokBitOrAssign:
        case tokBitXorAssign:
        case tokSalAssign:
        case tokSarAssign:
            return true;
        default:
            return false;
    }
}

bool Token_isUnaryOp(const Token* tok)
{
    csAssert0(tok != NULL);
    switch (tok->kind) {
        case tokPlus:
        case tokMinus:
        case tokNot:
        case tokComplement:
        case tokMinusMinus:
        case tokPlusPlus:
            return true;
        default:
            return false;

    }
}

bool Token_isTernaryOp(const Token* tok)
{
    csAssert0(tok != NULL);
    return tok->kind == tokQuestion;
}

bool Token_isStmtBoundary(const Token* tok)
{
    csAssert0(tok != NULL);
    switch(tok->kind) {
#define XX(N) case tok##N:
        STMT_MARK_TOKENS(XX) return true;
#undef XX
        default: return false;
    }
}

bool Token_isLogicalOp(const Token* tok)
{
    csAssert0(tok != NULL);
    switch (tok->kind) {
        case tokEqual:
        case tokNeq:
        case tokGt:
        case tokGte:
        case tokLt:
        case tokLte:
        case tokLAnd:
        case tokLOr:
            return true;
        default:
            return false;
    }
}

void Token_toString0(Token *tok, Stream *os)
{
    csAssert0(tok != NULL);
    switch (tok->kind) {
        case tokInteger:
            Stream_printf(os, "<integet: %" PRIu64 ">", tok->value.i);
            break;
        case tokFloat:
            Stream_printf(os, "<float: %g>", tok->value.f);
            break;
        case tokChar:
            Stream_puts(os, "<char: ");
            Stream_putUtf8(os, tok->value.c);
            Stream_putc(os, '>');
            break;
        case tokString:
            Stream_puts(os, "<string: ");
            Stream_puts(os, tok->value.s);
            Stream_putc(os, '>');
            break;
        case tokIdentifier: {
            StringView sv = Range_view(&tok->range);
            Stream_puts(os, "<ident: ");
            Stream_write(os, sv.data, sv.count);
            Stream_putc(os, '>');
            break;
        }
        default: {
            StringView sv = TokenKind_toString(tok->kind);
            Stream_write(os, sv.data, sv.count);
            break;
        }
    }
}
