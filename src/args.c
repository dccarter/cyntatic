/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-21
 */

#include "args.h"

#include <stdlib.h>


i32  parseCommandLineArguments_(int *argc,
                                char **argv,
                                char **eArgv,
                                CmdlCommand **commands,
                                u32 cmdCount,
                                CmdlCommand *defCmd,
                                CmdlArg *gArgs,
                                u32 gArgsCount)
{
    return 0;
}

bool cmdlParseString(CmdlArgValue* dst, const char *str, u32 size)
{
    dst->state = cmdlString;
    dst->str = strndup(str, size);
    return true;
}

bool cmdlBooleanString(CmdlArgValue* dst, const char *str, u32 size)
{
    if (size == 1) {
        if (str[0] != '1' && str[0] != '0') {
            // report error
            return false;
        }
        dst->num = str[0] == '1';
    }
    else if (strncasecmp("TRUE", str, size) == 0) {
        dst->num = true;
    }
    else if (strncasecmp("FALSE", str, size) == 0) {
        dst->num = false;
    }
    else {

    }

    dst->state = cmdlNumber;
    return true;
}

bool cmdlNumberString(CmdlArgValue* dst, const char *str, u32 size)
{
    char *p = strndup(str, size);
    char *end;
    dst->num = strtod(p, &end);
    if (end - p != size) {
        free((void *)p);
        // error
        return false;
    }

    dst->state = cmdlNumber;
    return true;
}

void cmdlGetHelp(const CmdlParser *parser, const char *cmd, char output[], size_t osz)
{

}
