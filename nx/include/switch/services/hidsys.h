/**
 * @file hidsys.h
 * @brief hid:sys service IPC wrapper.
 * @author exelix
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/sm.h"

Result hidSysInitialize(void);

void hidSysExit(void);

Result hidSysEnableAppletToGetInput(bool enable);

/**
* @brief Returns an event that fires when the home button is pressed, this will prevent the home menu from opening when the button is pressed.
**/ 
Result hidSysAcquireHomeButtonEventHandle(Event* event_out);