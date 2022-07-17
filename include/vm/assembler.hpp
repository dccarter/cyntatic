/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-16
 */

#pragma once

#include <compiler/lexer.hpp>
#include <vm/vm.h>

#include <stack>
#include <unordered_set>
#include <map>
#include <set>

namespace cyn {

    class Assembler {
        struct Symbol {
            u32 id;
            enum {
                symLabel,
                symVar
            } tag;
            Range range{};
        };

        class ParseError : public std::exception {
        public:
            using std::exception::exception;
        };

    public:
        Assembler(Log& L, Token::Tange tange)
            : L{L}, _tokens{std::move(tange)}
        { _current = _tokens.first; }

        u32 assemble(Code& code);

    private:
        void parseLabel();
        void parseInstruction();
        void parseVarDcl();
        u32 addSymbolRef(u32 pos, std::string_view name, const Range& range);
        Symbol& findSymbol(std::string_view name, const Range& range);
        std::pair<bool, Register> parseInstructionArg(Instruction& instr);

        void pushSourceFile(std::string_view path);

        template<typename ...K>
        bool check(Token::Kind kind, K... kinds) {
            if (EoT()) return false;
            return checkKind(kind, std::forward<K>(kinds)...);
        }

        template<typename ...K>
        bool match(Token::Kind kind, K... kinds) {
            if (check(kind, std::forward<K>(kinds)...)) {
                advance();
                return true;
            }
            return false;
        }

        template<typename ...K>
        bool checkKind(Token::Kind kind, K... kinds) {
            if (kind == current()->kind) return true;
            else {
                if constexpr (sizeof...(kinds) > 0) {
                    return checkKind(std::forward<K>(kinds)...);
                }
                return false;
            }
        }

        bool EoT();

        Token::Ref advance();
        Token::Ref current() { return _current; }
        Token::Ref peek();
        Token::Ref previous();
        void synchronize();

        Token::Ref save() { return _current; }
        void restore(Token::Ref ref) { _current = ref; }

        template <typename... Args>
        Token::Ref consume(Token::Kind kind, Args... args)
        {
            if (check(kind)) return advance();
            fail(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(Args... args)
        {
            L.error(current()->range(), std::forward<Args>(args)...);
        }

        template <typename... Args>
        [[noreturn]] void fail(Args... args)
        {
            L.error(current()->range(), std::forward<Args>(args)...);
            throw ParseError();
        }

        std::unordered_map<std::string_view, Symbol> _symbols{};
        std::unordered_map<u32, std::pair<std::string_view, Range>> _patchWork{};
        std::unordered_map<u32, std::unordered_set<u32>> _symRefs{};

        std::vector<Instruction> _instructions{};
        std::vector<u8> _constants{};
        u32 _main{0};
        u32 _ip{0};
        Log& L;
        Token::Ref _current{};
        Token::Tange _tokens{};
    };
}