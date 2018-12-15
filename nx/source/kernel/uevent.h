// Copyright 2018 plutoo
#pragma once
#include "kernel/uevent.h"
#include "wait.h"

Result _ueventTryAutoClear(UEvent* e);
bool _ueventAddListener(UEvent* e, WaiterNode* w);
