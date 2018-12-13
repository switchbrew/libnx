// Copyright 2018 plutoo
#pragma once
#include "kernel/uevent.h"

void _ueventTryAutoClear(UEvent* e);
bool _ueventAddListener(UEvent* e, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread);
