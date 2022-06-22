/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-05-14
 */

#pragma once

#include <filesystem>
#include <list>
#include <string>
#include <string_view>


namespace cstar {
    class Log;

    class Source {
    public:
        Source() = default;
        Source(std::string name, std::string contents)
            : _name{std::move(name)},
              _contents{std::move(contents)}
        {}

        Source(Log& L, std::filesystem::path file);

        const std::string& name() const { return _name; }
        const std::string& contents() const { return _contents; }
        std::size_t size() const { return _contents.size(); }

        const char& operator[](uint32_t index) const;
        const char* at(uint32_t index) const;
        bool operator==(const Source& other) const { return this == &other; }
        bool operator!=(const Source& other) const { return this != &other; }

    private:
        std::string readFile(Log& L, const std::filesystem::path& fname);
        std::string _name{};
        std::string _contents{};
    };
}