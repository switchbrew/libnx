// Copyright 2018 plutoo
#pragma once
#include "kernel/svc.h"

typedef struct UsermodeTimer UsermodeTimer;

struct UsermodeTimer
{
    u64 next_time;
    u64 interval;
};

void utimerCreate(UsermodeTimer* t, u64 interval, bool start);
void utimerStart(UsermodeTimer* t);
void utimerStop(UsermodeTimer* t);
