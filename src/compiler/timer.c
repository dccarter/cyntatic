/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-27
 */

#include "compiler/timer.h"
#include "compiler/heap.h"

#include "vector.h"

#include <time.h>
#include <sys/time.h>

typedef struct {
    u64 start;
    u64 end;
} Timer;

static struct {
    bool init;
    Vector(Timer) timers;
} sTimers = {.init = false };

void Timer_init(void)
{
    if (sTimers.init) return;
    sTimers.init = true;
    Vector_initWith(&sTimers.timers, ArenaAllocator);
}

TimerId Timer_add(bool start)
{
    u32 id = Vector_len(&sTimers.timers);
    Timer *timer = Vector_expand(&sTimers.timers, 1);
    if (start)
        timer->start = Timer_now();
    else
        timer->start = 0;
    timer->end = 0;

    return id;
}

void Timer_start(TimerId id)
{
    Timer *timer;
    csAssert(id < Vector_len(&sTimers.timers), "Timer id out of range");

    timer = Vector_at(&sTimers.timers, id);
    csAssert0(timer->start == 0);
    timer->start = Timer_now();
}

u64  Timer_stop(TimerId id)
{
    Timer *timer;
    csAssert(id < Vector_len(&sTimers.timers), "Timer id out of range");

    timer = Vector_at(&sTimers.timers, id);
    csAssert0(timer->start != 0 && timer->end == 0);

    timer->end = Timer_now();
    return timer->end - timer->start;
}

u64  Timer_elapsed(TimerId id)
{
    Timer *timer;
    csAssert(id < Vector_len(&sTimers.timers), "Timer id out of range");

    timer = Vector_at(&sTimers.timers, id);
    csAssert0(timer->start != 0);

    if (timer->end == 0)
        return Timer_now() - timer->start;

    return timer->end = timer->start;
}

u64  Timer_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}