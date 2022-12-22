/**
 * Copyright (c) 2022 suilteam, Carter
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Mpho Mbotho
 * @date 2022-12-15
 */

#include "vm/builder.h"
#include "compiler/common/ident.h"
#include "compiler/common/itp.h"
#include "compiler/common/log.h"
#include "compiler/common/source.h"
#include "vm/builtins.h"
#include <unistd.h>

typedef Pair(u32, Range) Patch;
typedef Pair(u32, RbTree(u32)) SymbolRefers;
typedef RbTree(SymbolRefers *) SymbolReferences;
typedef Pair(u32, Vector(u32)) RefList_t;


struct Builder {
    u32 main;
    Vector(u8) constants;
    Vector(Instruction) instructions;
    RbTree(Symbol) symbols;
    RbTree(Patch)  patchWork;
    SymbolReferences references;
    Log *L;
};

static inline
int Builder_symbol_ref_cmp(const void *lhs, u32 len, const void *rhs)
{
    const SymbolRefers *aa = *((const SymbolRefers **)lhs),
                       *bb = *((SymbolRefers **)rhs);
    return RbTree_cmp_u32(&aa->f, len, &bb->f);
}

static inline
int Builder_ref_list_cmp(const void *lhs, u32 len, const void *rhs)
{
    const RefList_t *aa = (const RefList_t *)lhs,
                    *bb = (RefList_t *)rhs;
    return RbTree_cmp_u32(&aa->f, len, &bb->f);
}

static inline
int Builder_patch_cmp(const void *lhs, u32 len, const void *rhs)
{
    const Patch *aa = (const Patch *)lhs, *bb = (const Patch *)rhs;
    return RbTree_cmp_u32(&aa->f, len, &bb->f);
}

static inline
int Builder_symbol_cmp(const void *lhs, u32 len, const void *rhs)
{
    const Symbol *aa = (const Symbol *)lhs, *bb = (Symbol *)rhs;
    if (Ident_equals(&aa->name, &bb->name)) return 0;

    return RbTree_cmp_string(&aa->name.name, len, &bb->name.name);
}

static inline
void Builder_symbol_ref_dctor(void *elem)
{
    SymbolRefers *ref = *((SymbolRefers **)elem);
    RbTree_deinit(&ref->s);
    Allocator_dealloc(ref);
}

Symbol* Builder_findSymbol(Builder *builder, Ident name)
{
    Symbol q = {.name = name };
    RbTreeNode *it = RbTree_find_(&builder->symbols.base, &q, strlen(name.name));

    if (it == NULL)
        ITP_fail(builder, "referenced symbol '%s' does not exist", name.name);

    return RbTree_ref0(&builder->symbols, it);
}

Symbol *Builder_addSymbol(Builder *builder,
                          u32 pos,
                          SymbolTag tag,
                          Ident name,
                          const Range *range)
{
    Symbol q = { .name = name };
    FindOrAdd foa = RbTree_find_or_add_(&builder->symbols.base, &q, 0);
    if (!foa.f) {
        ITP_fail0(builder,
                  range,
                  "symbol with name '%s' already defined",
                  q.name.name);
    }
    RbTree_get0(&builder->symbols, foa.s) = make(Symbol,
                                            .name = q.name,
                                            .size = 0,
                                            .id = pos,
                                            .tag = tag,
                                            .range = *range);

    return RbTree_ref0(&builder->symbols, foa.s);
}

u32 Builder_addSymbolReference(Builder *builder,
                               u32 pos,
                               Ident name,
                               const Range *range,
                               bool addToPatchWork)
{
    Symbol q = {.name = name };
    RbTreeNode *it = RbTree_find_(&builder->symbols.base, &q, 0);
    if (it == NULL || RbTree_ref(&builder->symbols, it)->tag == sytLabel) {
        if (addToPatchWork) {
            RbTree_add(&builder->patchWork, make(Patch, .f = pos, .s = *range));
            return 0;
        }
        ITP_fail(builder,
                 "referenced symbol '%s' must be defined before use", name.name);
    }
    else
        return RbTree_ref(&builder->symbols, it)->id;
}

void Builder_define(Builder *builder, const char *name, u64 value)
{
    Symbol q = make(Symbol, .name = Ident_foa1(name));
    FindOrAdd foa = RbTree_find_or_add_(&builder->symbols.base, &q, 0);
    if (!foa.f) {
        Log_error(builder->L, &make(Range, 0), "builtin variable '%s' already defined", name);
        abortCompiler(builder->L, "assembler initialization failed");
    }
    RbTree_get0(&builder->symbols, foa.s) = make(Symbol,
                                            .name = q.name,
                                            .id = value,
                                            .tag = sytDefine);
}

void Builder_appendInstruction_(Builder *builder, Instruction *instr)
{
    Vector_push(&builder->instructions, *instr);
}

u32 Builder_appendString_(Builder *builder, const char *str, u32 len)
{
    u32 pos = Vector_len(&builder->constants) + sizeof(CodeHeader);
    Vector_pushArr(&builder->constants, str, len);
    return pos;
}

u32 Builder_pos(Builder *builder)
{
    return Vector_len(&builder->instructions);
}

u32 Builder_appendIntegralData(Builder *builder, i64 value, Mode mode)
{
    const u8 size = vmSizeTbl[mode];
    const u32 pos = Vector_len(&builder->constants);

    Vector_expand(&builder->constants, size);
    VM_write(Vector_at(&builder->constants, pos), value, mode);

    return size;
}

u32 Builder_link(Builder *builder, Code *code)
{
    u32 db, ip;
    CodeHeader *header;
    RbTree(RefList_t) refs;

    RbTree_initWith(&refs, Builder_ref_list_cmp, PoolAllocator);

    RbTree_for_each(&builder->patchWork, patch) {
        Symbol *sym;
        StringView name = Range_view(&patch->s);
        RbTreeNode *it = RbTree_find_(&builder->symbols.base,
                                      &make(Symbol, .name = Ident_foa0(name)), 0);
        if (it == NULL) {
            Log_error(builder->L, &patch->s, "undefined symbol '%.*s' referenced", name.count, name.data);
            continue;
        }
        sym = RbTree_ref(&builder->symbols, it);
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

    if (builder->L->errors) goto Builder_link_exit;

    // Start adding code
    Vector_expand(code, sizeof(CodeHeader));
    Vector_pushArr(code, Vector_begin(&builder->constants), Vector_len(&builder->constants));
    db = Vector_len(code);

    // Linking stage, patch all instructions that need to be patched
    ip = db;

    for (int i = 0; i < Vector_len(&builder->instructions); i++) {
        RbTreeNode *it;
        Instruction *instr = Vector_at(&builder->instructions, i);

        if (RbTree_find_(&builder->patchWork.base, &make(Patch, .f = i), 0) != NULL) {
            instr->ii -= ip;
        }

        it = RbTree_find_(&refs.base, &make(RefList_t, .f = i), 0);
        if (it != NULL) {
            RefList_t *rfl = RbTree_ref(&refs, it);
            Vector_foreach(&rfl->s, j) {
                Instruction *ins = Vector_at(&builder->instructions, j);
                ins->ii += ip;
            }
        }

        ip += instr->osz;
        if (instr->rmd == amImm || instr->iea)
            ip += vmSizeTbl[instr->ims];
    }

    VM_code_append_(code, Vector_begin(&builder->instructions), Vector_len(&builder->instructions));

    header = (CodeHeader *) Vector_begin(code);
    header->size = Vector_len(code);
    header->main = db + builder->main;
    header->db = db;

Builder_link_exit:
    return Vector_len(code);
}

static void Builder_init(Builder *builder, Log *L)
{
    builder->L = L;
    RbTree_initWith(&builder->symbols, Builder_symbol_cmp, PoolAllocator);
    RbTree_initWith(&builder->patchWork, Builder_patch_cmp, PoolAllocator);
    RbTree_initWith(&builder->references, Builder_symbol_ref_cmp, PoolAllocator);

    Vector_init0With(&builder->instructions, DefaultAllocator, 1024);
    Vector_init0With(&builder->constants,    DefaultAllocator, 1024);

    // argc can be used in source to refer to
    // the position of the number of function arguments in stack
    Builder_define(builder, "argc", 16);
    Builder_define(builder, "argv", 24);

    // define builtin variables
#define XX(I, N) Builder_define(builder, "__"#N, (u64)bnc##I);
#define UU(...)
    VM_NATIVE_OS_FUNCS(XX, UU)
#undef UU
#undef XX

    Builder_define(builder, "__stdin",  STDIN_FILENO);
    Builder_define(builder, "__stdout", STDOUT_FILENO);
    Builder_define(builder, "__stderr", STDERR_FILENO);
}

Builder *Builder_create(Log *L)
{
    Builder *builder = New(DefaultAllocator, Builder);
    Builder_init(builder, L);
    return builder;
}

void Builder_deinit(Builder *builder)
{
    RbTree_deinit(&builder->symbols);
    RbTree_deinit(&builder->patchWork);
    RbTree_deinit0(&builder->references, Builder_symbol_ref_dctor);
    Vector_deinit(&builder->instructions);
    Vector_deinit(&builder->constants);
    Allocator_dealloc(builder);
}



