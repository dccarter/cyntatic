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
    Vector(Diagnostic *) diagnostics;
} Log;

attr(always_inline)
static void logAppend(Log *log, LogKind kind, Range* range, char *message)
{
}

#ifdef __cplusplus
}
#endif