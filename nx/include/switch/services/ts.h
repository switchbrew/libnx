/**
 * @file ts.h
 * @brief Temperature measurement (ts) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Location
typedef enum {
    TsLocation_Internal = 0,    ///< TMP451 Internal: PCB
    TsLocation_External = 1,    ///< TMP451 External: SoC
} TsLocation;

typedef enum {
    TsDeviceCode_LocationInternal = 0x41000001u,
    TsDeviceCode_LocationExternal = 0x41000002u,
} TsDeviceCode;

typedef struct {
    Service s;
} TsSession;

/// Initialize ts.
Result tsInitialize(void);

/// Exit ts.
void tsExit(void);

/// Gets the Service for ts.
Service* tsGetServiceSession(void);

/**
 * @brief Gets the min/max temperature for the specified \ref TsLocation.
 * @param[in] location \ref TsLocation
 * @param[out] min_temperature Output minimum temperature in Celsius.
 * @param[out] max_temperature Output maximum temperature in Celsius.
 */
Result tsGetTemperatureRange(TsLocation location, s32 *min_temperature, s32 *max_temperature);

/**
 * @brief Gets the temperature for the specified \ref TsLocation.
 * @param[in] location \ref TsLocation
 * @param[out] temperature Output temperature in Celsius.
 */
Result tsGetTemperature(TsLocation location, s32 *temperature);

/**
 * @brief Gets the temperature for the specified \ref TsLocation, in MilliC. [1.0.0-13.2.1]
 * @param[in] location \ref TsLocation
 * @param[out] temperature Output temperature in MilliC.
 */
Result tsGetTemperatureMilliC(TsLocation location, s32 *temperature);

Result tsOpenSession(TsSession *s, u32 device_code); ///< [8.0.0+]

Result tsSessionGetTemperature(TsSession *s, float *temperature); ///< [10.0.0+]
void tsSessionClose(TsSession *s);