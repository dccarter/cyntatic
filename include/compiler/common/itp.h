/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-08-12
 */

#pragma once

#include <compiler/common/lexer.h>
#include <e4c.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Lexer *lX;
    Token curr;
    Token peek;
    Token prev;
    struct Log_t  *L;
} ITokenParser;

void ITokenParser_init(ITokenParser *itp, Lexer *lX);
#define ITP_init(itp, lX) ITokenParser_init(ITP(itp), lX)

Token* ITokenParser_advance(ITokenParser *itp);
#define ITP_advance(itp) ITokenParser_advance(ITP(itp))

Token* ITokenParser_peek(ITokenParser *itp);
#define ITP_peek(itp) ITokenParser_peek(ITP(itp))

bool ITokenParser_match_(ITokenParser *itp, const TokenKind *kinds, u32 count);
#define ITP_match_(itp, kinds, count) ITokenParser_match_(ITP(itp), (kinds), (count))

bool ITokenParser_check_(ITokenParser *itp, const TokenKind *kinds, u32 count);
#define ITP_check_(itp, kinds, count) ITokenParser_check_(ITP(itp), (kinds), (count))

#define ITP(p) ((ITokenParser *)(p))

E4C_DECLARE_EXCEPTION(ITPException);

#define ITP_prev(itp) (&ITP(itp)->prev)
#define ITP_curr(itp) (&ITP(itp)->curr)

#define ITP_eof(itp)   (ITP(itp)->curr.kind == tokEoF ||ITP(itp)->curr.kind == tokCOUNT)
#define ITP_match__(itp, KINDS)   ITP_match_((itp), (KINDS), sizeof__(KINDS))
#define ITP_match(itp, KIND, ...) ITP_match__((itp), make(TokenKind[], (KIND), ##__VA_ARGS__))

#define ITP_check__(itp, KINDS) ITP_check_((itp), (KINDS), sizeof__(KINDS))
#define ITP_check(itp, KIND, ...)                              \
    ITP_check__((itp), make(TokenKind[], (KIND), ##__VA_ARGS__))

#define ITP_error0(itp, RNG, ...) Log_error(ITP(itp)->L, (RNG), ##__VA_ARGS__)
#define ITP_error(itp, ...)       Log_error(ITP(itp)->L, &ITP_curr(itp)->range, ##__VA_ARGS__)

#define ITP_fail(itp, ...)                                                 \
    ({ ITP_error((itp), ##__VA_ARGS__); E4C_THROW(ITPException, ""); unreachable(); })

#define ITP_fail0(itp, RNG, ...)                                                 \
    ({ ITP_error0((itp), (RNG), ##__VA_ARGS__); E4C_THROW(ITPException, ""); unreachable(); })

#define ITP_consume(itp, KIND, ...)  ({                 \
        Token *LineVAR(iAs) = NULL;                     \
        do {                                            \
                                                        \
            if (ITP_check((itp), (KIND)))               \
                LineVAR(iAs) = ITP_advance((itp));      \
            else                                        \
                ITP_fail((itp), ##__VA_ARGS__);         \
        } while(0);                                     \
        LineVAR(iAs);                                   \
    })

#ifdef __cplusplus
}
#endif
