/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */

#pragma once

#include <common.h>

#define View(T) struct { const T* data; u32 count; }

typedef View(char) StringView;

#define View_init(self, DATA, COUNT) ((self)->data = (DATA), (self)->count = (COUNT))
#define View_at(self, I) \
    ({ __typeof__(I) LineVAR(idx) = (I); (LineVAR(idx) < (self)->count? &(self)->data[LineVAR(idx)]: NULL); })


