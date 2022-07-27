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

#include <compiler/source.h>
#include <compiler/heap.h>

#include <stream.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    logError,
    logWarning
} LogKind;

typedef struct CynDiagnostic {
    LogKind kind;
    Range   range;
    char   *message;
} Diagnostic;

typedef struct Log_t {
    u32 errors;
    u32 warnings;
    Vector(Diagnostic) diagnostics;
} Log;

void Log_init(Log *L);

void Log_append(Log *K, LogKind kind, Range *range, char *message);

#define Log_appendf(L, KIND, RNG, FMT, ...) do {                         \
        Buffer LineVAR(buf);                                             \
        Vector_initWith(&LineVAR(buf), PoolAllocator);                   \
        Buffer_appendf(&LineVAR(buf), (FMT), ##__VA_ARGS__);             \
        Log_append((L), (KIND), (RNG), Buffer_relocate(&LineVAR(buf),    \
                                                     ArenaAllocator));   \
    } while (0)

#define Log_error(LOG, RNG, fmt, ...) Log_appendf((LOG), logError, (RNG), (fmt), ##__VA_ARGS__)
#define Log_warn(LOG, RNG, fmt, ...)  Log_appendf((LOG), logWarning, (RNG), (fmt), ##__VA_ARGS__)

void Diagnostics_print0(const Diagnostic* diagnostic, Stream *os);
#define Diagnostics_print(D) \
    Diagnostics_print0((D), ((D)->kind == logError)? FileStream_attach(Stderr) : Stdout)

attr(format, printf, 3, 4)
void Log_print0(const Log* L, Stream *os, const char *fmt, ...);

#define Log_print(L, fmt, ...) \
    Log_print0(L, ((L)->errors? Stderr : Stdout), fmt, ##__VA_ARGS__)

attr(noreturn)
void abortCompiler0(const Log *L, Stream *os, const char *msg);
#define abortCompiler(L, msg)   \
    abortCompiler0((L), (L)->errors? Stderr : Stdout, (msg))\

#ifdef __cplusplus
}
#endif