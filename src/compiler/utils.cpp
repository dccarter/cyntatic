/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-04-29
 */

#include "compiler/log.hpp"
#include "compiler/source.hpp"
#include "compiler/utils.hpp"

#include <unordered_map>

namespace {
    static std::unordered_map<std::string_view, std::string> gStringCache{};
}

namespace cstar {

    const Source _InvalidSource;
    const Source& InvalidSource{_InvalidSource};

    Range::Range(const Range &other) noexcept
        : _source{other._source},
          start{other.start},
          end{other.end},
          position{other.position}
    {}

    Range::Range(Range &&other) noexcept
        : _source{std::exchange(other._source, nullptr)},
          start{std::exchange(other.start, 0)},
          end{std::exchange(other.end, 0)},
          position{std::exchange(other.position, {0, 0})}
    {}

    Range &Range::operator=(const Range &other) noexcept {
        if (this != &other) {
            _source = other._source;
            start = other.start;
            end = other.end;
            position = other.position;
        }
        return *this;
    }

    Range &Range::operator=(Range &&other) noexcept     {
        if (this != &other) {
            _source = std::exchange(other._source, nullptr);
            start = std::exchange(other.start, 0);
            end = std::exchange(other.end, 0);
            position = std::exchange(other.position, {0, 0});
        }
        return *this;
    }

    std::string_view Range::toString() const
    {
        std::string_view sv{_source->contents()};
        return sv.substr(start, end-start);
    }

    const Source &Range::source() const
    {
        if (_source == nullptr) {
            return InvalidSource;
        }
        return *_source;
    }

    Range Range::enclosingLine() const
    {
        uint32_t s{start}, e{end};
        auto src = source();
        while (s > 0 and src[s] != '\n') s--;
        s = src[s] == '\n'? s+1 : s;
        while (e < src.size() and src[e] != '\n') e++;
        return Range{*_source, s, e, position};
    }

    Range Range::rangeAtEnd() const
    {
        return Range{*_source, end, end};
    }

    Range Range::merge(const Range &other) const
    {
        csAssert(_source == other._source);
        auto s = std::min(start, other.start);
        auto e = std::max(end, other.end);

        return Range{*_source, s, e, (s == start)? position : other.position};
    }

    void Range::merge(const Range &other)
    {
        csAssert(_source == other._source);
        if (other.start < start) {
            position = other.position;
        }

        start = std::min(start, other.start);
        end = std::max(end, other.end);
    }

    Range Range::extend(const Range &other) const
    {
        csAssert(_source == other._source);
        csAssert(start <= other.start);
        csAssert(end <= other.end);
        return Range{*_source, start, other.end, position};
    }

    void Range::extend(const Range &other)
    {
        csAssert(_source == other._source);
        csAssert(start <= other.start);
        csAssert(end <= other.end);
        end = other.end;
    }

    Range Range::sub(uint32_t s, uint32_t len) const
    {
        auto i = start + s;
        csAssert(i <= end);
        auto e = (len == 0)? end: i + len;
        csAssert(e <= end);
        auto x = 0u;
        LineColumn pos = position;
        auto src = source();
        while (x < i) {
            if (src[x] == '\n') {
                pos.line++;
                pos.column = 0;
            }
            else {
                pos.column++;
            }
            x++;
        }

        return {*_source, i, e, pos};
    }

    const Range &Range::Invalid()
    {
        static const Range INVALID_RANGE{};
        return INVALID_RANGE;
    }

    std::string_view locs(std::string_view str)
    {
        auto it = gStringCache.find(str);
        if (it == gStringCache.end()) {
            std::string tmp{str};
            str = tmp;
            it = gStringCache.emplace(str, std::move(tmp)).first;
        }
        return it->second;
    }

    std::string_view locs(std::string&& str)
    {
        std::string_view tmp{str};
        auto it = gStringCache.find(tmp);
        if (it == gStringCache.end()) {
            it = gStringCache.emplace(tmp, std::move(str)).first;
        }
        return it->second;
    }
}
