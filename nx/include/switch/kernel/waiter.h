// Copyright 2018 plutoo
#pragma once
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "kernel/wait.h"

#define MAX_WAIT 0x40

typedef struct WaiterNode WaiterNode;
typedef struct Waiter Waiter;

struct WaiterNode
{
    Waiter* owner;
    s32 idx;
    WaiterNode* prev;
    WaiterNode* next;
};

struct Waiter
{
    Mutex mutex;
    Handle thread;
    s32 signalled_idx;

    WaiterNode nodes[MAX_WAIT];
    size_t num_nodes;
};

// Internal methods (do not use!):
void _waiterCreate(Waiter* w);
void _waiterFree(Waiter* w, WaitObject* objects);
void _waiterSubscribe(Waiter* w, UsermodeEvent* e);
bool _waiterSignal(Waiter* w, s32 idx);
s32  _waiterGetSignalledIndex(Waiter* w);
