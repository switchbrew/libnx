// Copyright 2018 plutoo
#pragma once
#include "kernel/uevent.h"

void _ueventTryAutoClear(UEvent* e);
bool _ueventAddListener(UEvent* e, WaiterNode* w, s32 idx, s32* idx_out, Handle thread);
