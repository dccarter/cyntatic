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
} Diagnostic;

typedef struct CynLog {

} Log;

#ifdef __cplusplus
}
#endif