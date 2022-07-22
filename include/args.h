/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-21
 */

#pragma once

#include <common.h>

enum {
    cmdlNoValue,
    cmdlNumber,
    cmdlString
};

typedef struct CommandLineArgumentValue {
    u8   state;
    union {
        f64 num;
        const char *str;
    };
} CmdlArgValue;

typedef struct CommandLineArgument {
    const char *name;
    char sf;
    const char *help;
    bool(*validator)(CmdlArgValue*, const char *, u32);
    const char *def;
} CmdlArg;

typedef struct CommandLinePostional {
    const char *name;
    const char *help;
    const char *def;
    bool(*validator)(CmdlArgValue*, const char *, u32);
} CmdlPositional;

typedef struct CommandLineCommand {
    const char *name;
    const char *help;
    u32         nargs;
    u32         npos;
} CmdlCommand;

typedef struct CommandLineParser {
    const char *name;
    const char *version;
    CmdlCommand *def;
    u32         ncmds;
    u32         nargs;
} CmdlParser;

bool cmdlParseString(CmdlArgValue* dst, const char *str, u32 size);
bool cmdlBooleanString(CmdlArgValue* dst, const char *str, u32 size);
bool cmdlNumberString(CmdlArgValue* dst, const char *str, u32 size);

#define Name(N) .name = N
#define Sf(S) .sf = S
#define Help(H) .help = H
#define Type(V) .validator = V
#define Def(D) .def = D
#define Positionals(...) { __VA_ARGS__ }
#define Arg(...) ((CmdlArg){ __VA_ARGS__ })
#define Opt(...)  {__VA_ARGS__, .validator = NULL}
#define Str(...)  {__VA_ARGS__, .validator = cmdlParseString}
#define Num(...)  {__VA_ARGS__, .validator = cmdlNumberString}
#define Bool(...) {__VA_ARGS__, .validator = cmdlBooleanString}

#define Sizeof(T, ...) (sizeof((T[]){__VA_ARGS__})/sizeof(T))
#define Command(N, H, P, ...)                                                   \
    static int CMD_##N = 0;                                                     \
    typedef struct Cmd##N Cmd##N;                                               \
    struct Cmd##N {                                                             \
        CmdlCommand meta;                                                       \
        CmdlArg args[Sizeof(CmdlArg, __VA_ARGS__)];                             \
        CmdlArgValue vals[Sizeof(CmdlArg, __VA_ARGS__) +                        \
                         (sizeof((CmdlPositional[])P)/sizeof(CmdlPositional))]; \
        CmdlPositional pos[sizeof((CmdlPositional[])P)/sizeof(CmdlPositional)]; \
    } N = {                                                                     \
        .meta = {                                                               \
            .name = #N,                                                         \
            .help = H,                                                          \
            .nargs = Sizeof(CmdlArg, __VA_ARGS__),                              \
            .npos  = (sizeof((CmdlPositional[])P)/sizeof(CmdlPositional))       \
        },                                                                      \
        .args = {                                                               \
            __VA_ARGS__                                                         \
        }                                                                       \
    }

#define AddCmd(N) ({ CMD_##N = cmdCOUNT++; &((N).meta); })

i32  parseCommandLineArguments_(int *argc,
                                char **argv,
                                char **eArgv,
                                CmdlCommand **commands,
                                u32 cmdCount,
                                CmdlCommand *defCmd,
                                CmdlArg *gArgs,
                                u32 gArgsCount);

#define RequireCmd NULL
#define DefaultCmd(C) (&((C).meta))
#define Commands(...) { __VA_ARGS__ }


#define CMDL_HELP_CMD                                                                           \
Command(help, "Get the application or help related to a specific command",                      \
        Positionals(Str("command", "The command whose help should be retrieved", Def("")))      \
);

#define Parser(N, V, CMDS, DEF, ...)                                    \
    CMDL_HELP_CMD                                                       \
    int cmdCOUNT = 0;                                                   \
    struct {                                                            \
        CmdlParser p;                                                   \
        CmdlCommand *cmds[(sizeof((CmdlCommand*[])CMDS) /               \
                               sizeof(CmdlCommand*)) + 1];              \
        CmdlArg     args[2+ Sizeof(CmdlArg, __VA_ARGS__)];              \
    } parser = {                                                        \
        .p = {                                                          \
            .name = N,                                                  \
            .version = V,                                               \
            .def = DEF,                                                 \
            .ncmds = 1+ (sizeof((CmdlCommand*[])CMDS) /                 \
                            sizeof(CmdlCommand*)),                      \
            .nargs = 2 + Sizeof(CmdlArg, __VA_ARGS__)                   \
        },                                                              \
        .cmds = CMDS,                                                   \
        .args = { Opt(Name("version"), Sf('v'),                         \
                     Help("Show the application version")),             \
                  Opt(Name("help"), Sf('h'),                            \
                      Help("Get held for the selected command")),       \
                  ##__VA_ARGS__                                         \
                }                                                       \
    }

void cmdlGetHelp(const CmdlParser *parser, const char *cmd, char output[], size_t osz);

#define argparse(ARGC, ARGV, EARGV, P)                  \
    parseCommandLineArguments_((ARGC), (ARGV), (EARGV), \
        (P).cmds, (P).p.ncmds,                          \
        (P).p.def,                                      \
        (P).args, (P).p.nargs)

