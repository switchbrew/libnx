/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet, exelix
 * @copyright libnx Authors
 */

#pragma once
#include "../kernel/ipc.h"
#include "../services/sm.h"

typedef enum {
    NifmServiceType_NotInitialized = 0,
    NifmServiceType_User = 1,
    NifmServiceType_System = 2,
    NifmServiceType_Admin = 3,
} NifmServiceType;

/**
 * @brief Sets the \ref NifmServiceType for initialization. Call this function before \ref nifmInitialize.
 * @note By default nifm:u will be used.
 */
void nifmSetServiceType(NifmServiceType serviceType);

Result nifmInitialize(void);
void nifmExit(void);

Result nifmGetCurrentIpAddress(u32* out);

Result nifmIsWirelessCommunicationEnabled(bool* out);

/**
 * @note Works only if called from nifm:a or nifm:s.
 */
Result nifmSetWirelessCommunicationEnabled(bool enable);

Result nifmIsEthernetCommunicationEnabled(bool* out);
Result nifmIsAnyForegroundRequestAccepted(bool* out);
Result nifmPutToSleep(void);
Result nifmWakeUp(void);
