/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "compiler/source.h"
#include "compiler/log.h"
#include "compiler/ident.h"

#include <stdio.h>

void Source_init(Source *S, char *path)
{
    Vector_initWith(&S->contents, ArenaAllocator);
    S->name = path;
}

bool Source_open0(Source *S, struct CynLog *L, Range *range, const char *path)
{
    csAssert0(S->name == NULL);

    Source_init(S, S->name);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        Log_appendf(L, logError, range, "opening source file '%s' failed\n", path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    Vector_expand(&S->contents, ftell(fp));
    fseek(fp, 0, SEEK_SET);
    fread((char *) Vector_begin(&S->contents), Source_len(S), 1, fp);
    fclose(fp);

    return true;
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

StringView Range_view(Range *S)
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

    u32 s = S->start, e = S->end, len;

    const char *src = Source_src(S->source);
    len = Source_len(S->source);

    while (s > 0 && src[s] != '\n') s--;
    s += (src[s] == '\n'? 1 : 0);
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
