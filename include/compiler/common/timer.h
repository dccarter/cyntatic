/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef u32 TimerId;
void Timer_init(void);
TimerId Timer_add(bool start);
void Timer_start(TimerId id);
u64  Timer_stop(TimerId id);
u64  Timer_elapsed(TimerId id);
u64  Timer_now(void);

#ifdef __cplusplus
}
#endif

