/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-08-04
 */

#pragma once

#include <allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *File_get_name(const char *path);

char *File_replace_ext(Allocator *A, const char *path, char *with);

#ifdef __cplusplus
}
#endif