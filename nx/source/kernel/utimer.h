// Copyright 2018 plutoo
#pragma once
#include "kernel/utimer.h"

void _utimerRecalculate(Utimer* t, u64 old_tick);
u64  _utimerGetNextTick(Utimer* t);
void _utimerAddListener(Utimer* t, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread);
