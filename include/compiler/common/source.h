/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#pragma once

#include "buffer.h"
#include "view.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Log_t;

typedef struct CynLineColumn {
    u32  line, column;
} LineColumn;

typedef struct CynPosition {
    u32 idx;
    LineColumn coord;
} Position;

typedef struct CynSource {
    char      *name;
    Buffer    contents;
} Source;

typedef struct CynRange {
    u32        start;
    u32        end;
    LineColumn coord;
    const Source *source;
} Range;

void Source_init(Source *S, const char *name);
void Source_deinit(Source *S);
void Source_load(Source *S, const char *name, const char* code);
bool Source_open0(Source *S, struct Log_t *Log, Range *range, const char *path);
#define Source_open(S, L, PATH) Source_open0((S), (L), NULL, (PATH))

#define Source_len(self)     Buffer_size(&(self)->contents)

attr(always_inline)
static const char* Source_at(const Source *S, u32 i)
{
    cynAssert(i <= Source_len(S), "Index out of bounds %d", i);
    return Vector_at(&S->contents, i);
}

#define Source_src(S) Source_at(S, 0)

attr(always_inline)
static void Range_init(Range *S)
{
    memset(S, 0, sizeof(*S));
}

attr(always_inline)
u32 Range_size(const Range *range)
{
    csAssert0(range->start <= range->end);
    return range->end - range->start;
}

void Range_update(Range *S, const Source *src, u32 start, u32 end, LineColumn pos);


StringView Range_view(const Range *S);

int Range_equals(const Range *rhs, const Range *lhs);

void Range_enclosingLine(const Range *S, Range *range);

attr(always_inline)
Range Range_enclosingLine0(const Range *S)
{
    Range range;
    Range_enclosingLine(S, &range);
    return range;
}

void Range_atEnd(const Range *S, Range *range);

attr(always_inline)
static Range Range_atEnd0(const Range *S)
{
    Range range;
    Range_atEnd(S, &range);
    return range;
}

void Range_merge(const Range *S, const Range *with, Range *into);

attr(always_inline)
static Range Range_merge0(const Range *S, const Range *with)
{
    Range range;
    Range_merge(S, with, &range);
    return range;
}

void Range_extend(const Range *S, const Range *with, Range *into);

attr(always_inline)
static Range Range_extend0(const Range *S, const Range *with, Range *into)
{
    Range range;
    Range_extend(S, with, &range);
    return range;
}

void Range_mergeWith(Range *S, const Range *with);

void Range_extendWith(Range *S, const Range *with);

void Range_sub(const Range *S, u32 s, u32 len, Range *into);

attr(always_inline)
static Range Range_sub0(const Range *S, u32 s, u32 len)
{
    Range range;
    Range_sub(S, s, len, &range);
    return range;
}


#ifdef __cplusplus
}
#endif