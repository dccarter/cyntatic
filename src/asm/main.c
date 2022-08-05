/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#include "args.h"
#include "file.h"

#include "asm/asm.h"

#include "compiler/compile.h"
#include "compiler/log.h"
#include "compiler/lexer.h"

Command(assem, "Assembles the given cyn assemble file into bytecode",
        Positionals(
                Str(Name("path"), Help("Path to the file containing the bytecode to disassemble"))
        )
);

Command(dassem, "disassembles the given bytecode file instead of running it",
        Positionals(
                Str(Name("path"), Help("Path to the file containing the bytecode to disassemble"))
        ),
        Opt(
                Name("hide-addr"), Sf('H'), Help("Hide instruction addresses from generated assembly")
        ),
        Str(
                Name("output"), Sf('o'),
                Help("Path to the output file, if not specified the disassembly will be dumped to console"),
                Def(""))
);

int cmdAssemble(CmdCommand *cmd);
int cmdDisAssemble(CmdCommand* cmd);

int main(int argc, char *argv[])
{
    Parser(CYN_APPLICATION_NAME, CYN_APPLICATION_VERSION,
           Commands(AddCmd(assem), AddCmd(dassem)),
           DefaultCmd(assem));

    i32 selected = argparse(&argc, &argv, parser);

    if (selected == CMD_assem) {
        return cmdAssemble(&assem.meta);
    }
    else if (selected == CMD_dassem) {
        return cmdDisAssemble(&dassem.meta);
    }
    else if (selected == CMD_help) {
        CmdFlagValue *cmd = cmdGetPositional(&help.meta, 0);
        cmdShowUsage(P, (cmd == NULL? NULL : cmd->str), stdout);
        return EXIT_SUCCESS;
    }

    fputs(P->error, stderr);
    return EXIT_FAILURE;
}

int cmdAssemble(CmdCommand *cmd)
{
#define checkErrors(L, msg)({                                                                   \
    do {                                                                                        \
        if ((L)->errors || (L)->warnings)                                                       \
            Log_print0(L, Stderr, "%s, errors: %u, warnings: %u", msg,                          \
                                    (L)->errors, (L)->warnings);                                \
    } while (0);                                                                                \
        (L)->errors;                                                                            \
    })

    Log L = {0};
    Source src = {0};
    Lexer lX = {0};
    Assembler as = {0};
    Code code = {0};
    u32 bytes;
    FILE *fp;
    const char *path;

    int exitCode = EXIT_FAILURE;

    CmdFlagValue *input = cmdGetPositional(cmd, 0);
    CmdFlagValue *output = cmdGetFlag(cmd, 0);

    Compiler_init();
    Log_init(&L);

    Source_open(&src, &L, input->str);
    if (checkErrors(&L, "opening source file failed"))
        return EXIT_FAILURE;

    path = input->str;
    if (output == NULL) {
        path = File_get_name(path);
        path = File_replace_ext(ArenaAllocator, path, ".bin");
    }
    else {
        path = output->str;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "opening output file '%s' failed\n", path);
        return EXIT_FAILURE;
    }

    Lexer_init(&lX, &L, &src);
    Assembler_init(&as, &lX);
    Vector_init(&code);

    bytes = Assembler_assemble(&as, &code);
    if (checkErrors(&L, "assembling source file failed"))
        goto cmdAssemble_cleanup;

    fwrite((const char *)Vector_begin(&code), bytes, 1, fp);

    exitCode = EXIT_SUCCESS;


cmdAssemble_cleanup:
    fclose(fp);
    Assembler_deinit(&as);
    Vector_deinit(&code);
    Source_deinit(&src);

    return exitCode;
}

int cmdDisAssemble(CmdCommand* cmd)
{
    CmdFlagValue *input = cmdGetPositional(cmd, 0);
    CmdFlagValue *hAddr = cmdGetFlag(cmd, 0);
    CmdFlagValue *output = cmdGetFlag(cmd, 1);

    Code code;
    Vector_init(&code);
    {
        FILE *fp = fopen(input->str, "r");
        if (fp == NULL) {
            fprintf(stderr, "error: opening binary executable '%s' failed\n", input->str);
            goto cmdDisAssemble_failure;
        }

        fseek(fp, 0, SEEK_END);
        Vector_expand(&code, ftell(fp));
        fseek(fp, 0, SEEK_SET);
        fread((char *) Vector_begin(&code), Vector_len(&code), 1, fp);
        fclose(fp);
    }

    if (output) {
        FILE *fp = fopen(output->str, "w");
        if (fp == NULL) {
            fprintf(stdout, "Unable to open output file '%s'\n", output->str);
            goto cmdDisAssemble_failure;
        }
        vmCodeDisassemble_(&code, fp, !(bool)hAddr->num);
        fclose(fp);
    }
    else
        vmCodeDisassemble_(&code, stdout, !(bool)hAddr->num);

    Vector_deinit(&code);
    return EXIT_SUCCESS;

cmdDisAssemble_failure:
    Vector_deinit(&code);
    return EXIT_FAILURE;
}
