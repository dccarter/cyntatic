/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-07-23
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct CommandLineCommand;
int cmdAssemble(struct CommandLineCommand *cmd);
int cmdDisAssemble(struct CommandLineCommand* cmd);

#ifdef __cplusplus
}
#endif