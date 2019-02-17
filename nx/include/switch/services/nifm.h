/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet
 * @copyright libnx Authors
 */

#pragma once
#include "../kernel/ipc.h"
#include "../services/sm.h"

Result nifmInitialize(void);
void nifmExit(void);

Result nifmGetCurrentIpAddress(u32* out);

Result nifmIsWirelessCommunicationEnabled(bool* out);

Result nifmIsEthernetCommunicationEnabled(bool* out);
Result nifmIsAnyForegroundRequestAccepted(bool* out);
Result nifmPutToSleep(void);
Result nifmWakeUp(void);
