/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */

#pragma once

#include "source.h"

#include "stream.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TOKEN_LIST(XX, YY, ZZ, BB) \
    XX(EoF,                             "<eof>") \
    XX(Nl,                              "<nl>")         \
    XX(Char,                            "<char>")       \
    XX(String,                          "<string>")     \
    XX(Integer,                         "<integer>")    \
    XX(Float,                           "<float>")      \
    XX(Identifier,                      "<identifier>") \
    XX(Comment,                         "<comment>")    \
    \
    XX(Assign,                          "'='")       \
    XX(BitAnd,                          "'&'")       \
    XX(BitAndAssign,                    "'&='")      \
    XX(BitOr,                           "'|'")       \
    XX(BitOrAssign,                     "'|='")      \
    XX(BitXor,                          "'^'")       \
    XX(BitXorAssign,                    "'^='")      \
    XX(Colon,                           "':'")       \
    XX(DColon,                          "'::'")      \
    XX(Comma,                           "','")       \
    XX(Complement,                      "'~'")       \
    XX(CompAssign,                      "'~='")      \
    XX(Div,                             "'/'")       \
    XX(Dot,                             "'.'")       \
    XX(DotDot,                          "'..'")      \
    XX(Elipsis,                         "'...")      \
    XX(DivAssign,                       "'/='")      \
    XX(Equal,                           "'=='")      \
    XX(Gt,                              "'>'")       \
    XX(Gte,                             "'>='")      \
    XX(Lt,                              "'<'")       \
    XX(Lte,                             "'<='")      \
    XX(LBrace,                          "'{'")       \
    XX(RBrace,                          "'}'")       \
    XX(LBracket,                        "'['")       \
    XX(RBracket,                        "']'")       \
    XX(LParen,                          "'('")       \
    XX(RParen,                          "')'")       \
    XX(LAnd,                            "'&&'")     \
    XX(LOr,                             "'||'")      \
    XX(Minus,                           "'-'")       \
    XX(MinusMinus,                      "'--'")      \
    XX(MinisAssign,                     "'-='")      \
    XX(Mult,                            "'*'")       \
    XX(Exponent,                        "'**'")      \
    XX(MultAssign,                      "'*='")      \
    XX(Not,                             "'!'")       \
    XX(Neq,                             "'!=')")     \
    XX(Plus,                            "'+'")       \
    XX(PlusPlus,                        "'++'")      \
    XX(PlusAssign,                      "'+='")      \
    XX(Mod,                             "'%'")       \
    XX(ModAssign,                       "'%='")      \
    XX(Question,                        "'?'")       \
    XX(Semicolon,                       "';'")       \
    XX(Sal,                             "'<<'")      \
    XX(SalAssign,                       "'=<<'")     \
    XX(Sar,                             "'>>'")      \
    XX(SarAssign,                       "'>>='")     \
    XX(LArrow,                          "'<-'")      \
    XX(RArrow,                          "'->'")      \
    XX(At,                              "'@'")       \
    XX(Hash,                            "'#'")       \
    XX(Backquote,                       "'`")        \
    XX(Dollar,                          "'$'")       \
    XX(BackSlash,                       "'\'")       \
    \
    YY(Alingof,                        "alignof") \
    YY(As,                              "as")      \
    YY(Auto,                            "auto")    \
    YY(Break,                           "break")   \
    YY(Case,                            "case")    \
    YY(Continue,                        "continue")\
    YY(Const,                           "const")   \
    YY(Else,                            "else")    \
    YY(Enum,                            "enum")    \
    YY(Extern,                          "extern")  \
    YY(False,                           "false")   \
    YY(For,                             "for")     \
    YY(Func,                            "func")    \
    YY(If,                              "if")      \
    YY(Imm,                             "iu")      \
    YY(In,                              "in")      \
    YY(Inline,                          "inline")  \
    YY(Import,                          "import")  \
    YY(Macro,                           "macro")   \
    YY(Move,                            "move")    \
    YY(Mut,                             "mut")     \
    YY(New,                             "new")     \
    YY(Null,                            "null")    \
    YY(Return,                          "return")  \
    YY(Sizeof,                          "sizeof")  \
    YY(Static,                          "static")  \
    YY(Struct,                          "struct")  \
    YY(Switch,                          "switch")  \
    YY(This,                            "this")    \
    YY(Trait,                           "trait")   \
    YY(True,                            "true")    \
    YY(Unsafe,                          "unsafe")  \
    YY(Union,                           "union")   \
    YY(Using,                           "using")   \
    YY(While,                           "while")   \
    YY(Void,                            "void")    \
    BB(line,                            "line")    \
    BB(Column,                          "column")  \
    BB(FileExpr,                        "file")    \
    BB(ArgExpr,                         "arg")     \
    BB(Opaque,                          "opaque")  \
    \
    ZZ(And,                             "and", LAnd) \
    ZZ(Or,                              "or",  LOr)  \
    \
    XX(LStrExpr,                        "'>strexpr'")\
    XX(RStrExpr,                        "'<strexpr'")

#define STMT_MARK_TOKENS(XX) \
        XX(Break)       \
        XX(Case)        \
        XX(Continue)    \
        XX(Else)        \
        XX(Enum)        \
        XX(Extern)      \
        XX(For)         \
        XX(Func)        \
        XX(If)          \
        XX(Imm)         \
        XX(Inline)      \
        XX(Import)      \
        XX(Macro)       \
        XX(Return)      \
        XX(Static)      \
        XX(Struct)      \
        XX(Switch)      \
        XX(Trait)       \
        XX(Union)       \
        XX(Using)       \
        XX(While)

typedef enum {
    vkdNone,
    vkdBool,
    vkdChar,
    vkdInt,
    vkdFloat,
    vkdString
} ValueKind;

typedef enum {
    Int8    = 1,
    Int16   = 2,
    Int32   = 4,
    Int64   = 8
} IntSize;

typedef enum {
    Float32 = 4,
    Float64 = 8
} FloatSize;

typedef struct CynTokenValue {
    ValueKind kind;
    union {
        bool b;
        u32  c;
        u64  i;
        f64  f;
        char *s;
    };
} TokenValue;

typedef enum {
#define XX(NAME, _)     tok##NAME,
#define ZZ(NAME, ...)   tok##NAME,
#define YY(NAME, _)     tok##NAME,
#define BB(NAME, _)     tok##NAME,
    TOKEN_LIST(XX, YY, ZZ, BB)
#undef YY
#undef ZZ
#undef XX
#undef BB

    tokCOUNT
} TokenKind;

typedef struct CynToken {
    TokenKind kind;
    TokenValue value;
    Range range;
} Token;

void Token_init(TokenKind *kind);

attr(always_inline)
static void Token_setBool(Token* token, bool b)
{
    token->value.b = b;
    token->value.kind = vkdBool;
}

attr(always_inline)
static void Token_setChar(Token* token, u32 chr)
{
    token->value.c = chr;
    token->value.kind = vkdChar;
}

attr(always_inline)
static void Token_setInt(Token* token, u64 num)
{
    token->value.i = num;
    token->value.kind = vkdInt;
}

attr(always_inline)
static void Token_setFloat(Token* token, f64 flt)
{
    token->value.f = flt;
    token->value.kind = vkdInt;
}

attr(always_inline)
static void Token_setString(Token* token, char *s)
{
    token->value.s = s;
    token->value.kind = vkdString;
}

attr(always_inline)
static u64 Token_getInt(const Token* token)
{
    csAssert0(token->value.kind == vkdInt);
    return token->value.i;
}

attr(always_inline)
static u32 Token_getChar(const Token* token)
{
    csAssert0(token->value.kind == vkdChar);
    return token->value.c;
}

attr(always_inline)
static bool Token_getBool(const Token* token)
{
    csAssert0(token->value.kind == vkdBool);
    return token->value.b;
}

attr(always_inline)
static f64 Token_getFloat(const Token* token)
{
    csAssert0(token->value.kind == vkdFloat);
    return token->value.f;
}

attr(always_inline)
static char* Token_getString(Token* token)
{
    csAssert0(token->value.kind == vkdString);
    return token->value.s;
}

attr(always_inline)
static bool TokenKind_isKeyword(TokenKind kind)
{
    return (kind >= tokAlingof) && (kind <= tokVoid);
}

StringView TokenKind_toString(TokenKind kind);

void Token_toString0(Token *tok, Stream *stream);

#define Token_isKeyword(T) TokenKind_isKeyword((T)->kind)

bool Token_isBinaryOp(const Token* tok);
bool Token_isAssignOp(const Token* tok);
bool Token_isUnaryOp(const Token* tok);
bool Token_isTernaryOp(const Token* tok);
bool Token_isStmtBoundary(const Token* tok);
bool Token_isLogicalOp(const Token* tok);

#define Token_has(T, K) (T)->value.kind == vkd##K
#define Token_get(T, K) Token_get##K(T)
#define Token_set(T, K, V) Token_set##K((T), (V))


#ifdef __cplusplus
}
#endif