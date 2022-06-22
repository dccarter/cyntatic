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

#include <compiler/utils.hpp>
#include <compiler/token.hpp>

#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <vector>


namespace cstar {

    struct Diagnostic {
        typedef enum {
            ERROR,
            WARNING
        } Kind;

        Kind kind{WARNING};
        Range range{};
        std::string message{};
    };

    class Log {
    public:
        void error(Range range, std::string msg) {
            append(Diagnostic::ERROR, range, std::move(msg));
        }

        template<class... Args>
        void error(Range range, Args... args) {
            error(range, format(std::forward<Args>(args)...));
        }

        void warning(Range range, std::string msg) {
            append(Diagnostic::WARNING, range, std::move(msg));
        }

        template<class... Args>
        void warning(Range range, Args... args) {
            error(range, format(std::forward<Args>(args)...));
        }

        bool hasErrors() const;

        std::string toString() const;

        void toString(std::ostream& os) const;

        const std::vector<Diagnostic>& diagnostics() const { return _diagnostics; }

    private:
        template<typename... Args>
        std::string format(Args... args) const
        {
            // efficient stream
            std::stringstream ss;
            (ss << ... << args);
            return ss.str();
        }

        void append(Diagnostic::Kind kind, Range range, std::string msg);

        std::vector<Diagnostic> _diagnostics{};
    };

    [[noreturn]]
    void abortCompiler(const Log& L, std::ostream& os);

    [[noreturn]]
    inline void abortCompiler(const Log& L) {
        abortCompiler(L, L.hasErrors() ? std::cerr : std::cout);
    }

    template <typename... A>
    void csAbort(const char *file, int line, const char* cond, const A&... args)
    {
        std::cerr << file << ":" << line <<  ": Assert (" << cond << ")\n";
        if constexpr (sizeof...(args) != 0) {
            std::cerr << "\t";
            (std::cerr << ... << args) << "\n";
        }
        abort();
    }

#define csAssert(cond, ...) if (!(cond)) csAbort(__FILE__, __LINE__, #cond, ##__VA_ARGS__)
}

std::ostream& operator<<(std::ostream& os, const cstar::Diagnostic& diagnostic);