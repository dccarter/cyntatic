/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-26
 */

#include "compiler/lexer.h"
#include "compiler/log.h"
#include "map.h"

#include <ctype.h>
#include <errno.h>

#define MAX_DIGITS_IN_INTEGER  39
#define CONVERSION_BUFFER_SIZE 1084

typedef Pair(u32, u32) u32_u32_pair;

#define isoct(C) (('0' <= (C)) && ((C) <= '7'))
#define Lexer_mark(LX) (Position){(lX)->idx, (lX)->pos}
#define mkRange(S, E, C, SRC)  &((Range){(S), (E), (C), (SRC)})

static Map(TokenKind) sKeyWordList;
static bool sKeyWordListInitialized = false;

attr(always_inline)
static bool isucn(u32 c)
{
    if (0xD800 <= c && c <= 0xDFFF)
        return false;
    return 0xA0 <= c || c == '$' || c == '@' || c == '`';
}

attr(always_inline)
static void Lexer_updatePos(Lexer *lX, char c)
{
    if (c == '\n') {
        ++lX->pos.line;
        lX->pos.column = 0;
    }
    else
        ++lX->pos.column;
}

u32 Lexer_clz(char c)
{
    for (int i = 7; i >= 0; i--) {
        if ((c & (1 << i)) == 0)
            return 7 - i;
    }
    return 8;
}

static void Lexer_eatWhile(Lexer *lX, int(*func)(int))
{
    u32 max = Source_len(lX->src);
    const char *src = Source_src(lX->src);
    while (lX->idx < max && func(src[lX->idx])) {
        Lexer_updatePos(lX, src[lX->idx]);
        ++lX->idx;
    }
}

static void Lexer_eatUntil(Lexer *lX, bool(*func)(char))
{
    u32 max = Source_len(lX->src);
    const char *src = Source_src(lX->src);
    while (lX->idx < max && !func(src[lX->idx])) {
        Lexer_updatePos(lX, src[lX->idx]);
        ++lX->idx;
    }
}

attr(always_inline)
static bool isCharIn(const char chrs[], u8 len, char c)
{
    for (u8 i = 0; i < len; i++)
        if (chrs[i] == c) return true;
    return false;
}

static void Lexer_eatUntilAnyOf_(Lexer *lX, const char chrs[], u8 len)
{
    u32 max = Source_len(lX->src);
    const char *src = Source_src(lX->src);
    while (lX->idx < max && !isCharIn(chrs, len, src[lX->idx])) {
        Lexer_updatePos(lX, src[lX->idx]);
        ++lX->idx;
    }
}

#define Lexer_eatUntilAnyOf(LX, ...) \
    ({char LineVAR(chrs)[] = {__VA_ARGS__}; Lexer_eatUntilAnyOf_((LX), LineVAR(chrs), sizeof(LineVAR(chrs))); })

attr(always_inline)
static char Lexer_peek(Lexer *lX, u32 n)
{
    n += lX->idx;
    if (n < Source_len(lX->src))
        return *Source_at(lX->src, n);
    return EOF;
}

static u32 Lexer_advance(Lexer *lX, u32 n)
{
    u32 ret = lX->idx;
    const char *src = Source_src(lX->src);

    lX->idx = MIN(Source_len(lX->src), ret + n);

    for (u32 i = ret; i < lX->idx; ++i)
        Lexer_updatePos(lX, src[i]);

    return ret;
}

attr(always_inline)
static TokenKind Lexer_updateToken(Lexer *lX, Token *tok, TokenKind kind, u32 start, u32 end, LineColumn pos)
{
    Range_update(&tok->range, lX->src, start, end, pos);
    tok->kind = kind;
    return kind;
}

attr(always_inline)
static TokenKind Lexer_resetNext(Lexer *lX, Token *into)
{
    TokenKind kind = lX->next.kind;

    if (into != NULL)
        *into = lX->next;
    lX->next.kind = tokCOUNT;

    return kind;
}

attr(noreturn)
void Lexer_abort(Lexer *lX)
{
    abortCompiler(lX->L, "lexical analysis failed.");
}


static void Lexer_initKeywordsMap(void)
{
    if (sKeyWordListInitialized) return;
    Map_initWith(&sKeyWordList, ArenaAllocator);

#define YY(TOK, NAME)           Map_set(&sKeyWordList, NAME, tok##TOK);
#define ZZ(TOK, NAME, ALIAS)    Map_set(&sKeyWordList, NAME, tok##ALIAS);
#define XX(...)
#define BB(TOK, NAME)           Map_set(&sKeyWordList, NAME, tok##TOK);

    TOKEN_LIST(XX, YY, ZZ, BB)

#undef BB
#undef XX
#undef ZZ
#undef YY
}

static TokenKind Lexer_tokenize(Lexer *lX, Token *out);
static TokenKind Lexer_tokComment(Lexer *lX, Token *out);
static TokenKind Lexer_tokNumber(Lexer *lX, Token *out);
static TokenKind Lexer_tokString(Lexer *lX, Token *out, Position pos);
static TokenKind Lexer_tokChar(Lexer *lX, Token *out);
static TokenKind Lexer_tokIdent(Lexer *lX, Token *out);
static TokenKind Lexer_tokBinaryNumber(Lexer *lX, Token *tok);
static TokenKind Lexer_tokHexNumber(Lexer *lX, Token *tok);
static TokenKind Lexer_tokOctalNumber(Lexer *lX, Token *tok);
static TokenKind Lexer_tokDecimalNumber(Lexer *lX, Token *tok);
static TokenKind Lexer_parseInteger(Lexer *lX, Token *tok, const Position *start, int base);
static TokenKind Lexer_tokFloatingPoint(Lexer *lX, Token *tok, const Position *start);

static u32 Lexer_tokUniversalChar(Lexer *lX, u32 len);
static u32 Lexer_tokEscapedChar(Lexer *lX, bool inStr);
static u32_u32_pair Lexer_readRune(Lexer *lX, Range *range);
static void Lexer_toUtf16(Lexer *lX, Stream *os,const Range* range);
static void Lexer_toUtf32(Lexer *lX, Stream* S, const Range* range);

void Lexer_init(Lexer *lX, struct Log_t *log, Source *src)
{
    csAssert0(log != NULL);

    Lexer_initKeywordsMap();

    memset(lX, 0, sizeof(*lX));
    lX->L = log;
    lX->conv = Allocator_alloc(ArenaAllocator, CONVERSION_BUFFER_SIZE);
    csAssert0(lX->conv != NULL);

    Lexer_reset(lX, src);
}

void Lexer_reset(Lexer *lX, Source *src)
{
    csAssert0(src != NULL);

    lX->next.kind = tokCOUNT;
    lX->idx = 0;
    lX->src = src;
    lX->flags = 0;
    memset(&lX->pos, 0, sizeof(lX->pos));
}

TokenKind Lexer_next(Lexer *lX, Token *out)
{
    u32 max;
    const char *src;
    TokenKind kind;

    csAssert0(lX != NULL);
    csAssert0(out != NULL);

    memset(out, 0, sizeof(*out));
    Lexer_updateToken(lX, out, tokCOUNT, lX->idx, lX->idx, lX->pos);
    if (lX->next.kind != tokCOUNT)
        return Lexer_resetNext(lX, out);

    max = Source_len(lX->src);
    src = Source_src(lX->src);

    /* Skips whitespace a catches the first instance of a new line token */

    while (lX->idx < max && isspace(src[lX->idx])) {
        Lexer_updatePos(lX, src[lX->idx]);
        if (out->kind == tokCOUNT && src[lX->idx] == '\n')
            Lexer_updateToken(lX, out, tokNl, lX->idx, lX->idx, lX->pos);
        ++lX->idx;
    }

    if (out->kind != tokCOUNT)
        return out->kind;

    // Skip lexer errors and parse next token
    while ((kind = Lexer_tokenize(lX, out)) == tokCOUNT)
        return Lexer_next(lX, out);

    return kind;
}

TokenKind Lexer_tokenize(Lexer *lX, Token *out) {
    char c = Lexer_peek(lX, 0);
    char cc = Lexer_peek(lX, 1);
    char ccc = Lexer_peek(lX, 2);

    Position pos = Lexer_mark(lX);

#define IF(C, T) if (cc == (C)) \
    return Lexer_updateToken(lX, out, (T), pos.idx, Lexer_advance(lX, 2), pos.coord);

#define IF_ELSE(C, T, E) \
    if (cc == (C)) return Lexer_updateToken(lX, out, (T), pos.idx, Lexer_advance(lX, 2), pos.coord); \
    else return Lexer_updateToken(lX, out, (E), pos.idx, Lexer_advance(lX, 1), pos.coord)

#define IF_ELSE2(CC, CCC, T, E, EE)                                                             \
    if (cc == (CC)) {                                                                           \
        if (ccc == (CCC)) {                                                                     \
            Lexer_advance(lX, 3);                                                               \
            return Lexer_updateToken(lX, out, (T), pos.idx, Lexer_advance(lX, 1), pos.coord);   \
        }                                                                                       \
        else {                                                                                  \
            return Lexer_updateToken(lX, out, (E), pos.idx, Lexer_advance(lX, 2), pos.coord);   \
        }                                                                                       \
    }                                                                                           \
    else                                                                                        \
        return Lexer_updateToken(lX, out, (EE), pos.idx, Lexer_advance(lX, 1), pos.coord)

#define ADD(T) return Lexer_updateToken(lX, out, (T), pos.idx, Lexer_advance(lX, 1), pos.coord)

    switch(c) {
        case '%':
            IF_ELSE('=', tokModAssign, tokMod);
        case '/':
            IF('=', tokDivAssign)
            else if (cc == '*' || cc == '/')
                return Lexer_tokComment(lX, out);
            else
                ADD(tokDiv);
        case '*':
            IF('*', tokExponent)
            else IF_ELSE('=', tokMultAssign, tokMult);
        case '+':
            IF('+', tokPlusPlus)
            else IF_ELSE('=', tokPlusAssign, tokPlus);
        case '-':
            IF('-', tokMinusMinus)
            else IF('>', tokLArrow)
            else IF_ELSE('=', tokMinisAssign, tokMinus);
        case '|':
            IF('|', tokLOr)
            else IF_ELSE('=', tokBitOrAssign, tokBitOr);
        case '&':
            IF('&', tokLAnd)
            else IF_ELSE('=', tokBitAndAssign, tokBitAnd);
        case '^':
            IF_ELSE('=', tokBitXorAssign, tokBitOr);
        case '~':
            IF_ELSE('=', tokCompAssign, tokComplement);
        case '>':
        IF_ELSE2('>', '=', tokSarAssign, tokSar, tokGt);
        case '<':
            IF ('-', tokLArrow)
            else IF_ELSE2('<', '=', tokSalAssign, tokSal, tokLt);
        case '=':
            IF_ELSE('=', tokEqual, tokAssign);
        case '.':
            if (isdigit(cc))
                // there are instance floating point numbers will be typed starting with .
                return Lexer_tokNumber(lX, out);
            else IF_ELSE2('.', '.', tokElipsis, tokDotDot, tokDot);
        case ':':
            IF_ELSE(':', tokDColon, tokColon);
        case '!':
            ADD(tokNot);
        case '{':
            ADD(tokLBrace);
        case '}':
            if (lX->flags & lxfInStrExpr) {
                Lexer_advance(lX, 1);
                return Lexer_tokString(lX, out, pos);
            }
            else {
                ADD(tokRBrace);
            }
        case '[':
            ADD(tokLBracket);
        case ']':
            ADD(tokRBracket);
        case '(':
            ADD(tokLParen);
        case ')':
            ADD(tokRParen);
        case '@':
            ADD(tokAt);
        case '#':
            ADD(tokHash);
        case '?':
            ADD(tokQuestion);
        case '`':
            ADD(tokBackquote);
        case ';':
            ADD(tokSemicolon);
        case ',':
            ADD(tokComma);
        case '$':
            ADD(tokDollar);
        case '\\':
            ADD(tokBackSlash);
        case '\'':
            return Lexer_tokChar(lX, out);
        case '"':
            Lexer_advance(lX, 1);
            return Lexer_tokString(lX, out, pos);
        case 'f':
            if (cc == '"') {
                lX->flags |= lxfInStrExpr;
                return Lexer_updateToken(lX, out, tokLStrExpr, pos.idx, Lexer_advance(lX, 1), pos.coord);
            }
            else
                return Lexer_tokIdent(lX, out);
        case '0' ... '9':
            return Lexer_tokNumber(lX, out);
        case '_':
        case 'a' ... 'e':
        case 'g' ... 'z':
        case 'A' ... 'Z':
            return Lexer_tokIdent(lX, out);
        case EOF:
            return tokEoF;
        default:
            Log_error(lX->L,
                      mkRange(pos.idx, Lexer_advance(lX, 1), pos.coord, lX->src),
                      "unexpected character in source '%c'", c);
            return tokCOUNT;
    }
}

u32 Lexer_tokUniversalChar(Lexer *lX, u32 len)
{
    u32 r = 0;
    u32 start = lX->idx - 1;

    for (u32 i = 0u; i < len; i++) {
        char c = Lexer_peek(lX, 1);
        switch (c) {
            case '0' ... '9': r  = (r << 4) | (c - '0'); break;
            case 'a' ... 'f': r  = (r << 4) | (c - 'a' + 10); break;
            case 'A' ... 'F': r  = (r << 4) | (c - 'A' + 10); break;
            default: {
                Log_error(lX->L,
                          mkRange(start, Lexer_advance(lX, 1), lX->pos, lX->src),
                          "invalid character character: %c", c);
                Lexer_abort(lX);;
            }
        }
        Lexer_advance(lX, 1);
    }

    if (!isucn(r)) {
        Log_error(lX->L,
                  mkRange(start, lX->idx, lX->pos, lX->src),
                  "invalid character character");
        Lexer_abort(lX);;
    }

    return r;
}

u32 Lexer_tokOctalChar(Lexer *lX, char c)
{
    int r = c - '0';

    c = Lexer_peek(lX, 0);
    if (!isoct(c))
        return r;

    Lexer_advance(lX, 1);
    r = (r << 3) | (c - '0');

    c = Lexer_peek(lX, 0);
    if (!isoct(c))
        return r;

    Lexer_advance(lX, 1);
    return (r << 3) | (c - '0');
}

u32 Lexer_tokHexChar(Lexer *lX)
{
    char c = Lexer_peek(lX, 0);
    if (!isxdigit(c)) {
        Log_error(lX->L, mkRange(lX->idx-1, lX->idx, lX->pos, lX->src),
                  "\\%c is not followed by a hexadecimal literal", c);
        Lexer_abort(lX);;
    }

    uint32_t r = 0;
    for (; c != EOF; c = Lexer_peek(lX, 0)) {
        switch(c) {
            case '0' ... '9': r  = (r << 4) | (c - '0'); break;
            case 'a' ... 'f': r  = (r << 4) | (c - 'a' + 10); break;
            case 'A' ... 'F': r  = (r << 4) | (c - 'A' + 10); break;
            default: return r;
        }
        Lexer_advance(lX, 1);
    }

    return r;
}

char Lexer_unescapeChar(char c)
{
    switch (c) {
        case '\n': return 'n';
        case '\a': return 'a';
        case '\b': return 'b';
        case '\f': return 'f';
        case '\r': return 'r';
        case '\t': return 't';
        case '\v': return 'v';
        case '\033': return 'e';
        default: return c;
    }
}

u32 Lexer_tokEscapedChar(Lexer *lX, bool inStr)
{
    Position pos = Lexer_mark(lX);
    char c = Lexer_peek(lX, 0);
    Lexer_advance(lX, 1);
    if (c == '\n' && inStr) return '\n';

    switch (c) {
        case '\'': case '"': case '?': case '\\': case '$': return c;
        case 'n': return '\n';
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case 'e': return '\033';
        case 'x': return Lexer_tokHexChar(lX);
        case 'u': return Lexer_tokUniversalChar(lX, 4);
        case 'U': return Lexer_tokUniversalChar(lX, 8);
        case '0' ... '7': return Lexer_tokOctalChar(lX, c);
        default:
            Log_warn(lX->L,
                     mkRange(pos.idx, pos.idx, pos.coord, lX->src),
                     "unknown escape character: \\%c", Lexer_unescapeChar(c));
            return c;
    }
}

u32_u32_pair Lexer_readRune(Lexer *lX, Range *range)
{
    StringView sv = Range_view(range);
    const char *s = sv.data;
    u32 len = Lexer_clz(s[0]);

    if (len == 0)
        return make(u32_u32_pair, 1, s[0]);

    if (sv.count > len) {
        Log_error(lX->L, range, "invalid UTF-8 character sequence");
        Lexer_abort(lX);;
    }

    for (u32 i = 1u; i < len; i++) {
        if ((s[i] & 0xC0) != 0x80) {
            Range r0 = Range_sub0(range, i, 1);
            Log_error(lX->L, &r0, "invalid UTF-8 continuation byte");
            Lexer_abort(lX);;
        }
    }

    switch (len) {
        case 2:
            return (u32_u32_pair){2, ((s[0] & 0x1F) << 6) | (s[1] & 0x3F)};
        case 3:
            return (u32_u32_pair){3, ((s[0] & 0xF) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F)};
        case 4:
            return (u32_u32_pair){4, ((s[0] & 0x7) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F)};
        default:
            Log_error(lX->L, range, "invalid UTF-8 sequence");
            Lexer_abort(lX);;
    }

    unreachable("");
}

void Lexer_toUtf16(Lexer *lX, Stream *os, const Range* range)
{
    u32 i = 0u;
    u32 sz = Range_size(range);
    while (i < sz) {
        Range r0 = Range_sub0(range, i, 0);
        u32_u32_pair rune = Lexer_readRune(lX, &r0);
        if (rune.s < 0x10000) {
            Stream_bWrite16(os, (u16)(rune.s));
        }
        else {
            Stream_bWrite16(os, (u16)((rune.s >> 10) + 0xD7C0));
            Stream_bWrite16(os, (u16)((rune.s & 0x3FF) + 0xDC00));
        }
        i++;
    }
}

void Lexer_toUtf32(Lexer *lX, Stream* S, const Range* range)
{
    u32 i = 0u;
    u32 sz = Range_size(range);
    while (i < sz) {
        Range r0 = Range_sub0(range, i, 0);
        u32 rune = Lexer_readRune(lX, &r0).s;
        Stream_bWrite32(S, rune);
        i++;
    }
}

TokenKind Lexer_tokChar(Lexer *lX, Token *tok)
{
    Position pos = Lexer_mark(lX);
    char c = Lexer_peek(lX, 1);
    u32 chr = (uint8_t)c & 0xFF;

    Lexer_advance(lX, 2);

    if (c == '\\') {
        chr = Lexer_tokEscapedChar(lX, false);
    }

    if (chr >= 0x80) {
        while (c != '\'') {
            Lexer_advance(lX, 1);
            c = Lexer_peek(lX, 0);
        }

        chr  = Lexer_readRune(lX, mkRange(pos.idx+1, lX->idx, pos.coord, lX->src)).s;
    }

    if (Lexer_peek(lX, 0) != '\'') {
        // unterminated character literal
        Lexer_eatUntilAnyOf(lX, '\n', '\'');
        if (Lexer_peek(lX, 0) == '\'') {
            Lexer_advance(lX, 1);
            Log_error(lX->L,
                      mkRange(pos.idx, lX->idx, pos.coord, lX->src),
                      "character literal has more than 1 character");
        }
        else {
            Log_error(lX->L,
                      mkRange(pos.idx, lX->idx, pos.coord, lX->src),
                      "unterminated character sequence");
        }

        return tokCOUNT;
    }
    else {
        Token_set(tok, Char, chr);
        return Lexer_updateToken(lX, tok, tokChar, pos.idx, Lexer_advance(lX, 1), pos.coord);
    }
}

TokenKind Lexer_tokString(Lexer *lX, Token *tok, Position pos)
{
    Buffer buffer;
    Stream os;

    Buffer_init1With(&buffer, PoolAllocator, 64);
    os = StringStream_attach(&buffer);

#define IsInStrExpr() (lX->flags & lxfInStrExpr)

    char c = Lexer_peek(lX, 0);
    bool inStrExpr = IsInStrExpr();

    for (;c != EOF; c = Lexer_peek(lX, 0)) {
        char cc;
        u32 es;
        u32 chr;
        if (c == '"') {
            lX->flags &= ~lxfInStrExpr;
            break;
        }

        if (c == '\n') {
            break;
        }

        Lexer_advance(lX, 1);

        cc = Lexer_peek(lX, 0);
        if (IsInStrExpr() && c == '$' && cc == '{') {
            break;
        }

        if (c != '\\') {
            Stream_putc(&os, c);
            continue;
        }

        bool ucn = (cc == 'u' || cc == 'U');
        es = lX->idx;
        chr = Lexer_tokEscapedChar(lX, true);
        if (ucn) {
            if (!Stream_putUtf8(&os, chr)) {
                Log_error(lX->L, mkRange(es, lX->idx, lX->pos, lX->src),
                          "invalid UCS character: \\U%x", chr);
            }
        }
        else {
            Stream_putc(&os, (char)chr);
        }
    }

    if (!IsInStrExpr() && c != '"') {
        Lexer_eatUntilAnyOf(lX, '"', '\n');
        Log_error(lX->L, mkRange(pos.idx, lX->idx, pos.coord, lX->src), "unterminated string literal");
        if (Lexer_peek(lX, 0) == '"')
            Lexer_advance(lX, 1);

        StringStream_detach(&os);
        Vector_deinit(&buffer);

        return tokCOUNT;
    }
    else {
        TokenKind kind = tokCOUNT;
        Lexer_advance(lX, 1);
        if (!inStrExpr || (lX->idx - pos.idx) > 1) {
            Token_set(tok, String, Buffer_release_(&buffer, false));
            kind = Lexer_updateToken(lX, tok, tokString, pos.idx, lX->idx, pos.coord);
        }

        StringStream_detach(&os);
        Vector_deinit(&buffer);

        if (inStrExpr && c == '"') {
            if (kind == tokCOUNT)
                return Lexer_updateToken(lX, tok, tokRStrExpr, pos.idx, lX->idx, pos.coord);

            Lexer_updateToken(lX, &lX->next, tokRStrExpr, pos.idx, lX->idx, pos.coord);
        }

        return kind;
    }
}

TokenKind Lexer_tokBinaryNumber(Lexer *lX, Token *tok)
{
    char c;
    Position pos = Lexer_mark(lX);

    Lexer_advance(lX, 2);
    c = Lexer_peek(lX, 0);

    for (; (c == '0' || c == '1'); c = Lexer_peek(lX, 0))
        Lexer_advance(lX, 1);

    if (isdigit(c)) {
        Position p = Lexer_mark(lX);
        Lexer_eatWhile(lX, isdigit);
        Log_error(lX->L,
                  mkRange(p.idx, lX->idx, p.coord, lX->src),
                  "invalid digit in a binary number '%c'", c);

        return tokCOUNT;
    }
    return Lexer_parseInteger(lX, tok, &pos, 2);
}

TokenKind Lexer_tokHexNumber(Lexer *lX, Token *tok)
{
    char c;
    Position pos = Lexer_mark(lX);

    Lexer_advance(lX, 2);
    c = Lexer_peek(lX, 0);

    for (; isxdigit(c); c = Lexer_peek(lX, 0))
        Lexer_advance(lX, 1);

    if (toupper(c) == 'P') {
        return Lexer_tokFloatingPoint(lX, tok, &pos);
    }
    else {
        return Lexer_parseInteger(lX, tok, &pos, 16);
    }
}

TokenKind Lexer_tokOctalNumber(Lexer *lX, Token *tok)
{
    char c, x;
    Position pos = Lexer_mark(lX);
    
    c = Lexer_peek(lX, 0);
    for (; isoct(c); c = Lexer_peek(lX, 0))
        Lexer_advance(lX, 1);

    x = Lexer_peek(lX, 0);
    if (isdigit(x)) {
        // could be a floating point number, try to jump to a '.' or an 'E'
        for (; isdigit(c); c = Lexer_peek(lX, 0))
            Lexer_advance(lX, 1);

        if (c == '.' || c == 'e' || c == 'E')
            return Lexer_tokFloatingPoint(lX, tok, &pos);

        Log_error(lX->L, mkRange(pos.idx, lX->idx, pos.coord, lX->src),
                         "'%c' is not a valid octal digit", x);
        return tokCOUNT;
    }

    return Lexer_parseInteger(lX, tok, &pos, 8);
}

TokenKind Lexer_tokDecimalNumber(Lexer *lX, Token *tok)
{
    char c;
    int C;
    Position pos = Lexer_mark(lX);

    c = Lexer_peek(lX, 0);
    for (; isdigit(c); c = Lexer_peek(lX, 0))
        Lexer_advance(lX, 1);

    C = toupper(c);
    if (C == 'E' || c == '.')
        // this is possibly a floating point number
        return Lexer_tokFloatingPoint(lX, tok, &pos);

    return Lexer_parseInteger(lX, tok, &pos, 10);
}

TokenKind Lexer_parseInteger(Lexer *lX, Token *tok, const Position *start, int base)
{
    char *end;
    u64 value;

    u32 idx = (base == 16 || base == 2)? start->idx + 2 : start->idx;
    u32 len = lX->idx - idx;

    csAssert0(len <= MAX_DIGITS_IN_INTEGER, "integer number too large to be parsed");
    strncpy(lX->conv, Source_at(lX->src, idx), len);
    lX->conv[len] = '\0';

    value = strtoull(lX->conv, &end, base);
    if (errno == ERANGE || *end != '\0') {
        Log_error(lX->L,
                  mkRange(start->idx, lX->idx, start->coord, lX->src),
                  "parsing integer failed: %s",
                  (errno == ERANGE)? strerror(errno): " invalid number");

        return tokCOUNT;
    }

    Token_set(tok, Int, value);
    return Lexer_updateToken(lX, tok, tokInteger, start->idx, lX->idx, start->coord);
}

TokenKind Lexer_tokFloatingPoint(Lexer *lX, Token *tok, const Position *start)
{
    char c, *end;
    u32 idx, len;
    double value;

    c = (char)toupper(Lexer_peek(lX, 0));

    csAssert0(c == '.' || c == 'E' || c == 'P');
    Lexer_advance(lX, 1);

    if (c == 'E' || c == 'P') {
        c = Lexer_peek(lX, 0);

        if (c == '-' || c == '+')
            Lexer_advance(lX, 1);

        if (!isdigit(Lexer_peek(lX, 0))) {
            Log_error(lX->L, mkRange(start->idx, lX->idx, start->coord, lX->src),
                             "'%c' exponent has no digits", c);
            return tokCOUNT;
        }
    }

    Lexer_eatWhile(lX, isdigit);

    idx = start->idx;
    len = lX->idx - idx;
    strncpy(lX->conv, Source_at(lX->src, idx), len);
    lX->conv[len] = 0;


    value = strtod(lX->conv, &end);
    if (errno == ERANGE || *end != '\0') {
        Log_error(lX->L,
                  mkRange(start->idx, lX->idx, start->coord, lX->src),
                  "parsing floating number failed: %s",
                  (errno == ERANGE)? strerror(errno): " invalid number");
        return tokCOUNT;
    }

    Token_set(tok, Float, value);
    return Lexer_updateToken(lX, tok, tokFloat, start->idx, lX->idx, start->coord);
}

TokenKind Lexer_tokNumber(Lexer *lX, Token *tok)
{
    char c, cc, ccc;

    c = Lexer_peek(lX, 0);
    cc = (char)toupper(Lexer_peek(lX, 1));
    ccc = Lexer_peek(lX, 2);

    if (c == '0') {
        if (cc == 'X' && isxdigit(ccc)) {
            return Lexer_tokHexNumber(lX, tok);
        }
        else if (cc == 'B' && (ccc == '0' || ccc == '1')) {
            return Lexer_tokBinaryNumber(lX, tok);
        }
        else if (cc == '.' || cc == 'E') {
            Position pos = Lexer_mark(lX);
            Lexer_advance(lX, 1);
            return Lexer_tokFloatingPoint(lX, tok, &pos);
        }

        return Lexer_tokOctalNumber(lX, tok);
    }

    return Lexer_tokDecimalNumber(lX, tok);
}

TokenKind Lexer_tokIdent(Lexer *lX, Token *tok)
{
    char c;
    Range range;
    StringView sv;
    TokenKind  *kw;
    Position pos = Lexer_mark(lX);

    c = Lexer_peek(lX, 0);

    // consume all letters that can be an identifier
    for (; (c == '_' || c == '$' || isdigit(c) || isalnum(c)); c = Lexer_peek(lX, 0))
        Lexer_advance(lX, 1);

    Range_update(&range,  lX->src, pos.idx, lX->idx, pos.coord);
    sv = Range_view(&range);

    kw = Map_ref0(&sKeyWordList, sv.data, sv.count);
    if (kw != NULL) {
        // this is a keyword
        if (*kw == tokTrue || *kw == tokFalse)
            Token_set(tok, Bool, *kw == tokTrue);

        return Lexer_updateToken(lX, tok, *kw, pos.idx, lX->idx, pos.coord);
    }

    // this is an identifier
    tok->value.b = false;
    return Lexer_updateToken(lX, tok, tokIdentifier, pos.idx, lX->idx, pos.coord);
}

TokenKind Lexer_tokComment(Lexer *lX, Token *tok)
{
    char c;
    bool isMultiLine;
    i32 level = 1;

    Position pos = Lexer_mark(lX);
    Lexer_advance(lX, 1);

    c = Lexer_peek(lX, 0);
    isMultiLine = c == '*';

    Lexer_advance(lX, 1);

    while (c != EOF) {
        char cc;
        c = Lexer_peek(lX, 0);
        Lexer_advance(lX, 1);

        if (c == '\n' && !isMultiLine)
            break;

        cc = Lexer_peek(lX, 0);
        if (c == '*' && cc == '/') {
            Lexer_advance(lX, 1);
            if (--level == 0)
                break;
        }

        if (c == '/' && cc == '*')
            level++;
    }

    if (isMultiLine && level != 0) {
        Log_error(lX->L, mkRange(pos.idx, lX->idx, pos.coord, lX->src), "unterminated multiline comment");
        return tokCOUNT;
    }

    // the token value will indicate whether the comment is multiline or not
    return Lexer_updateToken(lX, tok, tokComment, pos.idx, lX->idx, pos.coord);
}

