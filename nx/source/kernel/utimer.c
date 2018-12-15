// Copyright 2018 plutoo
#include "result.h"
#include "arm/counter.h"
#include "kernel/svc.h"
#include "kernel/utimer.h"
#include "wait.h"

#define STOPPED 0

static bool _utimerBeginWait(Waitable* ww, WaiterNode* w, u64 cur_tick, u64* next_tick);
static Result _utimerOnTimeout(Waitable* ww, u64 old_tick);
static Result _utimerOnSignal(Waitable* ww);

static const WaitableMethods g_utimerVt = {
    .beginWait = _utimerBeginWait,
    .onTimeout = _utimerOnTimeout,
    .onSignal = _utimerOnSignal,
};

void utimerCreate(UTimer* t, u64 interval, TimerType type)
{
    _waitableInitialize(&t->waitable, &g_utimerVt);

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

static void _utimerRecalculate(UTimer* t, u64 old_tick)
{
    if (t->next_tick == old_tick) {
        u64 interval = t->interval;
        u64 new_tick = 0;

        switch (t->type) {
            case TimerType_OneShot:
                new_tick = STOPPED;
                break;
            case TimerType_Repeating:
                new_tick = old_tick + ((armGetSystemTick() - old_tick + interval - 1)/interval)*interval;
                break;
        }

        t->next_tick = new_tick;
    }
}

static bool _utimerBeginWait(Waitable* ww, WaiterNode* w, u64 cur_tick, u64* next_tick)
{
    UTimer* t = (UTimer*)ww;
    mutexLock(&t->waitable.mutex);

    // By default we do want to perform a wait, because if the
    // timer's start/stop state changes we want to detect that.
    u64 timer_tick = t->next_tick;
    bool do_wait = true;

    // Skip timer if stopped.
    if (timer_tick != STOPPED) {
        s64 diff = timer_tick - cur_tick;

        // If the timer is already signalled, we're done.
        if (diff < 0) {
            _utimerRecalculate(t, timer_tick);
            do_wait = false;
        } else
            *next_tick = diff;
    }

    if (do_wait)
        _waiterNodeAdd(w);

    mutexUnlock(&t->waitable.mutex);
    return do_wait;
}

static Result _utimerOnTimeout(Waitable* ww, u64 old_tick)
{
    UTimer* t = (UTimer*)ww;
    mutexLock(&t->waitable.mutex);
    _utimerRecalculate(t, old_tick);
    mutexUnlock(&t->waitable.mutex);
    return 0;
}

static Result _utimerOnSignal(Waitable* ww)
{
    // Timer state changed, so we need to retry the wait.
    return KERNELRESULT(Cancelled);
}
