/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-24
 */


#include "vm/builtins.h"
#include "args.h"
#include <unistd.h>

void vmShowUsage(void)
{

}

Command(dassem, "disassembles the given bytecode file instead of running it",
    Positionals(Str("file", "Path to the file containing the bytecode to disassemble")),
    Opt(Name("hide-addr"), Sf('h'), Help("Hide instruction addresses from generated assembly")),
    Str(Name("output"), Sf('h'), Help("Path to the output file, if not specified the disassembly will be dumped to console"), Def(""))
);

void cmdDassem(CmdlArgValue *args, int argc, char **argv);

Command(run, "runs the given bytecode file, parsing any command line arguments following the file name to the program",
    Positionals(Str("path", "Path to the file containing the bytecode to run")),
    Str(Name("hide-addr"), Sf('h'), Help("Hide instruction addresses from generated assembly"), Def("")));

void cmdRun(CmdlArgValue *args, int argc, char **argv);

int main(int argc, char *argv[])
{
    char *eArgv[argc];

    Parser(CYN_APPLICATION_NAME, CYN_APPLICATION_VERSION,
           Commands(AddCmd(help), AddCmd(run), AddCmd(dassem)),
           DefaultCmd(run));


    int selected = argparse(&argc, argv, eArgv, parser);

    if (selected == CMD_dassem) {
        cmdDassem(dassem.vals, argc, eArgv);
    }
    else if (selected == CMD_run) {
        cmdRun(run.vals, argc, eArgv);
    }
    else if (selected == CMD_help) {
        char buf[512];
        cmdlGetHelp(&parser.p, help.vals[0].str, buf, sizeof(buf));
        fputs(buf, stdout);
        fputc('\n', 0);
    }

    return EXIT_SUCCESS;
}

bool loadCode(Code *code, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "error: opening binary executable '%s' failed\n", path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    Vector_expand(code, ftell(fp));
    fseek(fp, 0, SEEK_SET);
    fread((char *) Vector_begin(code), Vector_len(code), 1, fp);
    fclose(fp);
    return true;
}

void cmdDassem(CmdlArgValue *args, int argc, char **argv)
{
    FILE *fp = stdout;
    Code code;
    Vector_init(&code);
    if (!loadCode(&code, args[0].str))
        exit(EXIT_FAILURE);

    if (args[2].str) {
        fp = fopen(args[2].str, "w");
        if (fp == NULL) {
            fprintf(stderr, "error: opening output file '%s' failed\n", args[2].str);
            Vector_deinit(&code);
            exit(EXIT_FAILURE);
        }
    }

    vmCodeDisassemble_(&code, fp, (bool)args[1].num);
    Vector_deinit(&code);
}

void cmdRun(CmdlArgValue *args, int argc, char **argv)
{
    VM vm = {0};
    Code code;
    Vector_init(&code);
    if (!loadCode(&code, args[0].str))
        exit(EXIT_FAILURE);

    vmInit(&vm, &code, CYN_VM_DEFAULT_MS);
    vmRun(&vm, argc, argv);
    vmDeInit(&vm);
    Vector_deinit(&code);
}