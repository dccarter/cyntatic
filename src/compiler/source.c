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

void rangeUpdate(Range *S, const Source *src, u32 start, u32 end, LineColumn pos)
{
    csAssert(S != NULL);
    csAssert(end >= start);
    S->source = src;
    S->start = start;
    S->end = end;
    S->coord = pos;
}