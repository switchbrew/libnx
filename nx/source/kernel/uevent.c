// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "kernel/uevent.h"
#include "wait.h"
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

void _ueventTryAutoClear(UEvent* e)
{
    mutexLock(&e->waitable.mutex);
    if (e->auto_clear)
        e->signal = false;
    mutexUnlock(&e->waitable.mutex);
}

bool _ueventAddListener(UEvent* e, WaiterNode* w, s32 idx, s32* idx_out, Handle thread)
{
    mutexLock(&e->waitable.mutex);

    bool signalled = e->signal;

    if (signalled) {
        if (e->auto_clear)
            e->signal = false;
    } else {
        _waiterNodeCreate(w, WaiterNodeType_Event, &e->waitable, thread, idx, idx_out);
        _waiterNodeAddToWaitable(w, &e->waitable);
    }

    mutexUnlock(&e->waitable.mutex);
    return !signalled;
}
