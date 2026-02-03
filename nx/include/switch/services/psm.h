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

/// ChargerType
typedef enum {
    PsmChargerType_Unconnected = 0,  ///< No charger
    PsmChargerType_EnoughPower = 1,  ///< Full supported power
    PsmChargerType_LowPower = 2,     ///< Lower power supported USB-PD mode
    PsmChargerType_NotSupported = 3, ///< No common supported USB-PD modes
} PsmChargerType;

/// Vdd50State
typedef enum {
    PsmVdd50State_Unknown = 0,
    PsmVdd50State_Vdd50AOffVdd50BOff = 1,
    PsmVdd50State_Vdd50AOnVdd50BOff = 2,
    PsmVdd50State_Vdd50AOffVdd50BOn = 3,
} PsmVdd50State;

/// BatteryVoltageState
typedef enum {
    PsmBatteryVoltageState_NeedsShutdown = 0,      ///< Power state should transition to shutdown
    PsmBatteryVoltageState_NeedsSleep = 1,         ///< Power state should transition to sleep
    PsmBatteryVoltageState_NoPerformanceBoost = 2, ///< Performance boost modes cannot be entered
    PsmBatteryVoltageState_Normal = 3,             ///< Everything is normal
} PsmBatteryVoltageState;

/// BatteryChargeInfoFieldsOld [1.0.0-16.1.0]
typedef struct {
    u32 input_current_limit;
    u32 boost_mode_current_limit;
    u32 fast_charge_current_limit;
    u32 charge_voltage_limit;
    PsmChargerType charger_type;
    u8 hi_z_mode;
    bool battery_charging;
    u8 pad[2];
    PsmVdd50State vdd50_state;
    u32 temperature_celcius;
    u32 battery_charge_percentage;
    u32 battery_charge_milli_voltage;
    u32 battery_age_percentage;
    u32 usb_power_role;
    u32 usb_charger_type;
    u32 charger_input_voltage_limit;
    u32 charger_input_current_limit;
    bool fast_battery_charging;
    bool controller_power_supply;
    bool otg_request;
    u8 reserved;
} PsmBatteryChargeInfoFieldsOld;

/// BatteryChargeInfoFields [17.0.0+]
typedef struct {
    u32 input_current_limit;          ///< Input (Sink) current limit in mA
    u32 boost_mode_current_limit;     ///< Output (Source/VBUS/OTG) current limit in mA
    u32 fast_charge_current_limit;    ///< Battery charging current limit in mA
    u32 charge_voltage_limit;         ///< Battery charging voltage limit in mV
    PsmChargerType charger_type;
    u8 hi_z_mode;
    bool battery_charging;
    u8 pad[2];
    PsmVdd50State vdd50_state;        ///< Power Delivery Controller State
    u32 temperature_celcius;          ///< Battery temperature in milli C
    u32 battery_charge_percentage;    ///< Raw battery charged capacity per cent-mille
    u32 battery_charge_milli_voltage; ///< Voltage average in mV
    u32 battery_age_percentage;       ///< Battery age per cent-mille
    u32 usb_power_role;
    u32 usb_charger_type;
    u32 charger_input_voltage_limit;  ///< Charger and external device voltage limit in mV
    u32 charger_input_current_limit;  ///< Charger and external device current limit in mA
    bool fast_battery_charging;
    bool controller_power_supply;
    bool otg_request;
    u8 reserved;
    u8 unk_x40[0x14];
} PsmBatteryChargeInfoFields;

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

/**
 * @brief GetBatteryChargePercentage
 * @param[out] out Battery charge percentage.
 */
Result psmGetBatteryChargePercentage(u32 *out);

/**
 * @brief GetChargerType
 * @param[out] out \ref PsmChargerType
 */
Result psmGetChargerType(PsmChargerType *out);

/**
 * @brief EnableBatteryCharging
 */
Result psmEnableBatteryCharging(void);

/**
 * @brief DisableBatteryCharging
 */
Result psmDisableBatteryCharging(void);

/**
 * @brief IsBatteryChargingEnabled
 * @param[out] out Output flag.
 */
Result psmIsBatteryChargingEnabled(bool *out);

/**
 * @brief AcquireControllerPowerSupply
 */
Result psmAcquireControllerPowerSupply(void);

/**
 * @brief ReleaseControllerPowerSupply
 */
Result psmReleaseControllerPowerSupply(void);

/**
 * @brief EnableEnoughPowerChargeEmulation
 */
Result psmEnableEnoughPowerChargeEmulation(void);

/**
 * @brief DisableEnoughPowerChargeEmulation
 */
Result psmDisableEnoughPowerChargeEmulation(void);

/**
 * @brief EnableFastBatteryCharging
 */
Result psmEnableFastBatteryCharging(void);

/**
 * @brief DisableFastBatteryCharging
 */
Result psmDisableFastBatteryCharging(void);

/**
 * @brief GetBatteryVoltageState
 * @param[out] out \ref PsmBatteryVoltageState
 */
Result psmGetBatteryVoltageState(PsmBatteryVoltageState *out);

/**
 * @brief GetRawBatteryChargePercentage
 * @param[out] out Raw battery charge percentage.
 */
Result psmGetRawBatteryChargePercentage(double *out);

/**
 * @brief IsEnoughPowerSupplied
 * @param[out] out Output flag.
 */
Result psmIsEnoughPowerSupplied(bool *out);

/**
 * @brief GetBatteryAgePercentage
 * @param[out] out Battery age percentage.
 */
Result psmGetBatteryAgePercentage(double *out);

/**
 * @brief GetBatteryChargeInfoEvent
 * @param[out] out_event Event object.
 * @param[in] autoclear Event autoclear.
 */
Result psmGetBatteryChargeInfoEvent(Event* out_event, bool autoclear);

/**
 * @brief GetBatteryChargeInfoFields
 * @param[out] out_fields \ref PsmBatteryChargeInfoFields
 */
Result psmGetBatteryChargeInfoFields(PsmBatteryChargeInfoFields *out_fields);

/**
 * @brief GetBatteryChargeCalibratedEvent
 * @note Only available on [3.0.0+].
 * @param[out] out_event Event object.
 * @param[in] autoclear Event autoclear.
 */
Result psmGetBatteryChargeCalibratedEvent(Event* out_event, bool autoclear);

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
