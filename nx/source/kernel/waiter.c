// Copyright 2018 plutoo
#include "kernel/waiter.h"
#include "kernel/uevent.h"
#include "waiter.h"
#include "uevent.h"
#include "../internal.h"

#define NOT_YET_SIGNALLED (-1)

void _waiterCreate(Waiter* w)
{
    mutexInit(&w->mutex);
    w->thread = getThreadVars()->handle;
    w->signalled_idx = NOT_YET_SIGNALLED;
    w->num_nodes = 0;
}

void _waiterFree(Waiter* w, WaitObject* objects)
{
    size_t i;
    for (i=0; i<w->num_nodes; i++)
        _ueventRemoveListener(objects[w->nodes[i].idx].event, &w->nodes[i]);
}

bool _waiterSignal(Waiter* w, s32 idx)
{
    bool ret = false;
    mutexLock(&w->mutex);

    if (w->signalled_idx == NOT_YET_SIGNALLED)
    {
        w->signalled_idx = idx;
        svcCancelSynchronization(w->thread);
        ret = true;
    }

    mutexUnlock(&w->mutex);
    return ret;
}

void _waiterSubscribe(Waiter* w, UsermodeEvent* e)
{
    _ueventAddListener(e, &w->nodes[w->num_nodes++]);
}

s32 _waiterGetSignalledIndex(Waiter* w)
{
    return w->signalled_idx;
}
