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

typedef struct {
    Service  s;
} IGeneralService;

Result nifmInitialize();
void nifmExit(void);

Result CreateGeneralService(IGeneralService*out);
Result _CreateGeneralService(IGeneralService* out, u64 in);
Result _CreateGeneralServiceOld(IGeneralService* out);

Result GetCurrentIpAddress(IGeneralService* srv, u32* out);