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

#include "compiler/log.hpp"
#include "compiler/token.hpp"

#include <functional>
#include <list>
#include <optional>

namespace cstar {

    class Lexer {
    public:
        Lexer(Log& log, Source& src)
            : L{log}, _src{src}
        {}

        std::list<Token>& tokens() { return _tokens; }
        Token::Tange tange() { return {_tokens.begin(), _tokens.end()}; }
        bool tokenize();

    private:
        bool tokenize(char c);
        void eatWhile(char c);
        void eatWhileFunc(std::function<bool(char)> func);
        void eatUntil(char c);
        void eatUntilFunc(std::function<bool(char)> func);
        void eatWhitespace();
        char peek(uint32_t n = 0);
        uint32_t advance(uint32_t n = 1);
        void tokCharacter();
        void tokNumber();
        void tokHexNumber();
        void tokBinaryNumber();
        void tokFloatingPoint(const Position& start);
        void tokDecimalNumber();
        void tokOctalNumber();
        void tokString();
        void tokIdentifier();
        void tokComment();
        uint32_t tokEscapedChar();
        uint32_t tokHexChar();
        uint32_t tokOctalChar(char c);
        uint32_t tokUniversalChar(uint32_t len);
        void parseInteger(const Position& start, int base);

        Token mkToken(Token::Kind kind, uint32_t start, uint32_t end);
        Token& addToken(Token::Kind kind, Position pos, uint32_t end);

        inline Position mark() { return {_idx, _pos}; }

        std::list<Token> _tokens{};
        bool _inStrExpr{false};
        LineColumn  _pos{};
        uint32_t    _idx{};
        Source&     _src;
        Log&        L;
    };
}