// Copyright 2018 plutoo
#pragma once
#include "kernel/utimer.h"

void _utimerRecalculate(UTimer* t, u64 old_tick);
u64  _utimerGetNextTick(UTimer* t);
void _utimerAddListener(UTimer* t, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread);
