// Copyright 2018 plutoo
#pragma once
#include "kernel/utimer.h"

void _utimerRecalculate(UsermodeTimer* t, u64 old_tick);
u64  _utimerGetNextTick(UsermodeTimer* t);
void _utimerAddListener(UsermodeTimer* t, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread);
