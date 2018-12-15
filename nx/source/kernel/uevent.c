// Copyright 2018 plutoo
#include "result.h"
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "kernel/uevent.h"
#include "wait.h"

static bool _ueventBeginWait(Waitable* ww, WaiterNode* w, u64 cur_tick, u64* next_tick);
static Result _ueventOnTimeout(Waitable* ww, u64 old_tick);
static Result _ueventOnSignal(Waitable* ww);

static const WaitableMethods g_ueventVt = {
    .beginWait = _ueventBeginWait,
    .onTimeout = _ueventOnTimeout,
    .onSignal = _ueventOnSignal,
};

void ueventCreate(UEvent* e, bool auto_clear)
{
    _waitableInitialize(&e->waitable, &g_ueventVt);

    e->signal = false;
    e->auto_clear = auto_clear;
}

void ueventClear(UEvent* e)
{
    mutexLock(&e->waitable.mutex);
    e->signal = false;
    mutexUnlock(&e->waitable.mutex);
}

void ueventSignal(UEvent* e)
{
    mutexLock(&e->waitable.mutex);
    e->signal = true;
    _waitableSignalAllListeners(&e->waitable);
    mutexUnlock(&e->waitable.mutex);
}

Result _ueventOnSignal(Waitable* ww)
{
    UEvent* e = (UEvent*)ww;
    Result rc = 0;
    mutexLock(&e->waitable.mutex);

    // Try to auto-clear the event. If auto-clear is enabled but
    // the event is not signalled, that means the state of the
    // event has changed and thus we need to retry the wait.
    if (e->auto_clear) {
        if (e->signal)
            e->signal = false;
        else
            rc = KERNELRESULT(Cancelled);
    }

    mutexUnlock(&e->waitable.mutex);
    return rc;
}

bool _ueventBeginWait(Waitable* ww, WaiterNode* w, u64 cur_tick, u64* next_tick)
{
    UEvent* e = (UEvent*)ww;
    mutexLock(&e->waitable.mutex);

    bool can_add = !e->signal;

    if (can_add)
        _waiterNodeAdd(w);
    else if (e->auto_clear)
        e->signal = false;

    mutexUnlock(&e->waitable.mutex);
    return can_add;
}

Result _ueventOnTimeout(Waitable* ww, u64 old_tick)
{
    // This is not supposed to happen.
    return KERNELRESULT(Cancelled);
}
