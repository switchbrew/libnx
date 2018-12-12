// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/utimer.h"
#include "arm/counter.h"
#include "utimer.h"
#include "wait.h"

#define STOPPED 0

void utimerCreate(UsermodeTimer* t, u64 interval, bool start)
{
    _waitableInitialize(&t->waitable);

    t->next_tick = STOPPED;
    t->interval = armNsToTicks(interval);

    if (start)
        utimerStart(t);
}

void utimerStart(UsermodeTimer* t)
{
    mutexLock(&t->waitable.mutex);

    if (t->next_tick == STOPPED)
    {
        u64 new_tick = armGetSystemTick() + t->interval;
        t->next_tick = new_tick;
        _waitableSignalAllListeners(&t->waitable);
    }

    mutexUnlock(&t->waitable.mutex);
}

void utimerStop(UsermodeTimer* t)
{
    mutexLock(&t->waitable.mutex);

    if (t->next_tick != STOPPED)
    {
        t->next_tick = STOPPED;
        _waitableSignalAllListeners(&t->waitable);
    }

    mutexUnlock(&t->waitable.mutex);
}

void _utimerRecalculate(UsermodeTimer* t, u64 old_tick)
{
    mutexLock(&t->waitable.mutex);

    if (t->next_tick == old_tick)
    {
        u64 interval = t->interval;
        u64 new_tick = ((armGetSystemTick() + interval - 1) / interval) * interval;

        t->next_tick = new_tick;
    }

    mutexUnlock(&t->waitable.mutex);
}

u64 _utimerGetNextTick(UsermodeTimer* t)
{
    u64 ret;

    mutexLock(&t->waitable.mutex);
    ret = t->next_tick;
    mutexUnlock(&t->waitable.mutex);

    return ret;
}

void _utimerAddListener(UsermodeTimer* t, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread)
{
    _waiterNodeInitialize(w, WaiterNodeType_Timer, &t->waitable, thread, idx, idx_out);

    mutexLock(&t->waitable.mutex);
    _waiterNodeAddToWaitable(w, &t->waitable);
    mutexUnlock(&t->waitable.mutex);
}
