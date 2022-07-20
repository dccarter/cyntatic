/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-04-29
 */

#pragma once


#include <memory>
#include <sstream>
#include <vector>
#include <iomanip>

namespace cyn {

#ifndef BIT
#define BIT(n) (1 << (n))
#endif

    template <typename T>
    using ptr = std::shared_ptr<T>;
    template <typename T>
    using wptr = std::shared_ptr<T>;

#define CSTAR_PTR(T) using Ptr = std::shared_ptr<T>

    template <typename T>
    using vec = std::vector<T>;


    template <typename T>
    struct Complex_;

    template <>
    struct Complex_<float> {
        typedef _Complex float type;
    };

    template <>
    struct Complex_<double> {
        typedef _Complex double type;
    };

    template <>
    struct Complex_<long double> {
        typedef _Complex long double type;
    };

    template <typename T>
        requires std::is_floating_point_v<T>
    using Complex = typename Complex_<T>::type;

    template <typename T>
    static inline T real(Complex<T> c) { return __real__ c; }

    template <typename T>
    static inline T imaginary(Complex<T> c) { return __imag__ c; }

    template <typename T, typename... Args>
    ptr<T> mk(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    class Source;
    extern const Source& InvalidSource;

    struct LineColumn {
        uint32_t line{0}, column{0};
    };

    struct Position {
        uint32_t index{0};
        LineColumn coord{};
    };

    struct Range {
        Range() = default;

        Range(const Source& src, uint32_t  start, uint32_t end, LineColumn pos = {0})
            : _source(&src),
              start{start},
              end{end},
              position{pos}
        {}

        Range(const Source& src, Position  pos, uint32_t end)
            : _source(&src),
              start{pos.index},
              end{end},
              position{pos.coord}
        {}

        Range(const Range& other) noexcept;
        Range& operator=(const Range& other) noexcept;
        Range(Range&& other) noexcept;
        Range& operator=(Range&& other) noexcept;

        const Source& source() const;

        const Source* src() const { return _source; }

        std::string_view toString() const;

        bool operator==(const Range& other) const {
            return (&_source == &other._source) &&
                   (start == other.start) &&
                   (end == other.end);
        }

        bool operator!=(const Range& other) const { return !(*this == other); }

        Range enclosingLine() const;

        Range rangeAtEnd() const;

        Range merge(const Range& other) const;
        Range extend(const Range& other) const;
        void merge(const Range& other);
        void extend(const Range& other);

        Range sub(uint32_t s, uint32_t len = 0) const;

        uint32_t size() const { return end - start; }

        static const Range& Invalid();

        uint32_t start{0};
        uint32_t end{0};
        LineColumn position{0};

    private:
        const Source* _source{nullptr};
    };

    std::string_view locs(std::string_view str);
    std::string_view locs(std::string&& str);

    template <typename Flags_t>
        requires std::is_enum_v<Flags_t>
    struct Flags {
        static constexpr Flags_t flNone = Flags_t(0);

        Flags() = default;

        Flags(Flags_t flags)
            : value{flags}
        {}

        Flags(uint64_t flags)
            : value{flags}
        {}

        Flags_t value{flNone};

        Flags operator|(Flags flags) const { return Flags_t(value | flags.value); }
        Flags& operator|=(const Flags& flags) { value = Flags_t(value | flags.value); return *this; }

        Flags operator&(Flags flags) const { return Flags_t(value & flags.value); }
        Flags& operator&=(const Flags& flags) { value = Flags_t(value & flags.value); return *this; }

        Flags operator^(Flags flags) const { return Flags_t(value ^ flags.value); }
        Flags& operator^=(const Flags& flags) { value = Flags_t(value ^ flags.value); return *this; }

        Flags operator|(Flags_t flags) const { return Flags_t(value | flags); }
        Flags& operator|=(Flags_t flags) { value = Flags_t(value | flags); return *this; }

        Flags operator&(Flags_t flags) const { return Flags_t(value & flags); }
        Flags operator&=(Flags_t flags) { value = Flags_t(value & flags); return *this; }
        Flags operator^(Flags_t flags) const { return Flags_t(value ^ flags); }
        Flags operator^=(Flags_t flags) { value = Flags_t(value ^ flags); return *this; }

        bool operator==(const Flags& other) const { return value == other.value; }
        bool operator!=(const Flags& other) const { return value != other.value; }
        bool operator||(Flags_t flags) const { return (flags == flNone) || ((value & flags) != 0); }
        bool operator&&(Flags_t flags) const { return (flags == flNone) || ((value & flags) == flags); }
    };

    template <typename T, typename F>
    std::string join(const vec<T>& src, std::string_view sep, F func = nullptr) {
        std::stringstream ss;
        bool first{true};
        for (auto& e: src) {
            if (!first) ss << sep;
            if (func) ss << func(e);
            else ss << e;
            first = false;
        }
        return ss.str();
    }

    typedef enum {
        gflNone,
        gflIsComptime = BIT(0),
        gflIsConstructor = BIT(1),
        gflIsVariadic  = BIT(2),
        gflIsGeneric = BIT(3),
        gflIsExtern    = BIT(4),
        gflIsOverload = BIT(5)
    } GenericFlags_t;

    using GenericFlags = Flags<GenericFlags_t>;
}
