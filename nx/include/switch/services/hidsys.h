/**
 * @file hidsys.h
 * @brief hid:sys service IPC wrapper.
 * @author exelix, yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/hid.h"
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

/**
 * @brief Gets the UniquePadIds for the specified controller.
 * @note Only available on [3.0.0+].
 * @param id Controller ID. Must not be CONTROLLER_P1_AUTO.
 * @param UniquePadIds Output array of UniquePadIds.
 * @param Max number of entries for the UniquePadIds array.
 * @param total_entries Total output array entries. Optional, can be NULL.
 */
Result hidsysGetUniquePadsFromNpad(HidControllerID id, u64 *UniquePadIds, size_t count, size_t *total_entries);

/**
 * @brief Gets a list of all UniquePadIds.
 * @param UniquePadIds Output array of UniquePadIds.
 * @param Max number of entries for the UniquePadIds array.
 * @param total_entries Total output array entries. Optional, can be NULL.
 */
Result hidsysGetUniquePadIds(u64 *UniquePadIds, size_t count, size_t *total_entries);

