// Copyright 2018 plutoo
#pragma once
#include "kernel/mutex.h"

typedef enum {
    WaitObjectType_Handle,
    WaitObjectType_UsermodeTimer,
    WaitObjectType_UsermodeEvent,
} WaitObjectType;

typedef struct UsermodeTimer UsermodeTimer;
typedef struct UsermodeEvent UsermodeEvent;

typedef struct {
    WaitObjectType type;

    union {
        Handle handle;
        UsermodeTimer* timer;
        UsermodeEvent* event;
    };
} WaitObject;

Result waitN(s32* idx_out, WaitObject* objects, size_t num_objects, u64 timeout);
