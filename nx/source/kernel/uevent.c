// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "kernel/uevent.h"
#include "wait.h"
#include "uevent.h"

void ueventCreate(UsermodeEvent* e, bool auto_clear)
{
    _waitableInitialize(&e->waitable);

    e->signal = false;
    e->auto_clear = auto_clear;
}

void ueventClear(UsermodeEvent* e)
{
    mutexLock(&e->waitable.mutex);
    e->signal = false;
    mutexUnlock(&e->waitable.mutex);
}

void ueventSignal(UsermodeEvent* e)
{
    mutexLock(&e->waitable.mutex);
    e->signal = true;
    _waitableSignalAllListeners(&e->waitable);
    mutexUnlock(&e->waitable.mutex);
}

void _ueventTryAutoClear(UsermodeEvent* e)
{
    mutexLock(&e->waitable.mutex);
    if (e->auto_clear) {
        e->signal = false;
    }
    mutexUnlock(&e->waitable.mutex);
}

bool _ueventAddListener(UsermodeEvent* e, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread)
{
    _waiterNodeCreate(w, WaiterNodeType_Event, &e->waitable, thread, idx, idx_out);

    mutexLock(&e->waitable.mutex);

    bool signalled = e->signal;
    bool ret;

    if (signalled)
    {
        if (e->auto_clear) {
            e->signal = false;
        }
        ret = false;
    }
    else
    {
        _waiterNodeAddToWaitable(w, &e->waitable);
        ret = true;
    }

    mutexUnlock(&e->waitable.mutex);
    return ret;
}
