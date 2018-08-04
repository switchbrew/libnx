/**
 * @file hid.h
 * @brief Human input device (hid:dbg) service IPC wrapper.
 * @author p-sam
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

#define HID_DBG_DHB_MAX_TRIES 100
#define HID_DBG_DHB_RC (0x16eca)

Result hidDbgInitialize(void);
void hidDbgExit(void);

/// Enables the default behavior of the HOME button (going back to the main menu) if it was disabled
Result hidDbgDeactivateHomeButton();
