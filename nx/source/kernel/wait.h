// Copyright 2018 plutoo
#pragma once
#include "kernel/mutex.h"
#include "kernel/wait.h"

static inline void _waitableInitialize(Waitable* ww)
{
    mutexInit(&ww->mutex);
    ww->list.next = &ww->list;
    ww->list.prev = &ww->list;
}

static inline void _waitableSignalAllListeners(Waitable* ww)
{
    WaitableNode* node = &ww->list;
    WaitableNode* end = node;

    while (node->next != end)
    {
        node = node->next;
        WaiterNode* w = (WaiterNode*) node;

        // Try to swap -1 => idx on the waiter thread.
        // If another waitable signals simultaneously only one will win the race and insert its own idx.
        size_t minus_one = -1;
        bool sent_idx = __atomic_compare_exchange_n(
            w->idx_out, &minus_one, w->idx, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

        if (sent_idx) {
            svcCancelSynchronization(w->thread);
        }
    }
}

static inline void _waiterNodeCreate(
    WaiterNode* w, WaiterNodeType type, Waitable* parent, Handle thread,
    size_t idx, size_t* idx_out)
{
    w->type = type;
    w->parent = parent;
    w->thread = thread;
    w->idx = idx;
    w->idx_out = idx_out;
}

static inline void _waiterNodeAddToWaitable(WaiterNode* w, Waitable* ww)
{
    w->node.next = ww->list.next;
    ww->list.next = &w->node;
    w->node.prev = &ww->list;
}

static inline void _waiterNodeFree(WaiterNode* w)
{
    mutexLock(&w->parent->mutex);
    w->node.prev->next = w->node.next;
    w->node.next->prev = w->node.prev;
    mutexUnlock(&w->parent->mutex);
}
