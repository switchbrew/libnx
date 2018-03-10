// Copyright 2018 plutoo
#pragma once
#include "../types.h"
#include "../result.h"
#include "../kernel/svc.h"

typedef struct {
    Handle revent;
    Handle wevent;
} Event;

Result eventCreate(Event* t);
void   eventLoadRemote(Event* t, Handle handle);
void   eventClose(Event* t);

Result eventWait(Event* t, u64 timeout);
Result eventFire(Event* t);
Result eventClear(Event* t);

