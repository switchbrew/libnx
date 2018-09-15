/**
 * @file psm.h
 * @brief PSM service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum {
    ChargerType_None = 0,    ///< No charger
    ChargerType_Charger = 1, ///< Official charger or dock
    ChargerType_Usb = 2      ///< Other USB-C chargers
} ChargerType;

Result psmInitialize(void);
void psmExit(void);

Result psmGetBatteryChargePercentage(u32 *out);
Result psmGetChargerType(ChargerType *out);
