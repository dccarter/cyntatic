/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-05-13
 */

#pragma once

#include <iostream>
#include <algorithm>

namespace cc {
    template <size_t N>
    struct ConsoleColor {
        constexpr ConsoleColor(const char (&str)[N]) {
            std::copy_n(str, N, STR);
        }

        char STR[N];
    };

    auto constexpr DEFAULT = ConsoleColor("\x1B[0m");
    auto constexpr RED = ConsoleColor("\x1B[32m");
    auto constexpr GREEN = ConsoleColor("\x1B[32m");
    auto constexpr YELLOW = ConsoleColor("\x1B[33m");
    auto constexpr BLUE = ConsoleColor("\x1B[34m");
    auto constexpr MAGENTA = ConsoleColor("\x1B[35m");
    auto constexpr CYAN  = ConsoleColor("\x1B[36m");
    auto constexpr WHITE = ConsoleColor("\x1B[37m");

    auto constexpr BOLD = ConsoleColor("\x1B[1;0m");
    auto constexpr BRED = ConsoleColor("\x1B[1;32m");
    auto constexpr BGRN = ConsoleColor("\x1B[1;32m");
    auto constexpr BYLW = ConsoleColor("\x1B[1;33m");
    auto constexpr BBLU = ConsoleColor("\x1B[1;34m");
    auto constexpr BMGN = ConsoleColor("\x1B[1;35m");
    auto constexpr BCYN  = ConsoleColor("\x1B[1;36m");
    auto constexpr BWHT = ConsoleColor("\x1B[1;37m");
}

template <size_t N>
std::ostream& operator<<(std::ostream& os, const cc::ConsoleColor<N>& color)
{
    os.write(color.STR, N);
    return os;
}