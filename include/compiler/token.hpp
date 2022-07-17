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

#include <list>
#include <string>
#include <string_view>
#include <variant>

#define TOKEN_LIST(XX, YY, ZZ, BB) \
    XX(EoF,                             "<eof>") \
    XX(Nl,                              "<nl>")         \
    XX(CHAR,                            "<char>")       \
    XX(STRING,                          "<string>")     \
    XX(INTEGER,                         "<integer>")    \
    XX(FLOAT,                           "<float>")      \
    XX(IDENTIFIER,                      "<identifier>") \
    XX(COMMENT,                         "<comment>")    \
    \
    XX(ASSIGN,                          "'='")       \
    XX(BITAND,                          "'&'")       \
    XX(BITANDASSIGN,                    "'&='")      \
    XX(BITOR,                           "'|'")       \
    XX(BITORASSIGN,                     "'|='")      \
    XX(BITXOR,                          "'^'")       \
    XX(BITXORASSIGN,                    "'^='")      \
    XX(COLON,                           "':'")       \
    XX(DCOLON,                          "'::'")      \
    XX(COMMA,                           "','")       \
    XX(COMPLEMENT,                      "'~'")       \
    XX(COMPASSIGN,                      "'~='")      \
    XX(DIV,                             "'/'")       \
    XX(DOT,                             "'.'")       \
    XX(DOTDOT,                          "'..'")      \
    XX(ELIPSIS,                         "'...")      \
    XX(DIVASSIGN,                       "'/='")      \
    XX(EQUAL,                           "'=='")      \
    XX(GT,                              "'>'")       \
    XX(GTE,                             "'>='")      \
    XX(LT,                              "'<'")       \
    XX(LTE,                             "'<='")      \
    XX(LBRACE,                          "'{'")       \
    XX(RBRACE,                          "'}'")       \
    XX(LBRACKET,                        "'['")       \
    XX(RBRACKET,                        "']'")       \
    XX(LPAREN,                          "'('")       \
    XX(RPAREN,                          "')'")       \
    XX(LAND,                            "'&&'")     \
    XX(LOR,                             "'||'")      \
    XX(MINUS,                           "'-'")       \
    XX(MINUSMINUS,                      "'--'")      \
    XX(MINUSASSIGN,                     "'-='")      \
    XX(MULT,                            "'*'")       \
    XX(EXPONENT,                        "'**'")      \
    XX(MULTASSIGN,                      "'*='")      \
    XX(NOT,                             "'!'")       \
    XX(NEQ,                             "'!=')")     \
    XX(PLUS,                            "'+'")       \
    XX(PLUSPLUS,                        "'++'")      \
    XX(PLUSASSGIN,                      "'+='")      \
    XX(MOD,                             "'%'")       \
    XX(MODASSIGN,                       "'%='")      \
    XX(QUESTION,                        "'?'")       \
    XX(SEMICOLON,                       "';'")       \
    XX(SHL,                             "'<<'")      \
    XX(SHLASSIGN,                       "'=<<'")     \
    XX(SHR,                             "'>>'")      \
    XX(SHRASSIGN,                       "'>>='")     \
    XX(LARROW,                          "'<-'")      \
    XX(RARROW,                          "'->'")      \
    XX(AT,                              "'@'")       \
    XX(HASH,                            "'#'")       \
    XX(BACKQUOTE,                       "'`")       \
    XX(DOLLAR,                          "'$'")      \
    \
    YY(ALIGINOF,                        "alignof") \
    YY(AS,                              "as")      \
    YY(AUTO,                            "auto")    \
    YY(BREAK,                           "break")   \
    YY(CASE,                            "case")    \
    YY(CONTINUE,                        "continue")\
    YY(CONST,                           "const")   \
    YY(ELSE,                            "else")    \
    YY(ENUM,                            "enum")    \
    YY(EXTERN,                          "extern")  \
    YY(FALSE,                           "false")   \
    YY(FOR,                             "for")    \
    YY(FUNC,                            "func")    \
    YY(IF,                              "if")      \
    YY(IMM,                             "iu")     \
    YY(IN,                              "in")      \
    YY(INLINE,                          "inline")  \
    YY(IMPORT,                          "import")  \
    YY(MACRO,                           "macro")   \
    YY(MOVE,                            "@move")   \
    YY(MUT,                             "mut")     \
    YY(NEW,                             "new")     \
    YY(NIL,                             "null")    \
    YY(RETURN,                          "return")  \
    YY(SIZEOF,                          "sizeof")  \
    YY(STATIC,                          "static")  \
    YY(STRUCT,                          "struct")  \
    YY(SWITCH,                          "switch")  \
    YY(THIS,                            "this")    \
    YY(TRAIT,                           "trait")   \
    YY(TRUE,                            "true")    \
    YY(UNSAFE,                          "unsafe")  \
    YY(UNION,                           "union")   \
    YY(USING,                           "using")   \
    YY(WHILE,                           "while")   \
    YY(VOID,                            "void")    \
    BB(LINE,                            "line")    \
    BB(COLUMN,                          "column")  \
    BB(FILEEXPR,                        "file")    \
    BB(ARGEXPR,                         "arg")     \
    BB(OPAQUE,                          "opaque")  \
    \
    ZZ(AND,                             "and", LAND) \
    ZZ(OR,                              "or",  LOR)  \
    \
    XX(LSTREXPR,                        "'>strexpr'")\
    XX(RSTREXPR,                        "'<strexpr'")\

#define STMT_MARK_TOKENS(XX) \
        XX(BREAK)       \
        XX(CASE)        \
        XX(CONTINUE)    \
        XX(ELSE)        \
        XX(ENUM)        \
        XX(EXTERN)      \
        XX(FOR)         \
        XX(FUNC)        \
        XX(IF)          \
        XX(IMM)         \
        XX(INLINE)      \
        XX(IMPORT)      \
        XX(MACRO)       \
        XX(RETURN)      \
        XX(STATIC)      \
        XX(STRUCT)      \
        XX(SWITCH)      \
        XX(TRAIT)       \
        XX(UNION)       \
        XX(USING)       \
        XX(WHILE)       \


namespace cyn {

    using TokenValue = std::variant<std::monostate, bool, std::uint32_t, std::uint64_t, double, std::string>;
    class Token {
    public:
        typedef enum {
#define XX(NAME, _) NAME,
#define ZZ(NAME, ...) NAME,
#define YY(NAME, _) NAME,
#define BB(NAME, _) NAME,
            TOKEN_LIST(XX, YY, ZZ, BB)
#undef YY
#undef ZZ
#undef XX
#undef BB
        } Kind;

        using Ref = std::list<Token>::iterator;
        using Tange = std::pair<Ref, Ref>;

    public:
        Token() = default;
        Token(Kind kind, Range range)
            : kind{kind},
              _range{range}
        {}

        template <typename T>
        Token(Kind kind, Range range, T value)
            : kind{kind},
              _range{range},
              _value{std::forward<T>(value)}
        {}

        Token split(Kind first, Kind second);

        std::string_view toString() const { return toString(kind); }

        bool isKeyword() const { return isKeyword(kind); }

        const Range& range() const { return _range; }

        static bool isKeyword(Kind kind) {
            return (kind >= ALIGINOF) and (kind <= VOID);
        }

        static bool isComptimeLiteral(Kind kind);

        bool isComptimeLiteral() const {return isComptimeLiteral(kind); }

        static std::string_view toString(Kind kind);

        template <typename T>
        const T& value() const {
            return get<T>(_value);
        }

        template <typename T>
        bool has() const {
           return holds_alternative<T>(_value);
        }

        std::string moveValue();

        bool isBinaryOperator() const;
        bool isUnaryOperator() const;
        bool isTernaryOperator() const;
        bool isStatementBoundary() const;
        static bool isLogicalOperator(Token::Kind kind);
    public:
        Kind kind{EoF};

    private:
        friend class Lexer;
        Range _range{};
        TokenValue _value{};
    };
}

std::ostream& operator<<(std::ostream& os, const cyn::Token& tok);