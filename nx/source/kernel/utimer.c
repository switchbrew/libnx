// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/utimer.h"
#include "arm/counter.h"
#include "wait.h"

#define STOPPED 0

void utimerCreate(UTimer* t, u64 interval, TimerType type)
{
    _waitableInitialize(&t->waitable);

    t->next_tick = STOPPED;
    t->interval = armNsToTicks(interval);
    t->type = type;
}

void utimerStart(UTimer* t)
{
    mutexLock(&t->waitable.mutex);

    if (t->next_tick == STOPPED) {
        u64 new_tick = armGetSystemTick() + t->interval;
        t->next_tick = new_tick;
        _waitableSignalAllListeners(&t->waitable);
    }

    mutexUnlock(&t->waitable.mutex);
}

void utimerStop(UTimer* t)
{
    mutexLock(&t->waitable.mutex);

    if (t->next_tick != STOPPED) {
        t->next_tick = STOPPED;
        _waitableSignalAllListeners(&t->waitable);
    }

    mutexUnlock(&t->waitable.mutex);
}

void _utimerRecalculate(UTimer* t, u64 old_tick)
{
    mutexLock(&t->waitable.mutex);

    if (t->next_tick == old_tick) {
        u64 interval = t->interval;
        u64 new_tick = 0;

        switch (t->type) {
            case TimerType_OneShot:
                new_tick = STOPPED;
                break;
            case TimerType_Repeating:
                new_tick = old_tick + ((svcGetSystemTick() - old_tick + interval - 1)/interval)*interval;
                break;
        }

        t->next_tick = new_tick;
    }

    mutexUnlock(&t->waitable.mutex);
}

u64 _utimerGetNextTick(UTimer* t)
{
    u64 ret;

    mutexLock(&t->waitable.mutex);
    ret = t->next_tick;
    mutexUnlock(&t->waitable.mutex);

    return ret;
}

void _utimerAddListener(UTimer* t, WaiterNode* w, s32 idx, s32* idx_out, Handle thread)
{
    mutexLock(&t->waitable.mutex);
    _waiterNodeAdd(w, &t->waitable, thread, idx, idx_out);
    mutexUnlock(&t->waitable.mutex);
}
