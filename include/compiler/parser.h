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

#include <compiler/ast.h>
#include <compiler/common/lexer.h>

#ifdef __cplusplus
extern "C" {
#endif

AstModule *parse(Lexer *lX);

#ifdef __cplusplus
}
#endif