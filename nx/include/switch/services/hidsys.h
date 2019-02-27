/**
 * @file hidsys.h
 * @brief hid:sys service IPC wrapper.
 * @author exelix
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/sm.h"

Result hidsysInitialize(void);
void hidsysExit(void);

Result hidsysEnableAppletToGetInput(bool enable);

/**
* @brief Returns an event that fires when the home button is pressed, this will prevent the home menu from opening when the button is pressed. This event does not auto clear.
**/ 
Result hidsysAcquireHomeButtonEventHandle(Event* event_out);

Result hidsysActivateHomeButton(void);
Result hidsysActivateSleepButton(void);
Result hidsysActivateCaptureButton(void);
