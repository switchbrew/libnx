/**
 * @file psm.h
 * @brief PSM service IPC wrapper.
 * @author XorTroll, endrift, and yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../kernel/event.h"

typedef enum {
    PsmChargerType_Unconnected = 0,  ///< No charger
    PsmChargerType_EnoughPower = 1,  ///< Full supported power
    PsmChargerType_LowPower = 2,     ///< Lower power supported USB-PD mode
    PsmChargerType_NotSupported = 3, ///< No common supported USB-PD modes
} PsmChargerType;

typedef enum {
    PsmBatteryVoltageState_NeedsShutdown = 0,      ///< Power state should transition to shutdown
    PsmBatteryVoltageState_NeedsSleep = 1,         ///< Power state should transition to sleep
    PsmBatteryVoltageState_NoPerformanceBoost = 2, ///< Performance boost modes cannot be entered
    PsmBatteryVoltageState_Normal = 3,             ///< Everything is normal
} PsmBatteryVoltageState;

/// IPsmSession
typedef struct {
    Service s;
    Event StateChangeEvent;  ///< autoclear=false
} PsmSession;

/// Initialize psm.
Result psmInitialize(void);

/// Exit psm.
void psmExit(void);

/// Gets the Service object for the actual psm service session.
Service* psmGetServiceSession(void);

Result psmGetBatteryChargePercentage(u32 *out);
Result psmGetChargerType(PsmChargerType *out);
Result psmGetBatteryVoltageState(PsmBatteryVoltageState *out);
Result psmGetRawBatteryChargePercentage(double *out);
Result psmIsEnoughPowerSupplied(bool *out);
Result psmGetBatteryAgePercentage(double *out);

/**
 * @brief Wrapper func which opens a PsmSession and handles event setup.
 * @note Uses the actual BindStateChangeEvent cmd internally.
 * @note The event is not signalled on BatteryChargePercentage changes.
 * @param[out] s PsmSession object.
 * @param[in] ChargerType Passed to SetChargerTypeChangeEventEnabled.
 * @param[in] PowerSupply Passed to SetPowerSupplyChangeEventEnabled.
 * @param[in] BatteryVoltage Passed to SetBatteryVoltageStateChangeEventEnabled.
 */
Result psmBindStateChangeEvent(PsmSession* s, bool ChargerType, bool PowerSupply, bool BatteryVoltage);

/// Wait on the Event setup by \ref psmBindStateChangeEvent.
Result psmWaitStateChangeEvent(PsmSession* s, u64 timeout);

/// Cleanup version of \ref psmBindStateChangeEvent. Must be called by the user once the PsmSession is done being used.
Result psmUnbindStateChangeEvent(PsmSession* s);
