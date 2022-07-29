/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-20
 */

#include <stdlib.h>

#include "args.h"
#include "asm.h"

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
