/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#include "compiler/asm/asm.h"

#include "compiler/common/ident.h"
#include "compiler/common/heap.h"
#include "compiler/common/log.h"
#include "compiler/common/itp.h"

#include "vm/builtins.h"

#include "e4c.h"
#include "tree.h"

#include <unistd.h>

enum {
    sytLabel,
    sytVar,
    sytDefine
};

typedef u32 SymbolTag;

typedef Pair(u32, Range) Patch;
typedef Pair(u32, RbTree(u32)) SymbolRefers;
typedef RbTree(SymbolRefers *) SymbolReferences;
typedef Pair(u32, Vector(u32)) RefList_t;

typedef struct {
    u64 id;
    u32 size;
    SymbolTag tag;
    Ident name;
    Range range;
} Symbol;

struct AssemblerCtx  {
    ITokenParser itp;
    struct Log_t *L;
    u32 main;
    Token curr;
    Token prev;
    Token peek;
    Vector(u8) constants;
    Vector(Instruction) instructions;
    RbTree(Symbol) symbols;
    RbTree(Patch)  patchWork;
    SymbolReferences references;
};

static inline
int Assembler_symbol_ref_cmp(const void *lhs, u32 len, const void *rhs)
{
    const SymbolRefers *aa = *((const SymbolRefers **)lhs),
            *bb = *((SymbolRefers **)rhs);
    return RbTree_cmp_u32(&aa->f, len, &bb->f);
}

static inline
int Assembler_ref_list_cmp(const void *lhs, u32 len, const void *rhs)
{
    const RefList_t *aa = (const RefList_t *)lhs,
            *bb = (RefList_t *)rhs;
    return RbTree_cmp_u32(&aa->f, len, &bb->f);
}

static inline
int Assembler_patch_cmp(const void *lhs, u32 len, const void *rhs)
{
    const Patch *aa = (const Patch *)lhs, *bb = (const Patch *)rhs;
    return RbTree_cmp_u32(&aa->f, len, &bb->f);
}

static inline
int Assembler_symbol_cmp(const void *lhs, u32 len, const void *rhs)
{
    const Symbol *aa = (const Symbol *)lhs, *bb = (Symbol *)rhs;
    if (Ident_equals(&aa->name, &bb->name)) return 0;

    return RbTree_cmp_string(&aa->name.name, len, &bb->name.name);
}

static inline
void Assembler_symbol_ref_dctor(void *elem)
{
    SymbolRefers *ref = *((SymbolRefers **)elem);
    RbTree_deinit(&ref->s);
    Allocator_dealloc(ref);
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

attr(always_inline)
static void Assembler_synchronize(AssemblerCtx *as)
{
    do {
        ITP_advance(as);
        if (ITP_prev(as)->kind == tokNl) break;
    } while (!ITP_eof(as));
}

attr(always_inline)
static bool Assembler_find_mode(const Mode *modes, u8 len, Mode mode)
{
    for (u8 i = 0; i < len; i++)
        if (modes[i] == mode) return true;
    return false;
}

static Symbol* Assembler_find_symbol(AssemblerCtx *as, const Range* range)
{
    StringView name = Range_view(range);
    Symbol q = {.name = Ident_foa(name.data, name.count)};
    RbTreeNode *it = RbTree_find_(&as->symbols.base, &q, name.count);

    if (it == NULL)
        ITP_fail(as, "referenced symbol '%.*s' does not exist", name.count, name.data);

    return RbTree_ref0(&as->symbols, it);
}

static Symbol *Assembler_add_symbol(AssemblerCtx *as, u32 pos, SymbolTag tag, const Range *range)
{
    StringView sv = Range_view(range);
    Symbol q = { .name = Ident_foa(sv.data, sv.count)};
    FindOrAdd foa = RbTree_find_or_add_(&as->symbols.base, &q, 0);
    if (!foa.f) {
        ITP_fail0(as, range, "symbol with name '%s' already defined", q.name.name);
    }
    RbTree_get0(&as->symbols, foa.s) = make(Symbol,
                                            .name = q.name,
                                            .size = 0,
                                            .id = pos,
                                            .tag = tag,
                                            .range = *range);

    return RbTree_ref0(&as->symbols, foa.s);
}

u32 Assembler_add_symbol_reference(AssemblerCtx *as, u32 pos, const Range* range, bool addToPatchWork)
{
    StringView name = Range_view(range);
    Symbol q = {.name = Ident_foa(name.data, name.count) };
    RbTreeNode *it = RbTree_find_(&as->symbols.base, &q, 0);
    if (it == NULL || RbTree_ref(&as->symbols, it)->tag == sytLabel) {
        if (addToPatchWork) {
            RbTree_add(&as->patchWork, make(Patch, .f = pos, .s = *range));
            return 0;
        }
        ITP_fail(as, "referenced symbol '%.*s' must be defined before use", name.count, name.data);
    }
    else
        return RbTree_ref(&as->symbols, it)->id;
}

void Assembler_define(AssemblerCtx *as, const char *name, u64 value)
{
    Symbol q = make(Symbol, .name = Ident_foa1(name));
    FindOrAdd foa = RbTree_find_or_add_(&as->symbols.base, &q, 0);
    if (!foa.f) {
        Log_error(as->L, &make(Range, 0), "builtin variable '%s' already defined", name);
        abortCompiler(as->L, "assembler initialization failed");
    }
    RbTree_get0(&as->symbols, foa.s) = make(Symbol,
                                                .name = q.name,
                                                .id = value,
                                                .tag = sytDefine);
}

u32 Assembler_get_variable_size(AssemblerCtx *as, const Range *range)
{
    StringView name = Range_view(range);
    Symbol q = {.name = Ident_foa(name.data, name.count) };
    RbTreeNode *it = RbTree_find_(&as->symbols.base, &q, 0);
    if (it == NULL)
        ITP_fail0(as, range, "reference to undefined variable '%.*s'",
                        name.count, name.data);
    if (RbTree_ref(&as->symbols, it)->tag != sytVar)
        ITP_fail0(as, range,
                        "cannot not read size of non variable symbol '%.*s'",
                        name.count, name.data);

    return RbTree_ref(&as->symbols, it)->size;
}

u32 Assembler_append_integral_data(AssemblerCtx *as, i64 value, Mode mode)
{
    const u8 size = vmSizeTbl[mode];
    const u32 pos = Vector_len(&as->constants);

    Vector_expand(&as->constants, size);
    VM_write(Vector_at(&as->constants, pos), value, mode);

    return size;
}

static Mode Assembler_parse_modes_(AssemblerCtx *as, const Mode *modes, u8 len)
{
    Token tok = *ITP_consume(as, tokIdentifier, "expecting either a 'b'/'s'/'w'/'q'");
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
        ITP_fail0(as, &tok.range,
                        "unsupported instruction '%.*s', use b/s/w/q",
                        ext.count, ext.data);
    }

    if (len > 0 && Assembler_find_mode(modes, len, mode)) {
#define AppendMode(B, M) Buffer_appendCstr((B), vmModeNamesTbl[(M)])
        __destroy char *supported = join(modes, len, "/", AppendMode);
#undef  AppendMode

        ITP_fail(
                as, "mode '%s' not supported in current context, use %s",
                vmModeNamesTbl[mode], supported);
    }

    return mode;
}

#define Assembler_parse_modes__(AS, modes) Assembler_parse_modes_((AS), (modes), sizeof__(modes))
#define Assembler_parse_modes(AS, ...) Assembler_parse_modes__((AS), make(Mode[], __VA_ARGS__))


static void Assembler_parse_label(AssemblerCtx *as)
{
    StringView sv;
    i32 id;
    RbTreeNode *it;
    Symbol q;

    Token tok = *ITP_consume(as, tokIdentifier, "expecting a label name");

    ITP_consume(as, tokColon, "expecting a colon ':' to terminate a label");

    sv = Range_view(&tok.range);
    q.name = Ident_foa(sv.data, sv.count);
    it = RbTree_find_(&as->symbols.base, &q, 0);
    if (it != NULL) {
        ITP_fail0(as, &tok.range, "label '%.*s' already defined", sv.count, sv.data);
    }

    id = Vector_len(&as->instructions);
    Assembler_add_symbol(as, id, sytLabel, &tok.range);
    Assembler_symbol_ref_init(as, id);
    if (strncmp("main", sv.data, sv.count) == 0) as->main = id;
}

typedef Pair(bool, Register) IsMemRegPair;

static IsMemRegPair Assembler_parse_instruction_arg(AssemblerCtx *as, Instruction *instr, bool isRb)
{
    bool isMem, isNeg, isSizeOp;
    Token sign, tok;
    Register reg = r0;
    u32 pos;

    isMem = ITP_match(as, tokLBracket);
    sign  = *ITP_curr(as);
    isNeg = ITP_match(as, tokMinus);
    if (!isNeg) ITP_match(as, tokPlus);

    if (!ITP_check(as, tokInteger, tokChar, tokFloat, tokIdentifier, tokHash))
        ITP_fail(as,
                       "expecting either a number, char literal, register, "
                       "variable or label as instruction argument");


    isSizeOp = ITP_match(as, tokHash);
    pos = Vector_len(&as->instructions);
    tok = *ITP_advance(as);

    if (tok.kind == tokIdentifier) {
        Register rX;
        StringView sv;
        if (sign.kind == tokPlus || sign.kind == tokMinus)
            ITP_fail0(as, &tok.range, "+/- not allowed on variables/labels");

        sv = Range_view(&tok.range);
        if ((rX = Vm_get_register_from_str_(sv.data, sv.count)) == regCOUNT) {
            instr->rmd = amImm;
            instr->iu = (isSizeOp ?
                         Assembler_get_variable_size(as, &tok.range) :
                         Assembler_add_symbol_reference(as, pos, &tok.range, true));
            instr->ims = instr->iu == 0? szWord: VM_integer_size(instr->iu);

            if (instr->iu > 0 && ITP_check(as, tokPlus, tokMinus)) {
                Token *tokP;
                union { i64 i; u64 u; } imm;
                isNeg = ITP_match(as, tokMinus);
                if (!isNeg) ITP_match(as, tokMinus);
                tokP = ITP_consume(as, tokInteger, "expecting an integer literal to add to a variable");
                imm.u = Token_get(tokP, Int);
                instr->ims = VM_integer_size(imm.u);
                if (isNeg) imm.i = -(i64)imm.u;
                instr->iu += imm.i;
            }
        }
        else {
            if (isSizeOp) {
                Range ext;
                Range_extend(&tok.range, &ITP_curr(as)->range, &ext);
                ITP_fail0(as, &ext, "'#' operator cannot be applied to register arguments");
            }
            instr->rmd = amReg;
            reg = rX;
        }

        if (isMem && isRb && ITP_match(as, tokComma)) {
            Token *tokP;
            sign = *ITP_curr(as);
            isNeg = ITP_match(as, tokMinus);
            if (!isNeg) ITP_match(as, tokPlus);

            tokP = ITP_advance(as);
            switch (tokP->kind) {
                case tokInteger: {
                    union { i64 i; u64 u; } imm = { .u = Token_get(tokP, Int) };
                    instr->ims = VM_integer_size(imm.u);
                    if (isNeg) imm.i = -(i64)imm.u;
                    instr->iu += imm.u;
                    break;
                }
                case tokHash:
                    isSizeOp = true;
                    if (!ITP_check(as, tokIdentifier))
                        ITP_fail(as, "'#' operator must be followed by an identifier");

                    attr(fallthrough);
                case tokIdentifier:
                    if (sign.kind == tokPlus || sign.kind == tokMinus)
                        ITP_fail0(as, &tok.range, "+/- not allowed on variables");
                    instr->ims = szQuad;
                    instr->ii += (isSizeOp ?
                                  Assembler_get_variable_size(as, &tokP->range) :
                                  Assembler_add_symbol_reference(as, pos, &tokP->range, false));
                    break;

                default:
                    ITP_error0(as, &tok.range, "unexpected effective address, only integers, "
                                                     "variables and '#' operations allowed");
            }

            instr->iea  = 1;
        }
    }
    else {
        instr->rmd = amImm;
        switch (tok.kind) {
            case tokChar:
                instr->iu = Token_get(&tok, Char);
                instr->ims = SZ_(u32);
                break;
            case tokFloat:
                instr->iu = f2u(Token_get(&tok, Float));
                instr->ims = SZ_(u64);
            case tokInteger: {
                union { i64 i; u64 u; } imm = {.u = Token_get(&tok, Int)};
                instr->ims = VM_integer_size(imm.u);
                if (isNeg) imm.i = -(i64)imm.u;
                instr->iu = imm.u;
                break;
            }
            default:
                unreachable("should not get here");
        }
    }

    if (isMem)
        ITP_consume(as, tokRBracket, "expecting a closing ']' to end a memory instruction argument");

    return make(IsMemRegPair, isMem, reg);
}

static void Assembler_parse_instruction(AssemblerCtx *as)
{
    Instruction instr = {0};
    Token tok = *ITP_curr(as);
    StringView name = Range_view(&tok.range);
    OpCodeInfo opc = VM_get_opcode_for_instr_(name.data, name.count);

    if (opc.f == opcCOUNT) {
        ITP_fail(as,
                       "unsupported instruction '%.*s'",
                       name.count, name.data);
    }
    ITP_advance(as);

    instr.osz = opc.s + 1;
    instr.opc = opc.f;
    instr.imd = szQuad;

    if (ITP_match(as, tokDot))
        instr.imd = Assembler_parse_modes(as);

    if (opc.s >= 1) {
        unpack(isMem, reg, Assembler_parse_instruction_arg(as, &instr, false));
        instr.iam = isMem;
        if (instr.rmd == amImm)
            instr.ra = instr.ims;
        else
            instr.ra = reg;
    }

    if (opc.s == 2) {
        unpack(isMem, reg, Assembler_parse_instruction_arg(as, &instr, true));
        instr.ibm = isMem;
        instr.rb = reg;
    }

    Vector_push(&as->instructions, instr);
}

static void Assembler_parse_var_decl(AssemblerCtx *as)
{
    Token tok;
    u32 pos, size = 0;
    Symbol *sym;

    ITP_advance(as);

    tok = *ITP_consume(as, tokIdentifier, "expecting the name of a variable");
    ITP_consume(as, tokAssign, "expecting assignment operator '='");

    pos = Vector_len(&as->constants) + sizeof(CodeHeader);
    sym = Assembler_add_symbol(as, pos, sytVar, &tok.range);

    if (ITP_match(as, tokLBrace)) {
        do {
            u32 val;
            Token *tokp = ITP_curr(as);
            if (tokp->kind == tokChar)
                val = Token_get(tokp, Char);
            else if (tokp->kind == tokInteger)
                val = Token_get(tokp, Int);
            else
                ITP_fail(as, "unexpected token in byte array initializer, expecting digits or a character");

            if (val > 0xFFu)
                ITP_error(as, "assembler byte array supports ascii characters or bytes in range 0x00 - 0xFF");

            ITP_advance(as);
            Vector_push(&as->constants, (u8)val);
            size++;
        } while (ITP_match(as, tokComma));
        ITP_consume(as, tokRBrace, "expecting a closing brace to terminate byte sequence");
    }
    else if (ITP_check(as, tokString)) {
        Token *tokP = ITP_advance(as);
        __destroy char* str = Token_get(tokP, String); // safe to destroy string as it won't be needed
        size = strlen(str);
        Vector_pushArr(&as->constants, str, size);
    }
    else {
        bool isNeg;

        Token *tokS = ITP_curr(as);
        isNeg = ITP_match(as, tokMinus);
        if (!isNeg) ITP_match(as, tokPlus);

        switch (ITP_curr(as)->kind) {
            case tokInteger: {
                Mode mode = szQuad;
                Token *tokP = ITP_advance(as);
                i64 value = u2i(Token_get(tokP, Int));
                if (isNeg) value = -value;
                if (ITP_match(as, tokBackquote))
                    mode = Assembler_parse_modes(as);
                size = Assembler_append_integral_data(as, value, mode);
                break;
            }
            case tokChar: {
                Token *tokP;
                if (tokS != ITP_curr(as))
                    // signs not supported on characters
                    ITP_fail0(as, &tokS->range, "unsupported sign '%c' before character", (isNeg? '-' : '+'));
                tokP = ITP_advance(as);
                size = Assembler_append_integral_data(as, u2i(Token_get(tokP, Char)), szByte);
                break;
            }
            case tokFloat: {
                Mode mode = szQuad;
                Token *tokP = ITP_advance(as);
                double value = Token_get(tokP, Float);
                if (isNeg) value = -value;
                if (ITP_match(as, tokBackquote))
                    mode = Assembler_parse_modes(as, szWord, szQuad);
                size = Assembler_append_integral_data(as, f2i(value), mode);
                break;
            }
            case tokLBracket: {
                if (tokS != ITP_curr(as))
                    // signs not supported on characters
                    ITP_fail0(as, &tokS->range, "unsupported sign '%c' before multibyte declarator",
                                    (isNeg? '-' : '+'));

                ITP_advance(as);
                tok = *ITP_consume(as, tokInteger, "expecting an integer to indicate number of bytes");
                size = vmSizeTbl[szByte];
                if (ITP_match(as, tokBackquote))
                    size = vmSizeTbl[Assembler_parse_modes(as)];
                ITP_consume(as, tokRBracket, "expecting a closing ']' to end memory reservation block");
                size *= Token_get(&tok, Int);
                Vector_expand(&as->constants, size);
                break;
            }
            default:
                ITP_fail(as, "unsupported data format for a variable initializer");
        }
    }

    sym->size = size;
}

static u32 Assembler_link(AssemblerCtx *as, Code *code)
{
    u32 db, ip;
    CodeHeader *header;
    RbTree(RefList_t) refs;

    RbTree_initWith(&refs, Assembler_ref_list_cmp, PoolAllocator);

    RbTree_for_each(&as->patchWork, patch) {
        Symbol *sym;
        StringView name = Range_view(&patch->s);
        RbTreeNode *it = RbTree_find_(&as->symbols.base,
                                        &make(Symbol, .name = Ident_foa0(name)), 0);
        if (it == NULL) {
            Log_error(as->L, &patch->s, "undefined symbol '%.*s' referenced", name.count, name.data);
            continue;
        }
        sym = RbTree_ref(&as->symbols, it);
        if (sym->tag == sytLabel) {
            RefList_t *rfl;
            FindOrAdd foa = RbTree_find_or_add_(&refs.base, &make(RefList_t, .f = sym->id), 0);
            rfl = RbTree_ref0(&refs, foa.s);
            if (foa.f) {
                rfl->f = sym->id;
                Vector_init0With(&rfl->s, PoolAllocator, 8);
            }
            Vector_push(&rfl->s, patch->f);
        }
    }

    if (as->L->errors) goto Assembler_link_exit;

    // Start adding code
    Vector_expand(code, sizeof(CodeHeader));
    Vector_pushArr(code, Vector_begin(&as->constants), Vector_len(&as->constants));
    db = Vector_len(code);

    // Linking stage, patch all instructions that need to be patched
    ip = db;

    for (int i = 0; i < Vector_len(&as->instructions); i++) {
        RbTreeNode *it;
        Instruction *instr = Vector_at(&as->instructions, i);

        if (RbTree_find_(&as->patchWork.base, &make(Patch, .f = i), 0) != NULL) {
            instr->ii -= ip;
        }

        it = RbTree_find_(&refs.base, &make(RefList_t, .f = i), 0);
        if (it != NULL) {
            RefList_t *rfl = RbTree_ref(&refs, it);
            Vector_foreach(&rfl->s, j) {
                Instruction *ins = Vector_at(&as->instructions, j);
                ins->ii += ip;
            }
        }

        ip += instr->osz;
        if (instr->rmd == amImm || instr->iea)
            ip += vmSizeTbl[instr->ims];
    }

    VM_code_append_(code, Vector_begin(&as->instructions), Vector_len(&as->instructions));

    header = (CodeHeader *) Vector_begin(code);
    header->size = Vector_len(code);
    header->main = db + as->main;
    header->db = db;

Assembler_link_exit:
    return Vector_len(code);
}

void Assembler_init(Assembler *as, Lexer *lX)
{
    AssemblerCtx *ctx;
    csAssert0(as->ctx == NULL);
    // allocate context for assembler
    ctx = Allocator_cAlloc(PoolAllocator, sizeof(AssemblerCtx), 1);
    csAssert0(ctx != NULL);

    ITP_init(ctx, lX);
    ctx->L = lX->L;

    RbTree_initWith(&ctx->symbols, Assembler_symbol_cmp, PoolAllocator);
    RbTree_initWith(&ctx->patchWork, Assembler_patch_cmp, PoolAllocator);
    RbTree_initWith(&ctx->references, Assembler_symbol_ref_cmp, PoolAllocator);

    Vector_init0With(&ctx->instructions, DefaultAllocator, 1024);
    Vector_init0With(&ctx->constants,    DefaultAllocator, 1024);

    as->ctx = ctx;

    // argc can be used in source to refer to
    // the position of the number of function arguments in stack
    Assembler_define(ctx, "argc", 16);
    Assembler_define(ctx, "argv", 24);

    // define builtin variables
#define XX(I, N) Assembler_define(ctx, "__"#N, (u64)bnc##I);
#define UU(...)
    VM_NATIVE_OS_FUNCS(XX, UU)
#undef UU
#undef XX

    Assembler_define(ctx, "__stdin",  STDIN_FILENO);
    Assembler_define(ctx, "__stdout", STDOUT_FILENO);
    Assembler_define(ctx, "__stderr", STDERR_FILENO);
}

void Assembler_deinit(Assembler *as)
{
    AssemblerCtx *ctx = as->ctx;

    as->ctx = NULL;
    if (ctx == NULL) return;

    RbTree_deinit(&ctx->symbols);
    RbTree_deinit(&ctx->patchWork);
    RbTree_deinit0(&ctx->references, Assembler_symbol_ref_dctor);
    Vector_deinit(&ctx->instructions);
    Vector_deinit(&ctx->constants);
    Allocator_dealloc(ctx);
}


u32  Assembler_assemble(Assembler *as, Code *into)
{
    AssemblerCtx *ctx = as->ctx;

    csAssert0(into != NULL);
    csAssert0(ctx   != NULL);

    while (!ITP_eof(ctx)) {
        E4C_TRY {
            switch (ITP_curr(ctx)->kind) {
                case tokIdentifier:
                    if (ITP_peek(ctx)->kind == tokColon)
                        Assembler_parse_label(ctx);
                    else
                        Assembler_parse_instruction(ctx);
                    break;
                case tokDollar:
                    Assembler_parse_var_decl(ctx);
                    break;
                case tokNl: case tokComment:
                    ITP_advance(ctx);
                    break;
                default:
                    ITP_fail(ctx, "unsupported token");
                    break;
            }
        }
        E4C_CATCH(ITPException) {
            Assembler_synchronize(ctx);
        }
    }

    if (ctx->L->errors)
        return 0;

    return Assembler_link(ctx, into);
}