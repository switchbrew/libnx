// Copyright 2018 plutoo
#pragma once
#include "../kernel/mutex.h"
#include "../kernel/event.h"
#include "../kernel/thread.h"

// Implementation details.
typedef struct UsermodeEvent UsermodeEvent;
typedef struct UsermodeTimer UsermodeTimer;

typedef enum {
    WaiterNodeType_Event,
    WaiterNodeType_Timer
} WaiterNodeType;

typedef struct Waitable Waitable;
typedef struct WaitableNode WaitableNode;

struct WaitableNode
{
    WaitableNode* prev;
    WaitableNode* next;
};

typedef struct {
    WaitableNode node;
    WaiterNodeType type;
    Handle thread;
    union {
        Waitable* parent;
        UsermodeEvent* parent_event;
        UsermodeTimer* parent_timer;
    };
    size_t idx;
    size_t* idx_out;
} WaiterNode;

struct Waitable
{
    WaitableNode list;
    Mutex mutex;
};

// User-facing API starts here.
typedef enum {
    WaiterType_Handle,
    WaiterType_UsermodeTimer,
    WaiterType_UsermodeEvent,
} WaiterType;

typedef struct {
    WaiterType type;

    union {
        Handle handle;
        UsermodeTimer* timer;
        UsermodeEvent* event;
    };
} Waiter;

static inline Waiter waiterForHandle(Handle h)
{
    Waiter wait_obj;
    wait_obj.type = WaiterType_Handle;
    wait_obj.handle = h;
    return wait_obj;
}

static inline Waiter waiterForUtimer(UsermodeTimer* t)
{
    Waiter wait_obj;
    wait_obj.type = WaiterType_UsermodeTimer;
    wait_obj.timer = t;
    return wait_obj;
}

static inline Waiter waiterForUevent(UsermodeEvent* e)
{
    Waiter wait_obj;
    wait_obj.type = WaiterType_UsermodeEvent;
    wait_obj.event = e;
    return wait_obj;
}

static inline Waiter waiterForEvent(Event* e) {
    return waiterForHandle(e->revent);
}

static inline Waiter waiterForThreadExit(Thread* t) {
    return waiterForHandle(t->handle);
}

#define waitMulti(idx_out, timeout, ...) \
    waitN((idx_out), (timeout), (Waiter[]) { __VA_ARGS__ }, sizeof((Waiter[]) { __VA_ARGS__ }) / sizeof(Waiter))


Result waitN(s32* idx_out, u64 timeout, Waiter* objects, size_t num_objects);
