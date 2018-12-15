// Copyright 2018 plutoo
#include "result.h"
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "kernel/uevent.h"
#include "uevent.h"

void ueventCreate(UEvent* e, bool auto_clear)
{
    _waitableInitialize(&e->waitable);

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

Result _ueventTryAutoClear(UEvent* e)
{
    Result rc = 0;
    mutexLock(&e->waitable.mutex);
    if (e->auto_clear) {
        if (e->signal)
            e->signal = false;
        else
            rc = KERNELRESULT(Cancelled);
    }
    mutexUnlock(&e->waitable.mutex);
    return rc;
}

bool _ueventAddListener(UEvent* e, WaiterNode* w)
{
    mutexLock(&e->waitable.mutex);

    bool can_add = !e->signal;

    if (can_add)
        _waiterNodeAdd(w);
    else if (e->auto_clear)
        e->signal = false;

    mutexUnlock(&e->waitable.mutex);
    return can_add;
}
