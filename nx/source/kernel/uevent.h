// Copyright 2018 plutoo
#pragma once
#include "kernel/uevent.h"

void _ueventTryAutoClear(Uevent* e);
bool _ueventAddListener(Uevent* e, WaiterNode* w, size_t idx, size_t* idx_out, Handle thread);
