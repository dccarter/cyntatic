/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-04-29
 */

#include "compiler/token.hpp"

#include "compiler/encoding.hpp"
#include "compiler/log.hpp"
#include "compiler/source.hpp"

namespace cstar {

    Token Token::split(Kind thisKind, Kind nextKind)
    {
        csAssert(_range.size() >= 2);
        kind = thisKind;
        auto nextEnd = _range.end;
        auto nextPos = _range.position;
        _range.end = _range.start+1;
        if (_range.source()[_range.end] == '\n') {
            nextPos.line++;
        }
        else {
            nextPos.column++;
        }

        return {nextKind, Range(_range.source(), _range.start + 1, nextEnd, nextPos)};
    }

    bool Token::isComptimeLiteral(Kind kind)
    {
        switch(kind) {
            case NIL: case TRUE: case FALSE: case CHAR:
            case INTEGER: case FLOAT: case STRING:
                return true;
            default: return false;
        }
    }

    std::string_view Token::toString(Token::Kind kind)
    {
        switch(kind) {
#define XX(NAME, STR)  case NAME: return STR;
#define YY(NAME, STR)  case NAME: return STR;
#define ZZ(NAME, STR, _)  case NAME: return STR;
#define BB(NAME, STR)  case NAME: return "@" STR;
            TOKEN_LIST(XX, YY, ZZ, BB)
#undef BB
#undef ZZ
#undef YY
#undef XX
            default: return "<unknown>";
        }
    }

    std::string Token::moveValue()
    {
        csAssert(has<std::string>());
        return std::move(std::get<std::string>(_value));
    }

    bool Token::isBinaryOperator() const
    {
        switch(kind) {
            case PLUS: case MINUS: case MULT: case DIV: case MOD: case EQUAL:
            case NEQ: case LT: case GT: case LTE: case GTE: case LAND: case LOR:
            case BITAND: case BITOR: case BITXOR: case SHL: case SHR: case ASSIGN:
            case PLUSASSGIN: case MINUSASSIGN: case MULTASSIGN: case DIVASSIGN:
            case MODASSIGN: case BITANDASSIGN: case BITORASSIGN: case BITXORASSIGN:
            case SHLASSIGN: case SHRASSIGN:
                return true;
            default: return false;
        }
    }

    bool Token::isUnaryOperator() const
    {
        switch (kind) {
            case PLUS: case MINUS: case NOT: case COMPLEMENT: case MINUSMINUS: case PLUSPLUS:
                return true;
            default: return false;
        }
    }

    bool Token::isTernaryOperator() const
    {
        return kind == QUESTION;
    }

    bool Token::isStatementBoundary() const
    {
        switch(kind) {
#define XX(N) case N:
            STMT_MARK_TOKENS(XX) return true;
#undef XX
            default: return false;
        }
    }

    bool Token::isLogicalOperator(Token::Kind kind)
    {
        switch (kind) {
            case EQUAL: case NEQ: case GT: case GTE: case LT: case LTE:
            case LAND: case LOR:
                return true;
            default:
                return false;
        }
    }
}

std::ostream& operator<<(std::ostream& os, const cstar::Token& tok)
{
    switch (tok.kind) {
        case cstar::Token::INTEGER:
            os << "<integer: " << tok.value<uint64_t>() << ">";
            break;
        case cstar::Token::FLOAT:
            os << "<float: " << tok.value<double>() << ">";
            break;
        case cstar::Token::CHAR:
            os << "<char: ";
            cstar::writeUtf8(os, tok.value<uint32_t>());
            os << ">";
            break;
        case cstar::Token::STRING:
            os << "<string: " << tok.value<std::string>() << ">";
            break;
        case cstar::Token::IDENTIFIER:
            os << "<ident: " << tok.range().toString() << ">";
            break;
        default:
            os << tok.toString();
            break;
    }
    return os;
}