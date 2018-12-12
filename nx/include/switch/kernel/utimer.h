// Copyright 2018 plutoo
#pragma once
#include "../kernel/mutex.h"
#include "../kernel/wait.h"

typedef struct UsermodeTimer UsermodeTimer;

struct UsermodeTimer
{
    Waitable waitable;
    u64 next_tick;
    u64 interval;
};

void utimerCreate(UsermodeTimer* t, u64 interval, bool start);
void utimerStart(UsermodeTimer* t);
void utimerStop(UsermodeTimer* t);
