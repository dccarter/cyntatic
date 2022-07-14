/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Carter
 * @date 2022-06-22
 */

#include "vm/vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void vmAbort(VM *vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    // vmStackTrace(vm);

    abort();
}
