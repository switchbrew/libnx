/**
 * @file irs.h
 * @brief HID IR sensor (irs) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../services/sm.h"
#include "../services/hid.h"

Result irsInitialize(void);
void irsExit(void);

Service* irsGetSessionService(void);
void* irsGetSharedmemAddr(void);

/// (De)activate the IR sensor, this is automatically used by irsExit(). Must be called after irsInitialize() to activate the IR sensor.
Result irsActivateIrsensor(bool activate);

Result irsGetIrCameraHandle(u32 *IrCameraHandle, HidControllerID id);
