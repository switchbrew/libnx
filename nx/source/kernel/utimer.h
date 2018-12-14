// Copyright 2018 plutoo
#pragma once
#include "kernel/utimer.h"
#include "wait.h"

void _utimerRecalculate(UTimer* t, u64 old_tick);
u64  _utimerGetNextTick(UTimer* t);
void _utimerAddListener(UTimer* t, WaiterNode* w, s32 idx, s32* idx_out, Handle thread);
