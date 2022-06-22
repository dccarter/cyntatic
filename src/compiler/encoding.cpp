/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 *  Modified from https://github.com/rui314/8cc/blob/b480958396f159d3794f0d4883172b21438a8597/encoding.c
 *
 * @author Carter
 * @date 2022-05-10
 */

#include "compiler/encoding.hpp"
#include "compiler/log.hpp"

namespace {

    std::uint32_t countLeadingZeros(char c)
    {
        for (int i = 7; i >= 0; i--) {
            if ((c & (1 << i)) == 0)
                return 7 - i;
        }
        return 8;
    }

    inline void write(std::ostream& os, uint8_t byte)
    {
        os.write((char *)&byte, 1);
    }

    void write(std::ostream& os, uint16_t v)
    {
        write(os, uint8_t(v & 0xFF));
        write(os, uint8_t(v >> 8));
    }

    void write(std::ostream& os, uint32_t v)
    {
        write(os, uint16_t(v & 0xFFFF));
        write(os, uint16_t(v >> 16));
    }
}

namespace cstar {

    std::pair<std::uint32_t, std::uint32_t> readRune(Log& L, const Range& range)
    {
        auto sv = range.toString();
        auto len = countLeadingZeros(sv[0]);
        if (len == 0) {
            return {1, sv[0]};
        }

        if (sv.size() > len) {
            L.error(range, "invalid UTF-8 character sequence");
            cstar::abortCompiler(L);
        }

        for (auto i = 1u; i < len; i++) {
            if ((sv[i] & 0xC0) != 0x80) {
                L.error(range.sub(i, 1), "invalid UTF-8 continuation byte");
                cstar::abortCompiler(L);
            }
        }

        switch (len) {
            case 2:
                return {2, ((sv[0] & 0x1F) << 6) | (sv[1] & 0x3F)};
            case 3:
                return {3, ((sv[0] & 0xF) << 12) | ((sv[1] & 0x3F) << 6) | (sv[2] & 0x3F)};
            case 4:
                return {4, ((sv[0] & 0x7) << 18) | ((sv[1] & 0x3F) << 12) | ((sv[2] & 0x3F) << 6) | (sv[3] & 0x3F)};
            default:
                L.error(range, "invalid UTF-8 sequence");
                cstar::abortCompiler(L);
        }
    }

    void toUtf16(std::ostream& os, Log& L, const Range& range)
    {
        auto i = 0u;
        auto sz = range.size();
        while (i < sz) {
            auto [l, rune] = readRune(L, range.sub(i));
            if (rune < 0x10000) {
                write(os, uint16_t(rune));
            }
            else {
                write(os, uint16_t((rune >> 10) + 0xD7C0));
                write(os, uint16_t((rune & 0x3FF) + 0xDC00));
            }
            i++;
        }
    }

    void toUtf32(std::ostream& os, Log& L, const Range& range)
    {
        auto i = 0u;
        auto sz = range.size();
        while (i < sz) {
            auto [l, rune] = readRune(L, range.sub(i));
            write(os, rune);
            i++;
        }
    }

    void writeUtf8(std::ostream& os, Log* L, const Range& range, uint32_t chr)
    {
        if (chr < 0x80) {
            write(os, uint8_t(chr));
        }
        else if (chr < 0x800) {
            char c[] = {char(0xC0|(chr >> 6)),  char(0x80|(chr &  0x3F)), '\0'};
            os << c;
        }
        else if (chr < 0x10000) {
            char c[] = {
                    char(0xE0 | (chr >> 12)),
                    char(0x80 | ((chr >> 6) & 0x3F)),
                    char(0x80 | (chr & 0x3F)),
                    '\0'
            };
            os << c;
        }
        else if (chr < 0x200000) {
            char c[] = {
                    char(0xF0 | (chr >> 18)),
                    char(0x80 | ((chr >> 12) & 0x3F)),
                    char(0x80 | ((chr >> 6) & 0x3F)),
                    char(0x80 | (chr & 0x3F)),
                    '\0'
            };
            os << c;
        }
        else if (L) {
            L->error(range, "invalid UCS character: \\U", chr);
        }
        else {
            csAssert(false, "invalid UCS character");
        }
    }
}