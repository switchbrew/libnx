// Copyright 2018 plutoo
#pragma once
#include "kernel/waiter.h"

void _waiterCreate(Waiter* w);
void _waiterFree(Waiter* w, WaitObject* objects);
void _waiterSubscribe(Waiter* w, UsermodeEvent* e);
bool _waiterSignal(Waiter* w, s32 idx);
s32  _waiterGetSignalledIndex(Waiter* w);

