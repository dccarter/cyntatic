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

#include "compiler/heap.h"
#include "compiler/log.h"

#include "e4c.h"
#include "map.h"
#include "tree.h"

enum {
    sytLabel,
    sytVar,
    sytDefine
};

typedef u32 SymbolTag;

typedef Pair(u32, Range) Patch;
typedef Pair(u32, RbTree(u32)) SymbolRefers;
typedef RbTree(SymbolRefers *) SymbolReferences;

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
    RbTree(Patch)  patchWork;
    SymbolReferences references;
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

static inline
int Assembler_symbol_ref_cmp(const void *lhs, u32 len, const void *rhs)
{
    const SymbolRefers *r1 = *((const SymbolRefers **)lhs),
            *r2 = *((SymbolRefers **)rhs);
    return r1->f < r2->f? -1 : (r1->f > r2->f? 1 : 0);
}

static inline
void Assembler_symbol_ref_dctor(void *elem)
{
    SymbolRefers *ref = *((SymbolRefers **)elem);
    RbTree_deinit(&ref->s);
}

void Assembler_symbol_ref_init(AssemblerCtx *as, u32 id)
{
    SymbolRefers tmp = {.f = id };
    FindOrAdd foa = RbTree_find_or_create(&as->references, &tmp);
    if (foa.f) {
        SymbolRefers *ref = Allocator_alloc(PoolAllocator, sizeof(SymbolRefers));
        ref->f = id;
        RbTree_initWith(&ref->s, RbTree_cmp_u32, PoolAllocator);
        RbTree_get0(&as->references, foa.s) = ref;
    }
}

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

#define make(T, ...) ((T){ __VA_ARGS__ })


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

attr(always_inline)
static bool Assembler_find_mode(const Mode *modes, u8 len, Mode mode)
{
    for (u8 i = 0; i < len; i++)
        if (modes[i] == mode) return true;
    return false;
}

static Symbol* Assembler_findSymbol(AssemblerCtx *as, const Range* range)
{
    StringView name = Range_view(range);
    Symbol *it = Map_ref0(&as->symbols, name.data, name.count);

    if (it == NULL)
        Assembler_fail(as, "referenced symbol '%*.s' does not exist", name.count, name.data);

    return it;
}

u32 Assembler_addSymbolReference(AssemblerCtx *as, u32 pos, const Range* range, bool addToPatchWork)
{
    StringView name = Range_view(range);
    Symbol *it = Map_ref0(&as->symbols, name.data, name.count);
    if (it == NULL || it->tag == sytLabel) {
        if (addToPatchWork) {
            RbTree_add(&as->patchWork, make(Patch, .f = pos, .s = *range));
            return 0;
        }
        Assembler_fail(as, "referenced symbol '%*.s' must be defined before use", name.count, name.data);
    }
    else
        return it->id;
}

static Mode Assembler_parseModes_(AssemblerCtx *as, const Mode *modes, u8 len)
{
    Token tok = *Assembler_consume(as, tokIdentifier, "expecting either a 'b'/'s'/'w'/'q'");
    StringView ext = Range_view(&tok.range);

    Mode mode = szByte;
    if (ext.count == 1) {
        switch (ext.data[0]) {
            case 'b' :
                break;
            case 's' :
                mode = szShort;
                break;
            case 'w' :
                mode = szWord;
                break;
            case 'q' :
                mode = szQuad;
                break;
            default:
                goto invalidExtension;
        }
    }
    else {
        invalidExtension:
        Assembler_fail0(as, &tok.range,
                        "unsupported instruction '%*.s', use b/s/w/q",
                        ext.count, ext.data);
    }

    if (len > 0 && Assembler_find_mode(modes, len, mode)) {
#define AppendMode(B, M) Buffer_appendCstr((B), vmModeNamesTbl[(M)])
        __destroy char *supported = join(modes, len, "/", AppendMode);
#undef  AppendMode

        Assembler_fail(
                as, "mode '%s' not supported in current context, use %s",
                vmModeNamesTbl[mode], supported);
    }

    return mode;
}

#define Assembler_parseModes__(AS, modes) Assembler_parseModes_((AS), (modes), sizeof__(modes))
#define Assembler_parseModes(AS, ...) Assembler_parseModes__((AS), (Mode[]){ __VA_ARGS__ })


static void Assembler_parseLabel(AssemblerCtx *as)
{
    Symbol *sym;
    StringView sv;
    i32 id;
    Token tok = *Assembler_consume(as, tokIdentifier, "expecting a label name");

    Assembler_consume(as, tokColon, "expecting a colon ':' to terminate a label");

    sv = Range_view(&tok.range);
    sym = Map_ref0(&as->symbols, sv.data, sv.count);
    if (sym != NULL) {
        Assembler_fail0(as, &tok.range, "label '%*.s' already defined", sv.count, sv.data);
        unreachable();
    }

    id = Vector_len(&as->instructions);
    Map_set0(&as->symbols, sv.data, sv.count, mkSymbol(id = id, .tag = sytLabel));
    Assembler_symbol_ref_init(as, id);
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
    RbTree_initWith(&ctx->references, Assembler_symbol_ref_cmp, PoolAllocator);

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