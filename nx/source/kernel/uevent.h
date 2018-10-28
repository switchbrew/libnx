// Copyright 2018 plutoo
#pragma once
#include "kernel/uevent.h"

bool _ueventConsumeIfSignalled(UsermodeEvent* e);
void _ueventAddListener(UsermodeEvent* e, WaiterNode* w);
void _ueventRemoveListener(UsermodeEvent* e, WaiterNode* w);
