/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */

#pragma once

#include <buffer.h>

#include <stdarg.h>
#include <stdio.h>

typedef int (*StreamPrintf)(void *os, const char *fmt, va_list args);
typedef u32 (*StreamWrite)(void *os, const char *data, u32 size);

typedef struct {
    StreamPrintf fnPrintf;
    StreamWrite  fnWrite;
} StreamApi;

typedef struct {
    StreamApi *api;
    void      *os;
} Stream;

extern Stream *Stdout;
extern Stream *Stderr;

void Streams_init(void);

Stream FileStream_attach(FILE *fp);
#define FileStream_detach(FS) F(S)->os = NULL

Stream StringStream_attach(Buffer *buffer);
#define StringStream_detach(SS) (SS)->os = NULL

int Stream_printf(Stream *S, const char *fmt, ...);

attr(always_inline)
static int Stream_vprintf(Stream *S, const char *fmt, va_list args)
{
    csAssert0(S != NULL);
    S->api->fnPrintf(S->os, fmt, args);
}

u32 Stream_write(Stream *S, const char *data, u32 size);

attr(always_inline)
static void Stream_putc(Stream *S, char c)
{
    Stream_write(S, &c, 1);
}

u32 Stream_putUtf8(Stream *S, u32 chr);

attr(always_inline)
static void Stream_bWrite8(Stream *S, u8 v)
{
    Stream_write(S, (char *)&v, 1);
}

#define Stream_bWrite16(OS, V)                           \
    ({ u16 LineVAR(v16) = (V);                           \
       Stream_bWrite8((OS), (u8)(LineVAR(v16) & 0xFF));  \
       Stream_bWrite8((OS), (u8)(LineVAR(v16) >> 8));    \
    })

#define Stream_bWrite32(OS, V)                               \
    ({ u16 LineVAR(v32) = (V);                               \
       Stream_bWrite16((OS), (u8)(LineVAR(v32) & 0xFFFF));   \
       Stream_bWrite16((OS), (u8)(LineVAR(v32) >> 16));      \
    })

#define Stream_puts(S, str) Stream_write((S), (str), strlen(str))
