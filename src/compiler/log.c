/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#include "compiler/log.h"

void Log_init(Log *L)
{
    Vector_initWith(&L->diagnostics, ArenaAllocator);
    L->errors = false;
}

void Log_append(Log *L, LogKind kind, Range *range, char *message)
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

void Diagnostics_print_(const Diagnostic* diagnostic, FILE *fp)
{
    u32 col;
    const Range *range = &diagnostic->range;
    if (range->source)
        fprintf(fp, "%s:%u:%u:",
            range->source->name, range->coord.line+1, range->coord.column+1);

    if (diagnostic->kind == logError)
        fputs("error: ", fp);
    else
        fputs("warning: ", fp);
    fputs(diagnostic->message, fp);
    fputc('\n', fp);

    if (range->source) {
        col = range->coord.column;
        fprintf(fp, "%*c", col, ' ');
        if (Range_size(range) <= 1) {
            fputc('^', fp);
        } else {
            u32 i = range->start;
            const char *src = Source_src(range->source);
            while (i < range->end && src[i++] != '\n')
                fputc('~', fp);
        }
        fputc('\n', fp);
    }
}

void Log_print_(const Log* L, FILE *fp,  const char *errMsg)
{
    if (L->errors) {
        fputs(cBRED "error:" cDEF " ", fp);
        if (errMsg) fputs(errMsg, fp);
        else fputs("compilation failed!", fp);
        fputc('\n', fp);
    }

    Vector_foreach_ptr(&L->diagnostics, diagnostic) {
        u32 i;
        Range *range = &diagnostic->range;

        if (range->source)
            fprintf(fp, cBOLD "%s:%u:%u " cDEF, range->source->name, range->coord.line, range->coord.column);

        if (diagnostic->kind == logError)
            fputs(cRED "error: " cBOLD, fp);
        else
            fputs(cYLW "warning: " cBOLD, fp);

        fputs(diagnostic->message, fp);
        fputs(cDEF, fp);
        fputc('\n', fp);

        if (range->source) {
            StringView view = Range_view(range);

            fwrite(view.data, 1, view.count, fp);
            fputc('\n', fp);
            fprintf(fp, "%*c", range->coord.column, ' ');
            fputc('^', fp);
            i = range->start + 1;
            while (i < range->end && view.data[i++] != '\n')
                fputc('~', fp);

            fputc('\n', fp);
        }
    }
}
