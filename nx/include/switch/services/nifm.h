/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108
 * @copyright libnx Authors
 */

#pragma once

#include "../kernel/ipc.h"
#include "../kernel/detect.h"

#include "../services/sm.h"

Result nifmInitialize(void);
void nifmExit(void);

Result nifmGetCurrentIpAddress(u32* out);
