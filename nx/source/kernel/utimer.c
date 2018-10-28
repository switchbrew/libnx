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
    u64 zero = 0;
    __atomic_compare_exchange_n(&t->next_time, &zero, armGetSystemTick() + armNsToTick(t->interval), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

void utimerStop(UsermodeTimer* t)
{
    while (!__atomic_compare_exchange_n(&t->next_time, &t->next_time, 0, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}

void _utimerRecalculate(UsermodeTimer* t, u64 old_time)
{
    s64 interval = t->interval;
    s64 new_time = ((armGetSystemTick() - old_time + interval - 1) / interval) * interval;
    __atomic_compare_exchange_n(&t->next_time, &old_time, new_time, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

u64 _utimerGetNextTime(UsermodeTimer* t)
{
    return __atomic_load_n(&t->next_time, __ATOMIC_SEQ_CST);
}
