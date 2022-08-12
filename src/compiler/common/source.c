/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "compiler/common/source.h"
#include "compiler/common/log.h"
#include "compiler/common/ident.h"

#include "file.h"

#include <stdio.h>

void Source_init(Source *S, const char *name)
{
    Vector_initWith(&S->contents, ArenaAllocator);
    S->name = Allocator_strdup(PoolAllocator, name);
}

void Source_deinit(Source *S)
{
    if (S->name == NULL) return;
    Allocator_dealloc(S->name);
    S->name = NULL;
    Vector_deinit(&S->contents);
}

bool Source_open0(Source *S, struct Log_t *L, Range *range, const char *path)
{
    u32 n = 0;
    csAssert0(S->name == NULL);

    Source_init(S, path);

    if (!File_read_all(path, &S->contents)) {
        Log_appendf(L, logError, range, "opening source file '%s' failed\n", path);
        return false;
    }

    return true;
}

void Source_load(Source *S, const char *name, const char* code)
{
    csAssert0(S->name == NULL);
    Source_init(S, name);
    Buffer_appendStr(&S->contents, code, strlen(code));
}

void Range_update(Range *S, const Source *src, u32 start, u32 end, LineColumn pos)
{
    csAssert0(S != NULL);
    csAssert0(end >= start);
    S->source = src;
    S->start = start;
    S->end = end;
    S->coord = pos;
}

int Range_equals(const Range *rhs, const Range *lhs)
{
    return (rhs == lhs) ||
           ((rhs->source == lhs->source) &&
           (rhs->start == lhs->start) &&
           (rhs->end == lhs->end));

}

StringView Range_view(const Range *S)
{
    csAssert0(S->source != NULL);
    return (StringView){
        .data = Vector_at(&S->source->contents, S->start),
        .count = S->end - S->start
    };
}

void Range_enclosingLine(const Range *S, Range *range)
{
    csAssert0(S->source != NULL);

    i64 s = S->start - 1, e = S->end, len;

    const char *src = Source_src(S->source);
    len = Source_len(S->source);

    while (s >= 0 && src[s] != '\n') s--;
    ++s;
    while (e < len && src[e] != '\n') e++;

    Range_update(range, S->source, s, e, S->coord);
}

void Range_atEnd(const Range *S, Range *range)
{
    csAssert0(S->source != NULL);
    Range_update(range, S->source, S->end, S->end, S->coord);
}

void Range_merge(const Range *S, const Range *with, Range *into)
{
    u32 s, e;
    csAssert0(S->source != NULL);
    csAssert0(S->source == with->source);

    s = MIN(S->start, with->start);
    e = MAX(S->end, with->end);

    Range_update(into, S->source, s, e, (s == S->start)? S->coord : with->coord);
}

void Range_extend(const Range *S, const Range *with, Range *into)
{
    csAssert0(S->source == with->source);
    csAssert0(S->start <= with->start);
    csAssert0(S->end <= with->end);

    Range_update(into, S->source, S->start, with->end, S->coord);
}

void Range_mergeWith(Range *S, const Range *with)
{
    csAssert0(S->source == with->source);

    if (with->start < S->start) {
        S->coord = with->coord;
    }
    S->start = MIN(S->start, with->start);
    S->end = MAX(S->end, with->end);
}

void Range_extendWith(Range *S, const Range *with)
{
    csAssert0(S->source == with->source);
    csAssert0(S->start <= with->start);
    csAssert0(S->end <= with->end);

    S->end = with->end;
}

void Range_sub(const Range *S, u32 s, u32 len, Range *into)
{
    u32 i, e, x = 0;
    LineColumn coord = {0};
    const char *src;

    csAssert0(S->source != NULL);

    i = S->start + s;
    csAssert0(i <= S->end);
    e = (len == 0)? S->end : i + len;
    csAssert0(e <= S->end);

    src = Source_src(S->source);
    while (x < i) {
        if (src[x] == '\n') {
            ++coord.line;
            coord.column = 0;
        }
        else
            ++coord.column;

        ++x;
    }

    Range_update(into, S->source, i, e, coord);
}
