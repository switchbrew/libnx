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

/// Mini Cycle struct for \ref HidsysNotificationLedPattern.
typedef struct {
    u8 ledIntensity;      ///< Mini Cycle X LED Intensity.
    u8 fadingTransition;  ///< Fading Transition Duration to Mini Cycle X (Uses PWM). Value is a Multiplier of HidsysNotificationLedPattern::globalMiniCycleDuration.
    u8 ledDuration;       ///< LED Duration Multiplier of Mini Cycle X. x0 = x1 = x1
    u8 pad;
} HidsysNotificationLedPatternCycle;

/// Structure for \ref hidsysSetNotificationLedPattern.
/// See also: https://switchbrew.org/wiki/HID_services#NotificationLedPattern
/// Only the low 4bits of each used byte in this struct is used.
typedef struct {
    u8 globalMiniCycleDuration;                         ///< Global Mini Cycle Duration. 8ms - 175ms. Value 0x0 = 0ms/OFF.
    u8 totalMiniCycles;                                 ///< Number of Mini Cycles. 1-15. If number of cycles is > 0 then x0 = x1
    u8 totalFullCycles;                                 ///< Number of Full Cycles. 1-15. Value 0x0 is repeat forever, but if globalMiniCycleDuration is set to 0x0, it does the 1st Mini Cycle and then the LED stays on with startIntensity.
    u8 startIntensity;                                  ///< LED Start Intensity. Value 0x0=0% - 0xF=100%.

    HidsysNotificationLedPatternCycle miniCycles[16];   ///< Mini Cycles

    u8 unk_x44[0x2];                                    ///< Unknown
    u8 pad_x46[0x2];                                    ///< Padding
} HidsysNotificationLedPattern;

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

/**
 * @brief Sets the HOME-button notification LED pattern, for the specified controller.
 * @param pattern \ref HidsysNotificationLedPattern
 * @param UniquePadId UniquePadId for the controller.
 */
Result hidsysSetNotificationLedPattern(const HidsysNotificationLedPattern *pattern, u64 UniquePadId);

