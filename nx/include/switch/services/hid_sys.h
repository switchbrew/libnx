/**
 * @file hid.h
 * @brief Human input device (hid:sys) service IPC wrapper.
 * @author p-sam
 * @copyright libnx Authors
 */
 #pragma once
#include "../types.h"

Result hidSysInitialize(void);
void hidSysExit(void);

/// Disables the default behavior of the HOME button (going back to the main menu) if it was enabled
Result hidSysActivateHomeButton();