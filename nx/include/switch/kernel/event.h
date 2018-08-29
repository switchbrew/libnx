// Copyright 2018 plutoo
#pragma once
#include "../types.h"
#include "../result.h"
#include "../kernel/svc.h"

typedef struct {
    Handle revent;
    Handle wevent;
    bool autoclear;
} Event;

Result eventCreate(Event* t, bool autoclear);
void   eventLoadRemote(Event* t, Handle handle, bool autoclear);
void   eventClose(Event* t);

Result eventWait(Event* t, u64 timeout);
Result eventFire(Event* t);
Result eventClear(Event* t);
