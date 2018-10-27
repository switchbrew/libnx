// Copyright 2018 plutoo
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "kernel/waiter.h"
#include "kernel/uevent.h"

void ueventCreate(UsermodeEvent* e, bool auto_clear)
{
    mutexInit(&e->mutex);
    e->waiter_list.prev = &e->waiter_list;
    e->waiter_list.next = &e->waiter_list;
    e->signal = false;
    e->auto_clear = auto_clear;
}

void ueventClear(UsermodeEvent* e)
{
    mutexLock(&e->mutex);
    e->signal = false;
    mutexUnlock(&e->mutex);
}

void ueventSignal(UsermodeEvent* e)
{
    mutexLock(&e->mutex);
    e->signal = true;

    WaiterNode* end = &e->waiter_list;
    WaiterNode* w = end;

    while (w->next != end)
    {
        w = w->next;

        bool signalled = _waiterSignal(w->owner, w->idx);

        if (signalled && e->auto_clear)
            e->signal = false;
    }

    mutexUnlock(&e->mutex);
}

bool _ueventConsumeIfSignalled(UsermodeEvent* e)
{
    bool ret;
    mutexLock(&e->mutex);

    ret = e->signal;

    if (ret && e->auto_clear)
        e->signal = false;

    mutexUnlock(&e->mutex);
    return ret;
}

void _ueventAddListener(UsermodeEvent* e, WaiterNode* w)
{
    mutexLock(&e->mutex);
    e->waiter_list.next->prev = w;
    w->next = e->waiter_list.next;
    w->prev = &e->waiter_list;
    e->waiter_list.next = w;
    mutexUnlock(&e->mutex);
}

void _ueventRemoveListener(UsermodeEvent* e, WaiterNode* w)
{
    mutexLock(&e->mutex);
    w->prev->next = w->next;
    w->next->prev = w->prev;
    mutexUnlock(&e->mutex);
}
