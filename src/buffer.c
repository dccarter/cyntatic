/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "buffer.h"

#include <stdarg.h>
#include <stdio.h>

void Buffer_appendf_(Ptr(Buffer) self, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Buffer_vappendf(self, fmt, args);
    va_end(args);
}

int Buffer_vappendf(Ptr(Buffer) self, const char *fmt, va_list arguments)
{
    Vector_reserve0(self, strlen(fmt));

    for (;;) {
        va_list args;
        u32 writable = Vector_capacity(self) - Vector_len(self);
        va_copy(args, arguments);
        int nwr = vsnprintf(Vector_end(self), writable, fmt, args);
        if (nwr > writable) {
            Vector_reserve(self, (nwr + writable));
            continue;
        }
        Vector_expand(self, nwr);
        return nwr;
    }
    unreachable();
}
