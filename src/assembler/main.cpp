/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-20
 */

#include "clip.h"
#include "vm/assembler.hpp"
#include "compiler/source.hpp"
#include "compiler/lexer.hpp"

#include <fstream>

using cyn::Source;
using cyn::Assembler;
using cyn::Log;
using cyn::Lexer;

using namespace clipp;

enum class CmdMode {
    Assem, DisAssem, Help
};

struct Command {
    std::string input{};
    std::string output{};
};

int cmdAssemble(Command& cmd);
int cmdDisAssemble(Command& cmd);
void showVersion();

int main(int argc, char *argv[])
{
    CmdMode mode{CmdMode::Assem};
    Command assem{}, disassem{};

    auto cmdAssem = (
            command("assem").set(mode, CmdMode::Assem),
            value("input", assem.input) % "Path to the file that needs to be assembled",
            option("-o", "--output") & value("output", assem.output) %
                "The file to save save the dis-assembled code to. (default: <input>.bin)");

    auto cmdDisAssem = (
            command("dis-assem").set(mode, CmdMode::DisAssem),
            value("input", disassem.input) % "Path to the file that need to be dis-assembled",
            option("-o", "--output") & value("output", disassem.output) % "The file to save save the dis-assembled code to");

    auto cli = (
            (cmdAssem | cmdDisAssem | command("help").set(mode, CmdMode::Help)),
            option("-v", "--version").call(showVersion).doc("Shows the version of this assembler"));

    if (parse(argc, argv, cli)) {
        switch (mode) {
            case CmdMode::Assem:
                return cmdAssemble(assem);
            case CmdMode::DisAssem:
                return cmdDisAssemble(disassem);
            default:
                std::cout << make_man_page(cli, CYN_APPLICATION_NAME);
                return EXIT_SUCCESS;
        }
    }
    else {
        std::cerr << usage_lines(cli, CYN_APPLICATION_NAME) << "\n";
        return EXIT_FAILURE;
    }
}

void checkErrors(Log& L)
{
    if (L.hasErrors())
        cyn::abortCompiler(L);
}

int cmdAssemble(Command& cmd)
{
    Log L;
    Source src(L, std::filesystem::path{cmd.input});
    checkErrors(L);

    Lexer lexer(L, src);
    lexer.tokenize();
    checkErrors(L);

    Code code;
    Vector_init(&code);
    Assembler assembler(L, lexer.tange());

    u32 bytes = assembler.assemble(code);
    checkErrors(L);

    {
        std::filesystem::path output{cmd.output};
        if (cmd.output.empty()) {
            output = cmd.input;
            output = output.filename();
            output.replace_extension(".bin");
        }
        std::ofstream ofs(output, std::ios::out|std::ios::binary);
        ofs.write((const char *)Vector_begin(&code), bytes);
        ofs.close();
    }

    return EXIT_SUCCESS;
}

int cmdDisAssemble(Command& cmd)
{
    Code code;
    Vector_init(&code);
    {
        std::ifstream ifs(cmd.input, std::ios::out | std::ios::binary);
        if (!ifs) {
            std::cerr << "opening input file '" << cmd.input << "' failed\n" << std::endl;
            Vector_deinit(&code);
            return EXIT_FAILURE;
        }

        ifs.seekg(0, std::ios::end);
        Vector_expand(&code, ifs.tellg());
        ifs.seekg(0, std::ios::beg);
        ifs.read((char *) Vector_begin(&code), Vector_len(&code));
        ifs.close();
    }

    if (!cmd.output.empty()) {
        FILE *fp = fopen(cmd.output.c_str(), "w");
        if (fp == nullptr) {
            std::cerr << "Unable to open output file '" << cmd.output << "'" << std::endl;
            Vector_deinit(&code);
            return EXIT_FAILURE;
        }
        vmCodeDisassemble(&code, fp);
        fclose(fp);
    }
    else
        vmCodeDisassemble(&code, stdout);

    Vector_deinit(&code);
    return EXIT_SUCCESS;
}

void showVersion()
{
    std::cout << "Version: " << CYN_APPLICATION_VERSION << "\n";
}
