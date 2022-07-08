/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-05-10
 */

#pragma once

#include <ostream>
#include <string_view>

#include <compiler/utils.hpp>

namespace cstar {
    struct Log;
    std::pair<std::uint32_t, std::uint32_t> readRune(Log& L, const Range& range);
    void toUtf16(std::ostream& os, Log& L, const Range& range);
    void toUtf32(std::ostream& os, Log& L, const Range& range);
    void writeUtf8(std::ostream& os, Log* L, const Range& range, uint32_t chr);
    inline void writeUtf8(std::ostream& os, uint32_t chr) {
        writeUtf8(os, nullptr, {}, chr);
    }
    inline void writeChar(std::ostream& os, char chr)
    { os.write(&chr, 1); }
}