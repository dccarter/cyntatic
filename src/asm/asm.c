/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#include "asm/asm.h"

#include <compiler/heap.h>
#include <compiler/log.h>

#include "e4c.h"
#include "map.h"

enum {
    sytLabel,
    sytVar,
    sytDefine
};

typedef u32 SymbolTag;

typedef Pair(StringView, Range) Patch;
typedef Vector(u32) SymbolRef;
typedef Vector(SymbolRef) SymbolsRefs;

typedef struct {
    u64 id;
    u32 size;
    SymbolTag tag;
    Range range;
} Symbol;

struct AssemblerCtx  {
    struct Log_t *L;
    Lexer *lX;
    u32 main;
    Token curr;
    Token prev;
    Token peek;
    Vector(u8) constants;
    Vector(Instruction) instructions;
    Map(Symbol) symbols;
    Map(Patch)  patchWork;
};

#define Assembler_EoT(as)   ((as)->curr.kind == tokEoF)

static bool Assembler_check_(AssemblerCtx *as, const TokenKind *kinds, u32 count)
{
    if (Assembler_EoT(as)) return false;

    for (int i = 0; i < count; i++) {
        if (kinds[i] == as->curr.kind) return true;
    }
    return false;
}

#define Assembler_check__(AS, KINDS) Assembler_check_((AS), (KINDS), sizeof__(KINDS))
#define Assembler_check(AS, KIND, ...)                              \
    Assembler_check__((AS), (TokenKind[]){(KIND), ##__VA_ARGS__})

static Token* Assembler_advance(AssemblerCtx *as)
{
    if (as->curr.kind == tokEoF)
        return &as->curr;

    as->prev = as->curr;
    if (as->peek.kind != tokCOUNT) {
        as->curr = as->peek;
        as->peek.kind = tokCOUNT;
    }
    else
        Lexer_next(as->lX, &as->curr);

    return &as->prev;
}

static Token* Assembler_peek(AssemblerCtx *as)
{
    if (as->curr.kind == tokEoF)
        return &as->curr;

    if (as->peek.kind == tokCOUNT)
        Lexer_next(as->lX, &as->peek);

    return &as->peek;
}

#define Assembler_prev(AS) (&(AS)->prev)
#define Assembler_curr(AS) (&(AS)->curr)

#define mkSymbol(...) ((Symbol){ __VA_ARGS__ })

attr(always_inline)
static void Assembler_synchronize(AssemblerCtx *as)
{
    do {
        Assembler_advance(as);
        if (Assembler_prev(as)->kind == tokNl) break;
    } while (!Assembler_EoT(as));
}

static bool Assembler_match_(AssemblerCtx *as, const TokenKind *kinds, u32 count)
{
    if (Assembler_check_(as, kinds, count)) {
        Assembler_advance(as);
        return true;
    }
    return false;
}

E4C_DEFINE_EXCEPTION(ParserException, "Parsing error", RuntimeException);

#define Assembler_match__ (AS, KINDS) Assembler_match_((AS), (KINDS), sizeof__(KINDS))
#define Assembler_match(AS, KIND, ...)                              \
    Assembler_match__((AS), &(TokenKind){(KIND), ##__VA_ARGS__})

#define Assembler_error0(AS, RNG, ...) Log_error((AS)->L, (RNG), ##__VA_ARGS__)
#define Assembler_error(AS, ...) Log_error((AS)->L, &Assembler_curr(AS)->range, ##__VA_ARGS__)

#define Assembler_fail(AS, ...)                                                 \
    ({ Assembler_error((AS), ##__VA_ARGS__); E4C_THROW(ParserException, ""); })

#define Assembler_fail0(AS, RNG, ...)                                                 \
    ({ Assembler_error0((AS), (RNG), ##__VA_ARGS__); E4C_THROW(ParserException, ""); })

#define Assembler_consume(AS, KIND, ...)  ({ \
        Token *LineVAR(iAs) = NULL;                      \
        do {                                             \
                                                         \
            if (Assembler_check((AS), (KIND)))           \
                LineVAR(iAs) = Assembler_advance((AS));  \
            else                                         \
                Assembler_fail((AS), ##__VA_ARGS__);     \
        } while(0);                                      \
        LineVAR(iAs);                                   \
    })


static void Assembler_parseLabel(AssemblerCtx *as)
{
    Symbol *sym;
    StringView sv;
    i32 id;
    Token tok = *Assembler_consume(as, tokIdentifier, "expecting a label name");

    Assembler_consume(as, tokColon, "expecting a colon ':' a terminate a label");

    sv = Range_view(&tok.range);
    sym = Map_ref0(&as->symbols, sv.data, sv.count);
    if (sym != NULL) {
        Assembler_fail0(as, &tok.range, "label '%*.s' already defined", sv.count, sv.data);
        unreachable();
    }

    id = Vector_len(&as->instructions);
    Map_set0(&as->symbols, sv.data, sv.count, mkSymbol(id = id, .tag = sytLabel));
    if (strncmp("main", sv.data, sv.count) == 0) as->main = id;
}

static void Assembler_parseInstruction(AssemblerCtx *as)
{

}

static void Assembler_parseVarDecl(AssemblerCtx *as)
{

}

void Assembler_init(Assembler *as, Lexer *lX)
{
    AssemblerCtx *ctx;
    csAssert0(as->ctx == NULL);
    // allocate context for assembler
    ctx = Allocator_cAlloc(PoolAllocator, sizeof(AssemblerCtx), 1);
    csAssert0(ctx != NULL);

    ctx->L = lX->L;
    ctx->lX = lX;
    ctx->main = 0;
    ctx->peek.kind = tokCOUNT;

    Map_init2(&ctx->symbols, DefaultAllocator, PoolAllocator, 128);
    Map_init2(&ctx->patchWork, DefaultAllocator, PoolAllocator, 128);

    Vector_init0With(&ctx->instructions, DefaultAllocator, 1024);
    Vector_init0With(&ctx->constants,    DefaultAllocator, 1024);
}

void Assembler_deinit(Assembler *as)
{
    AssemblerCtx *ctx = as->ctx;

    as->ctx = NULL;
    if (ctx == NULL) return;

    Map_deinit(&ctx->symbols);
    Map_deinit(&ctx->patchWork);
    Vector_deinit(&ctx->instructions);
    Vector_deinit(&ctx->constants);
    Allocator_dealloc(ctx);
}

u32  Assembler_assemble(Assembler *as, Code *into)
{
    AssemblerCtx *ctx = as->ctx;

    csAssert0(into != NULL);
    csAssert0(ctx   != NULL);

    while (!Assembler_EoT(ctx)) {
        E4C_TRY {
            switch (Assembler_curr(ctx)->kind) {
                case tokIdentifier:
                    if (Assembler_peek(ctx)->kind == tokColon)
                        Assembler_parseLabel(ctx);
                    else
                        Assembler_parseInstruction(ctx);
                    break;
                case tokDollar:
                    Assembler_parseVarDecl(ctx);
                    break;
                case tokNl: case tokComment:
                    Assembler_advance(ctx);
                    break;
                default:
                    Assembler_fail(ctx, "unsupported token");
                    break;
            }
        }
        E4C_CATCH(ParserException) {
            Assembler_synchronize(ctx);
        }
    }
}