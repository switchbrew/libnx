// Copyright 2018 plutoo
#pragma once
#include "../types.h"
#include "../result.h"
#include "wait.h"

typedef struct {
    Handle revent;
    Handle wevent;
    bool autoclear;
} Event;

/// Creates a \ref Waiter for a kernel-mode event.
static inline Waiter waiterForEvent(Event* t)
{
    Waiter wait_obj;
    wait_obj.type = t->autoclear ? WaiterType_HandleWithClear : WaiterType_Handle;
    wait_obj.handle = t->revent;
    return wait_obj;
}

Result eventCreate(Event* t, bool autoclear);
void   eventLoadRemote(Event* t, Handle handle, bool autoclear);
void   eventClose(Event* t);

/// Returns whether the Event is initialized.
static inline bool eventActive(Event* t)
{
    return t->revent != INVALID_HANDLE;
}

Result eventWait(Event* t, u64 timeout);
Result eventFire(Event* t);
Result eventClear(Event* t);
