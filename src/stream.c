/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-25
 */

#include "stream.h"

static int FileStream_printf(void *os, const char *fmt, va_list args)
{
    csAssert(os != NULL, "Stream might not be valid");
    return vfprintf((FILE *)os, fmt, args);
}

static u32 FileStream_write(void *os, const char *data, u32 size)
{
    csAssert(os != NULL, "Stream might not be valid");
    return fwrite(data, 1, size, (FILE *)os);
}

static int StringStream_printf(void *os, const char *fmt, va_list args)
{
    csAssert(os != NULL, "Stream might not be valid");
    return Buffer_vappendf((Buffer  *)os, fmt, args);
}

static u32 StringStream_write(void *os, const char *data, u32 size)
{
    csAssert(os != NULL, "Stream might not be valid");
    Buffer_appendStr((Buffer  *)os, data, size);
    return size;
}

static StreamApi sFileStreamApi = {
    .fnWrite = FileStream_write,
    .fnPrintf = FileStream_printf
};

static StreamApi sStringStreamApi = {
    .fnPrintf = StringStream_printf,
    .fnWrite = StringStream_write
};

Stream FileStream_attach(FILE *fp)
{
    return (Stream) {
        .os = fp,
        .api = &sFileStreamApi
    };
}

Stream StringStream_attach(Buffer *buffer)
{
    return (Stream) {
        .os = buffer,
        .api = &sStringStreamApi
    };
}

int Stream_printf(Stream *S, const char *fmt, ...)
{
    csAssert0(S != NULL);
    va_list args;
    va_start(args, fmt);
    S->api->fnPrintf(S->os, fmt, args);
    va_end(args);
}

u32 Stream_write(Stream *S, const char *data, u32 size)
{
    csAssert0(S != NULL);
    return S->api->fnWrite(S->os, data, size);
}