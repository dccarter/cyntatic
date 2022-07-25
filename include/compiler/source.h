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

#include <buffer.h>
#include <view.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CynLog;

typedef struct CynLineColumn {
    u32  line, column;
} LineColumn;

typedef struct CynPosition {
    u32 index;
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

void sourceOpen(Source *S, struct CynLog *Log, const char *path);

#define sourceLen(self)     Buffer_size(&(self)->contents)

attr(always_inline)
static const char* sourceAt(Source *S, u32 i)
{
    cynAssert(i < sourceLen(S), "Index out of bounds %d", i);
    return Vector_at(&S->contents, i);
}

attr(always_inline)
static void rangeInit(Range *S)
{
    memset(S, 0, sizeof(*S));
}

void rangeUpdate(Range *S, const Source *src, u32 start, u32 end, LineColumn pos);


StringView rangeView(Range *S);

int rangeEquals(const Range *rhs, const Range *lhs);

Range enclosingLine(const Range *S);

Range rangeAtEnd(const Range *S);

Range rangeMerge(const Range *S, const Range *with);

Range rangeExtend(const Range *S, const Range *with);

void rangeMergeWith(Range *S, const Range *with);

void rangeExtendWith(Range *S, const Range *with);

Range rangeSubrange(const Range *S, const Range *with);


#ifdef __cplusplus
}
#endif