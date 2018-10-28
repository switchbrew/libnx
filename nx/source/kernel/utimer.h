// Copyright 2018 plutoo
#pragma once
#include "kernel/utimer.h"

void _utimerRecalculate(UsermodeTimer* t, u64 old_time);
u64  _utimerGetNextTime(UsermodeTimer* t);
