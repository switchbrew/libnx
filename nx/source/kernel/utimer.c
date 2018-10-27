// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/utimer.h"
#include "arm/counter.h"

void utimerCreate(UsermodeTimer* t, u64 interval)
{
    t->next_time = armGetSystemTick() + armNsToTick(interval);
    t->interval = armNsToTick(interval);
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
