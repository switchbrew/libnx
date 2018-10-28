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
