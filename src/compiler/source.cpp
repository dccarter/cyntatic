/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-05-14
 */

#include "compiler/source.hpp"

#include "compiler/log.hpp"
#include <fstream>

namespace cstar {

    Source::Source(Log& L, std::filesystem::path file)
        : _contents(readFile(L, file)),
          _name{file.string()}
    {}

    const char& Source::operator[](uint32_t index) const
    {
        csAssert(index < _contents.size());
        return _contents[index];
    }

    const char *Source::at(uint32_t index) const
    {
        csAssert(index <= _contents.size());
        return _contents.data() + std::min(index, uint32_t(_contents.size()));
    }

    std::string Source::readFile(Log& L, const std::filesystem::path& fname)
    {
        std::ifstream f;
        f.open(fname);
        if (!f) {
            L.error({}, "could not open file '", fname, "'");
            abortCompiler(L);
        }

        f.seekg(0, std::ios::end);
        std::string contents;
        contents.reserve(f.tellg());
        f.seekg(0, std::ios::beg);
        contents.assign(std::istreambuf_iterator<char>(f),
                        std::istreambuf_iterator<char>());
        f.close();

        return contents;
    }

}
