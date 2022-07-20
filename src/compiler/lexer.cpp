/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-04-29
 */

#include "compiler/lexer.hpp"

#include "encoding.hpp"
#include "compiler/source.hpp"

#include <map>
#include <charconv>

namespace {
    inline bool isoct(char c) { return '0' <= c && c <= '7'; }

    bool isValidUcn(uint32_t c) {
        if (0xD800 <= c and c <= 0xDFFF)
            return false;
        return 0xA0 <= c || c == '$' || c == '@' || c == '`';
    }
}

namespace cyn {

    Token Lexer::mkToken(Token::Kind kind, uint32_t start, uint32_t end)
    {
        return Token{kind, Range{_src, start, end, _pos}};
    }

    Token& Lexer::addToken(Token::Kind kind, Position pos, uint32_t end)
    {
        return _tokens.emplace_back(kind, Range{_src, pos, end});
    }

    void Lexer::eatWhile(char c)
    {
        eatWhileFunc([c](char x) { return x == c; });
    }

    void Lexer::eatUntil(char c)
    {
        eatUntilFunc([c](char x) { return x != c; });
    }

    void Lexer::eatUntilFunc(std::function<bool(char)> func)
    {
        auto limit = _src.size();
        while (_idx < limit and !func(_src[_idx])) {
            if (_src[_idx] == '\n') {
                _pos.line++;
                _pos.column = 0;
            }
            else {
                _pos.column++;
            }
            _idx++;
        }
    }

    void Lexer::eatWhileFunc(std::function<bool(char)> func)
    {
        auto limit = _src.size();
        while (_idx < limit and func(_src[_idx])) {
            if (_src[_idx] == '\n') {
                _pos.line++;
                _pos.column = 0;
            }
            else {
                _pos.column++;
            }
            _idx++;
        }
    }

    void Lexer::eatWhitespace()
    {
        eatWhileFunc([](char c) { return isspace(c); });
    }

    char Lexer::peek(uint32_t n)
    {
        n += _idx;
        if (n < _src.size()) {
            return _src[n];
        }
        return EOF;
    }

    uint32_t Lexer::advance(uint32_t n)
    {
        auto ret = _idx;
        _idx = std::min(uint32_t(_src.size()), (_idx + n));
        for (auto i = ret; i < _idx; i++) {
            if (_src[i] == '\n') {
                _pos.column = 0;
                _pos.line ++;
            }
            else {
                _pos.column++;
            }
        }

        return ret;
    }

    bool Lexer::tokenize()
    {
        auto limit = _src.size();
        auto& code = _src.contents();

        while (_idx < limit) {
            auto c = code[_idx];
            if (std::isspace(c)) {
                auto m = _pos;
                auto idx = _idx;
                eatWhitespace();
                if (m.line < _pos.line)
                    addToken(Token::Nl, {idx, m}, idx + 1);
                continue;
            }

            if (!tokenize(c)) {
                return false;
            }
        }

        return true;
    }

    bool Lexer::tokenize(char c)
    {
        auto cc = peek(1);
        auto ccc = peek(2);
        auto pos = mark();

#define IF(C, T) if (cc == (C)) addToken((T), pos, advance(2));
#define IF_ELSE(C, T, E) if (cc == (C)) addToken((T), pos, advance(2)); else addToken((E), pos, advance())
#define IF_ELSE2(CC, CCC, T, E, EE) if (cc == (CC)) { if (ccc == (CCC)) { advance(3); addToken((T), pos, advance()); } else { addToken((E), pos, advance(2)); }} else addToken((EE), pos, advance())
#define ADD(T) addToken((T), pos, advance());

        switch(c) {
            case '%':
                IF_ELSE('=', Token::MODASSIGN, Token::MOD);
                break;

            case '/':
                IF('=', Token::DIVASSIGN)
                else if (cc == '*' or cc == '/') {
                    tokComment();
                }
                else {
                    ADD(Token::DIV);
                }
                break;
            case '*':
                IF('*', Token::EXPONENT)
                else IF_ELSE('=', Token::MULTASSIGN, Token::MULT);
                break;
            case '+':
                IF('+', Token::PLUSPLUS)
                else IF_ELSE('=', Token::PLUSASSGIN, Token::PLUS);
                break;
            case '-':
                IF('-', Token::MINUSMINUS)
                else IF('>', Token::RARROW)
                else IF_ELSE('=', Token::MINUSASSIGN, Token::MINUS);
                break;
            case '|':
                IF('|', Token::LOR)
                else IF_ELSE('=', Token::BITORASSIGN, Token::BITOR);
                break;
            case '&':
                IF('&', Token::LAND)
                else IF_ELSE('=', Token::BITANDASSIGN, Token::BITAND);
                break;
            case '^':
                IF_ELSE('=', Token::BITXORASSIGN, Token::BITXOR);
                break;
            case '~':
                IF_ELSE('=', Token::COMPASSIGN, Token::COMPLEMENT);
                break;
            case '>':
                IF_ELSE2('>', '=', Token::SHRASSIGN, Token::SHR, Token::GT);
                break;
            case '<':
                IF ('-', Token::LARROW)
                else IF_ELSE2('<', '=', Token::SHLASSIGN, Token::SHL, Token::LT);
                break;
            case '=':
                IF_ELSE('=', Token::EQUAL, Token::ASSIGN);
                break;
            case '.':
                if (isdigit(cc))
                    // there are instance floating point numbers will be typed starting with .
                    tokNumber();
                else IF_ELSE2('.', '.', Token::ELIPSIS, Token::DOTDOT, Token::DOT);
                break;
            case ':':
                IF_ELSE(':', Token::DCOLON, Token::COLON);
                break;
            case '!':
                ADD(Token::NOT);
                break;
            case '{':
                ADD(Token::LBRACE);
                break;
            case '}':
                if (_inStrExpr) {
                    advance();
                    tokString();
                }
                else {
                    ADD(Token::RBRACE);
                }
                break;
            case '[':
                ADD(Token::LBRACKET);
                break;
            case ']':
                ADD(Token::RBRACKET);
                break;
            case '(':
                ADD(Token::LPAREN);
                break;
            case ')':
                ADD(Token::RPAREN);
                break;
            case '@':
                ADD(Token::AT);
                break;
            case '#':
                ADD(Token::HASH);
                break;
            case '?':
                ADD(Token::QUESTION);
                break;
            case '`':
                ADD(Token::BACKQUOTE);
                break;
            case ';':
                ADD(Token::SEMICOLON);
                break;
            case ',':
                ADD(Token::COMMA);
                break;
            case '$':
                ADD(Token::DOLLAR);
                break;
            case '\'':
                tokCharacter();
                break;
            case '"':
                advance();
                tokString();
                break;
            case 'f':
                if (cc == '"') {
                    _inStrExpr = true;
                    advance(2);
                    addToken(Token::LSTREXPR, pos, _idx);
                    tokString();
                }
                else {
                    tokIdentifier();
                }
                break;
            case '0' ... '9':
                tokNumber();
                break;
            case '_':
            case 'a' ... 'e':
            case 'g' ... 'z':
            case 'A' ... 'Z':
                tokIdentifier();
                break;
            default:
                L.error({_src, pos, _idx}, "unknown token");
                advance();
                return false;
        }

        return true;
    }

    uint32_t Lexer::tokEscapedChar()
    {
        auto c = peek();
        advance();
        switch (c) {
            case '\'': case '"': case '?': case '\\': case '$': return c;
            case 'a': return '\a';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            case 'v': return '\v';
            case 'e': return '\033';
            case 'x': return tokHexChar();
            case 'u': return tokUniversalChar(4);
            case 'U': return tokUniversalChar(8);
            case '0' ... '7': return tokOctalChar(c);
            default:
                L.warning({_src, _idx, _idx, _pos}, "unknown escape character: \\", c);
                advance();
                return c;
        }
    }

    uint32_t Lexer::tokOctalChar(char c)
    {
        int r = c - '0';

        c = peek();
        if (!isoct(c)) {
            return r;
        }
        advance();
        r = (r << 3) | (c - '0');

        c = peek();
        if (!isoct(c)) {
            return r;
        }
        advance();
        return (r << 3) | (c - '0');
    }

    uint32_t Lexer::tokHexChar()
    {
        auto c = peek();
        if (!isxdigit(c)) {
            L.error({_src, _idx-1, _idx, _pos}, "\\x is not followed by a hexadecimal literal");
            abortCompiler(L);
        }

        uint32_t r = 0;
        for (; c != EOF; c = peek()) {
            switch(c) {
                case '0' ... '9': r  = (r << 4) | (c - '0'); break;
                case 'a' ... 'f': r  = (r << 4) | (c - 'a' + 10); break;
                case 'A' ... 'F': r  = (r << 4) | (c - 'A' + 10); break;
                default: return r;
            }
            advance();
        }

        return r;
    }

    uint32_t Lexer::tokUniversalChar(uint32_t len)
    {
        uint32_t r = 0;
        auto start = _idx - 1;
        for (auto i = 0u; i < len; i++) {
            auto c = peek();
            switch (c) {
                case '0' ... '9': r  = (r << 4) | (c - '0'); break;
                case 'a' ... 'f': r  = (r << 4) | (c - 'a' + 10); break;
                case 'A' ... 'F': r  = (r << 4) | (c - 'A' + 10); break;
                default: {
                    L.error({_src, start, _idx, _pos}, "invalid character character: ", c);
                    abortCompiler(L);
                }
            }
            advance();
        }

        if (!isValidUcn(r)) {
            L.error({_src, start, _idx, _pos}, "invalid character character");
            abortCompiler(L);
        }
        return r;
    }

    void Lexer::tokCharacter()
    {
        auto pos = mark();
        auto c = peek(1);
        auto chr = uint32_t((uint8_t)c & 0xFF);

        advance(2);

        if (c == '\\') {
            chr = tokEscapedChar();
        }

        if (chr >= 0x80) {
            while (c != '\'') {
                advance();
                c = peek();
            }

            chr  = readRune(L, {_src, pos.index+1, _idx, pos.coord}).second;
        }

        if (peek() != '\'') {
            // unterminated character literal
            L.error({_src, pos, _idx}, "unterminated character sequence");
        }
        else {
            advance();
            addToken(Token::CHAR, pos, _idx)._value = chr;
        }
    }

    void Lexer::tokString()
    {
        auto pos = mark();
        std::stringstream ss;
        auto c = peek();
        bool inStrExpr = _inStrExpr;

        for (;c != EOF; c = peek()) {
            if (c == '"') {
                _inStrExpr = false;
                break;
            }

            if (c == '\n') {
                break;
            }

            advance();
            auto cc = peek();
            if (_inStrExpr and c == '$' and cc == '{') {
                break;
            }

            if (c != '\\') {
                ss << c;
                continue;
            }

            auto ucn = (cc == 'u' or cc == 'U');
            auto es = _idx;
            auto chr = tokEscapedChar();
            if (ucn) {
                writeUtf8(ss, &L, {_src, es, _idx, _pos}, chr);
            }
            else {
                writeChar(ss, char(chr));
            }
        }

        if (!_inStrExpr and c != '"') {
            L.error({_src, pos, _idx}, "unterminated string literal");
        }
        else {
            advance();
            if (!inStrExpr or (_idx - pos.index) > 1)
                addToken(Token::STRING, pos, _idx)._value = ss.str();

            if (inStrExpr && c == '"')
                addToken(Token::RSTREXPR, mark(), _idx);
        }
    }

    void Lexer::tokBinaryNumber()
    {
        auto pos = mark();
        advance(2);
        auto c = peek();
        for (; (c == '0' or c == '1'); c = peek())
            advance();

        if (isdigit(c)) {
            auto p = mark();
            eatWhileFunc(isdigit);
            L.error({_src, p, _idx}, "invalid digit in a binary number '", c, "'");
        }
        else {
            parseInteger(pos, 2);
        }
    }

    void Lexer::tokHexNumber()
    {
        auto pos = mark();
        advance(2);
        auto c = peek();
        for (; isxdigit(c); c = peek())
            advance();

        if (toupper(c) == 'P') {
            tokFloatingPoint(pos);
        }
        else {
            parseInteger(pos, 16);
        }
    }

    void Lexer::tokOctalNumber()
    {
        auto pos = mark();
        auto c = peek();
        for (; isoct(c); c = peek())
            advance();

        auto x = peek();
        if (isdigit(x)) {
            // could be a floating point number, try to jump to a '.' or an 'E'
            for (; isdigit(c); c = peek())
                advance();

            if (c == '.' or c == 'e' or c == 'E') {
                tokFloatingPoint(pos);
            }
            else {
                L.error({_src, pos, _idx}, "'", x, "' is not a valid octal digit");
            }
        }
        else {
            parseInteger(pos, 8);
        }
    }

    void Lexer::tokDecimalNumber()
    {
        auto pos = mark();
        auto c = peek();
        for (; isdigit(c); c = peek())
            advance();
        auto C = toupper(c);
        if (C == 'E' or c == '.') {
            // this is possibly a floating point number
            tokFloatingPoint(pos);
        }
        else {
            parseInteger(pos, 10);
        }
    }

    void Lexer::parseInteger(const Position &start, int base)
    {
        std::uint64_t value{0};
        auto s = (base == 16 || base == 2)? _src.at(start.index + 2) : _src.at(start.index);

        auto [_, ec] = std::from_chars(s, _src.at(_idx), value, base);
        if (ec == std::errc::result_out_of_range) {
            L.error({_src, start, _idx}, "number too big parse");
        }
        else {
            addToken(Token::INTEGER, start, _idx)._value = value;
        }
    }

    void Lexer::tokFloatingPoint(const Position &start)
    {
        auto c = char(toupper(peek()));
        csAssert(c == '.' || c == 'E' or c == 'P');
        advance();

        if (c == 'E' or c == 'P') {
            c = peek();
            if (c == '-' or c == '+') {
                advance();
            }
            if (!isdigit(peek())) {
                L.error({_src, start, _idx}, "'", c, "' exponent has no digits");
                return;
            }
        }
        eatWhileFunc(isdigit);

        double value{0};
        auto [ptr, ec] = std::from_chars(_src.at(start.index), _src.at(_idx), value);
        if (ec == std::errc::result_out_of_range) {
            L.error({_src, start, _idx}, "number too big parse: ", (ptr - _src.at(start.index)));
        }
        else {
            addToken(Token::FLOAT, start, _idx)._value = value;
        }
    }

    void Lexer::tokNumber()
    {
        auto c = peek(), cc = char(toupper(peek(1))), ccc = peek(2);
        if (c == '0') {
            if (cc == 'X' and isxdigit(ccc)) {
                tokHexNumber();
            }
            else if (cc == 'B' and (ccc == '0' or ccc == '1')) {
                tokBinaryNumber();
            }
            else if (cc == '.' or cc == 'E') {
                auto pos = mark();
                advance();
                tokFloatingPoint(pos);
            }
            else {
                tokOctalNumber();
            }
        }
        else {
            tokDecimalNumber();
        }
    }

    void Lexer::tokIdentifier()
    {
#define YY(TOK, NAME) {std::string_view{NAME}, cyn::Token:: TOK},
#define ZZ(TOK, NAME, ALIAS) {std::string_view{NAME}, cyn::Token:: ALIAS},
#define XX(_, ___)
#define BB(TOK, NAME) {std::string_view{NAME}, cyn::Token:: TOK},
        static const std::map<std::string_view, Token::Kind> KeyWords({
            TOKEN_LIST(XX, YY, ZZ, BB)
        });
#undef BB
#undef XX
#undef ZZ
#undef YY
        auto pos = mark();
        auto c = peek();

        // consume all letters that can be an identifier
        for (; (c == '_' or c == '$' or isdigit(c) or std::isalnum(c)); c = peek())
            advance();

        auto range = Range{_src, pos, _idx};
        auto sv = range.toString();
        auto it = KeyWords.find(sv);
        if (it != KeyWords.end()) {
            // this is a keyword
            auto& tok = addToken(it->second, pos, _idx);
            if (tok.kind == Token::FALSE or tok.kind == Token::TRUE) {
                tok._value = tok.kind == Token::TRUE;
            }
        }
        else {
            // this is an identifier
            auto& tok = addToken(Token::IDENTIFIER, pos, _idx);
            tok._value = false;
        }
    }

    void Lexer::tokComment()
    {
        auto pos = mark();
        advance();
        auto c = peek();
        auto isMultiLine = c == '*';
        auto level = 1;
        advance();

        while (c != EOF) {
            c = peek();
            advance();

            if (c == '\n' and !isMultiLine) {
                break;
            }

            auto cc = peek();
            if (c == '*' and cc == '/') {
                advance();
                if (--level == 0)
                    break;
            }
            if (c == '/' and cc == '*')
                level++;
        }

        if (isMultiLine && level != 0) {
            L.error({_src, pos, _idx}, "unterminated multiline comment");
        }
        else {
            // the token value will indicate whether the comment is multiline or not
            addToken(Token::COMMENT, pos, _idx)._value = isMultiLine;
        }
    }
}