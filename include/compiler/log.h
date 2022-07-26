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

typedef struct CynLog {
    u32 errors;
    u32 warnings;
    Vector(Diagnostic) diagnostics;
} Log;

void Log_init(Log *L);

void Log_append(Log *K, LogKind kind, Range *range, char *message);

#define Log_appendf(L, KIND, RNG, FMT, ...) do {                         \
        Buffer LineVAR(buf);                                             \
        Vector_initWith(&LineVAR(buf), ArenaAllocator);                  \
        Buffer_appendf(&LineVAR(buf), (FMT), ##__VA_ARGS__);             \
        Log_append((L), (KIND), (RNG), Buffer_release(&LineVAR(buf)));   \
    } while (0)

#define Log_error(LOG, RNG, fmt, ...) Log_appendf((LOG), logError, (RNG), (fmt), ##__VA_ARGS__)
#define Log_warn(LOG, RNG, fmt, ...)  Log_appendf((LOG), logWarning, (RNG), (fmt), ##__VA_ARGS__)

void Diagnostics_print_(const Diagnostic* diagnostic, Stream *os);
#define Diagnostics_print(D) \
    Diagnostics_print_((D), ((D)->kind == logError)? FileStream_attach(Stderr) : Stdout)

void Log_print_(const Log* L, Stream *os,  const char *errMsg);
#define Log_print(L, msg) \
    Log_print_(L, ((L)->hasErrors? Stderr : Stdout), msg)

#ifdef __cplusplus
}
#endif