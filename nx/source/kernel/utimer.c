// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/utimer.h"
#include "arm/counter.h"

void utimerCreate(UsermodeTimer* t, u64 interval, bool start)
{
    t->next_time = 0;
    t->interval = armNsToTick(interval);

    if (start)
        utimerStart(t);
}

void utimerStart(UsermodeTimer* t)
{
    __sync_bool_compare_and_swap(&t->next_time, 0, armGetSystemTick() + armNsToTick(t->interval));
}

void utimerStop(UsermodeTimer* t)
{
    while (__sync_bool_compare_and_swap(&t->next_time, t->next_time, 0));
}

void _utimerRecalculate(UsermodeTimer* t, u64 old_time)
{
    s64 interval = t->interval;
    s64 new_time = ((armGetSystemTick() - old_time + interval - 1) / interval) * interval;
    __sync_bool_compare_and_swap(&t->next_time, old_time, new_time);
}

u64 _utimerGetNextTime(UsermodeTimer* t)
{
    return t->next_time;
}
