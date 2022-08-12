/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "compiler/common/log.h"

void Log_init(Log *L)
{
    Vector_init0With(&L->diagnostics, DefaultAllocator, 32);
    L->errors = false;
}

void Log_append(Log *L, LogKind kind, const Range *range, char *message)
{
    Diagnostic *diagnostic = Vector_expand(&L->diagnostics, 1);
    diagnostic->kind = kind;
    diagnostic->message = message;
    if (range)
        diagnostic->range = *range;
    else
        Range_update(&diagnostic->range, NULL, 0, 0, (LineColumn){});

    if (kind == logError)
        ++L->errors;
    else
        ++L->warnings;
}

void Diagnostics_print_(const Diagnostic* diagnostic, Stream *os)
{
    u32 col;
    const Range *range = &diagnostic->range;
    if (range->source)
        Stream_printf(os, "%s:%u:%u:",
            range->source->name, range->coord.line+1, range->coord.column+1);

    if (diagnostic->kind == logError)
        Stream_puts(os, "error: ");
    else
        Stream_puts(os, "warning: ");
    Stream_puts(os, diagnostic->message);
    Stream_putc(os, '\n');

    if (range->source) {
        col = range->coord.column;
        Stream_printf(os, "%*c", col, ' ');
        if (Range_size(range) <= 1) {
            Stream_putc(os, '^');
        } else {
            u32 i = range->start;
            const char *src = Source_src(range->source);
            while (i < range->end && src[i++] != '\n')
                Stream_putc(os, '~');
        }
        Stream_putc(os, '\n');
    }
}

void Log_print0(const Log* L, Stream *os,  const char *fmt, ...)
{
    if (L->errors) {
        Stream_puts(os, cBRED "error:" cDEF " ");
        if (fmt) {
            va_list args;
            va_start(args, fmt);
            Stream_vprintf(os, fmt, args);
            va_end(args);
        }
        else
            Stream_puts(os, "compilation failed!");
        Stream_putc(os, '\n');
    }

    Vector_foreach_ptr(&L->diagnostics, diagnostic) {
        u32 i;
        Range *range = &diagnostic->range;

        if (range->source)
            Stream_printf(os,
                          cBOLD "%s:%u:%u " cDEF,
                          range->source->name, range->coord.line+1, range->coord.column+1);

        if (diagnostic->kind == logError)
            Stream_puts(os, cRED "error: " cBOLD);
        else
            Stream_puts(os, cYLW "warning: " cBOLD);

        Stream_puts(os, diagnostic->message);
        Stream_puts(os, cDEF);
        Stream_putc(os, '\n');

        if (range->source) {
            Range line = Range_enclosingLine0(range);
            StringView view = Range_view(&line);

            Stream_write(os, view.data, view.count);
            Stream_putc(os, '\n');
            if (range->coord.column)
                Stream_printf(os, "%*c", range->coord.column, ' ');
            Stream_putc(os, '^');
            i = range->start + 1;
            while (i < range->end && view.data[i++] != '\n')
                Stream_putc(os, '~');

            Stream_putc(os, '\n');
        }
    }
}

void abortCompiler0(const Log *L, Stream *os, const char *msg)
{
    Log_print0(L, os, "%s, errors: %u, warnings: %u", msg, L->errors, L->warnings);

    exit(L->errors? EXIT_FAILURE : EXIT_SUCCESS);
}
