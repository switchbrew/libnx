// Copyright 2018 plutoo
#pragma once
#include "../kernel/mutex.h"
#include "../kernel/wait.h"

typedef struct UsermodeEvent UsermodeEvent;

struct UsermodeEvent
{
    Waitable waitable;
    bool signal;
    bool auto_clear;
};

void ueventCreate(UsermodeEvent* e, bool auto_clear);
void ueventClear(UsermodeEvent* e);
void ueventSignal(UsermodeEvent* e);
