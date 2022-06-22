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

#include "compiler/ccolor.hpp"
#include "compiler/source.hpp"

namespace cstar {

    void Log::append(Diagnostic::Kind kind, Range range, std::string msg)
    {
        _diagnostics.push_back({kind, range, std::move(msg)});
    }

    bool Log::hasErrors() const
    {
        for (const auto& d: _diagnostics) {
            if (d.kind == Diagnostic::ERROR) return true;
        }
        return false;
    }

    std::string Log::toString() const
    {
        std::stringstream ss;
        toString(ss);
        return ss.str();
    }

    void Log::toString(std::ostream &os) const
    {
        for (const auto& d: _diagnostics) {
            os << d;
        }
    }

    void abortCompiler(const Log& L, std::ostream& os)
    {
        if (L.hasErrors()) {
            os << cc::BRED
                << "error: "
              << cc::DEFAULT
                << "compilation failed!\n";
        }

        auto diagnostics = L.diagnostics();
        for (auto& diag: diagnostics) {
            auto& range = diag.range;
            os  << cc::BOLD
                    << range.source().name() << ':'
                    << (range.position.line+1) << ':'
                    << (range.position.column+1) << ": "
                << cc::DEFAULT
                << (diag.kind == Diagnostic::ERROR? cc::RED : cc::YELLOW)
                    << (diag.kind == Diagnostic::ERROR? "error: " : "warning: ")
                << cc::BOLD
                    << diag.message
                << cc::DEFAULT
                    << std::endl
                    << range.enclosingLine().toString()
                    << std::endl
                    << std::string(range.position.column, ' ');

            if (range.size() <= 1) {
                os << '^';
            }
            else {
                auto i = range.start+1;
                os << '^';
                while (i < range.end && range.source()[i++] != '\n') {
                    os << '~';
                }
            }
            os << "\n";
        }

        exit(L.hasErrors()? EXIT_FAILURE: EXIT_SUCCESS);
    }
}

std::ostream& operator<<(std::ostream& os, const cstar::Diagnostic& diagnostic)
{
    auto& range = diagnostic.range;
    os << range.source().name() << ':'
       << (range.position.line+1) << ':'
       << (range.position.column+1) << ": ";

    os << (diagnostic.kind == cstar::Diagnostic::ERROR ? "error: " : "warning: ")
       << diagnostic.message << "\n";

    auto enclosingLine = range.enclosingLine();
    os << enclosingLine.toString() << "\n";

    auto col = range.position.column;
    os << std::string(col, ' ');
    if (range.size() <= 1) {
        os << '^';
    }
    else {
        auto i = range.start;
        while (i < range.end && range.source()[i++] != '\n') {
            os << '~';
        }
    }
    return (os << '\n');
}