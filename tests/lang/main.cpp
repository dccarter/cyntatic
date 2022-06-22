/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-04
 */

#include "compiler/lexer.hpp"
#include "compiler/source.hpp"

using cstar::Lexer;
using cstar::Log;
using cstar::Source;

int main(int argc, char *argv[])
{
    auto testScript = CSTAR_LANG_DIR "/hello.cstr";
    Log L;
    Source src{L, testScript};
    Lexer lexer{L, src};
    lexer.tokenize();
    auto& tokens = lexer.tokens();
    for (auto& tok: tokens) {
        std::cout << tok << std::endl;
    }

    abortCompiler(L);
}