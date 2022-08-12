/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-08-04
 */

#include "file.h"

#include <string.h>

const char *File_get_name(const char *path)
{
    const char *p = strrchr(path, '/');
    return (p == NULL)? path : p+1;
}

char *File_replace_ext(Allocator *A, const char *path, char *with)
{
    char *str;
    u32 stem, ext = strlen(with);
    const char *e = strrchr(path, '.');

    if (e == NULL)
        stem = strlen(path);
    else
        stem = (e - path);
    str = Allocator_alloc(A, ext + stem + 1);
    memcpy(str, path, stem);
    strcpy(&str[stem], with);

    return str;
}

bool File_read_all0(const char *path, Buffer *dst, Stream *es)
{
    u32 n = 0, len;
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        if (es)
            Stream_printf(es, "error: opening binary executable '%s' failed\n", path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    Vector_expand(dst, len);
    fseek(fp, 0, SEEK_SET);
    while (n != len)
        n += fread((Vector_begin(dst) + n), 1, (len - n), fp);

    fclose(fp);
    return true;
}