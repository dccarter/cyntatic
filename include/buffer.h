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

#include <vector.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CYN_BUFFER_MIN_SIZE
#define CYN_BUFFER_MIN_SIZE 8
#endif

typedef Vector(char) Buffer;

#define Buffer_init(self) Vector_init0(self, CYN_BUFFER_MIN_SIZE)
#define Buffer_initWith(self, A) Vector_init0With(self,  (A), CYN_BUFFER_MIN_SIZE)
#define Buffer_ini1t(self, N) Vector_init0((self), (N))
#define Buffer_init1With(self, A, N) Vector_init0With(self,  (A), (N))
#define Buffer_init0(self, cstr) (Buffer_init(self), Vector_pushArr((self), (cstr), strlen(cstr))
#define Buffer_init0With(self, A, cstr) (Buffer_initWith((self), (A)), Vector_pushArr(self, (cstr), strlen(cstr)))

#define Buffer_appendStr(self, str, len) Vector_pushArr((self), (str), (len))
#define Buffer_appendCstr(self, cstr) Vector_pushArr((self), (cstr), strlen(cstr))
#define Buffer_appendC(self, C) Vector_push((self), (C))

#define Buffer_appendX(self, fmt, X, V) ({                                           \
        Vector_reserve((self), (X)+1);                                               \
        int LineVAR(len) = snprintf(Vector_end(self), (X), (fmt), (V));              \
        Vector_expand((self), LineVAR(len));                                         \
    })

#define Buffer_append(self, fmt, V)     \
    Buffer_appendX((self) (fmt), 64, (V))


#define Buffer_appendf(self, fmt, ...) Buffer_appendf_((self), (fmt), ##__VA_ARGS__)
#define Buffer_appendfX(self, X, fmt, ...) ({                                               \
        Vector_reserve((self), (N));                                                        \
        Buffer_appendf((self), fmt, __VA_ARGS__);                                           \
    })

#define Buffer_release(self)    Buffer_release_((self), true)
#define Buffer_cstr(self)       Vector_begin(self)

#define Buffer_size(self)       Vector_len(self)

attr(format, printf, 2, 3)
void Buffer_appendf_(Ptr(Buffer) self, const char *fmt, ...);

int Buffer_vappendf(Ptr(Buffer) self, const char *fmt, va_list args);

attr(always_inline)
const char* Buffer_seal(Ptr(Buffer) self)
{
    Vector_reserve(self, 1);
    Vector_back(self) = '\0';
    return Buffer_cstr(self);
}

char *Buffer_release_(Ptr(Buffer) self, bool compact);

char *Buffer_relocate(Ptr(Buffer) self, Allocator *to);

#define join(ARR, LEN, SEP, APPEND)                 \
    ({                                              \
        Buffer out;                                 \
        Buffer_init(&out);                          \
        for (u64 i = 0; i < (LEN); ++i ) {          \
            if (i) Buffer_appendCstr(&out, (SEP));  \
            APPEND(&out, (ARR)[i]);                 \
        }                                           \
        Buffer_release_(&out, false);               \
    })

#ifdef __cplusplus
}
#endif
