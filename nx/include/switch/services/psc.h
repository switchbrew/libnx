/**
 * @file psc.h
 * @brief PSC service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

typedef enum {
    PscPmState_Awake = 0,               ///< Everything is awake.
    PscPmState_ReadyAwaken = 1,         ///< Preparing to transition to awake.
    PscPmState_ReadySleep = 2,          ///< Preparing to transition to sleep.
    PscPmState_ReadySleepCritical = 3,  ///< Critical services are ready to sleep.
    PscPmState_ReadyAwakenCritical = 4, ///< Critical services are ready to wake up.
    PscPmState_ReadyShutdown = 5,       ///< Preparing to transition to shutdown.
} PscPmState;

typedef enum {
    PscPmModuleId_Usb           = 4,
    PscPmModuleId_Ethernet      = 5,
    PscPmModuleId_Fgm           = 6,
    PscPmModuleId_PcvClock      = 7,
    PscPmModuleId_PcvVoltage    = 8,
    PscPmModuleId_Gpio          = 9,
    PscPmModuleId_Pinmux        = 10,
    PscPmModuleId_Uart          = 11,
    PscPmModuleId_I2c           = 12,
    PscPmModuleId_I2cPcv        = 13,
    PscPmModuleId_Spi           = 14,
    PscPmModuleId_Pwm           = 15,
    PscPmModuleId_Psm           = 16,
    PscPmModuleId_Tc            = 17,
    PscPmModuleId_Omm           = 18,
    PscPmModuleId_Pcie          = 19,
    PscPmModuleId_Lbl           = 20,
    PscPmModuleId_Display       = 21,

    PscPmModuleId_Hid           = 24,
    PscPmModuleId_WlanSockets   = 25,

    PscPmModuleId_Fs            = 27,
    PscPmModuleId_Audio         = 28,

    PscPmModuleId_TmaHostIo     = 30,
    PscPmModuleId_Bluetooth     = 31,
    PscPmModuleId_Bpc           = 32,
    PscPmModuleId_Fan           = 33,
    PscPmModuleId_Pcm           = 34,
    PscPmModuleId_Nfc           = 35,
    PscPmModuleId_Apm           = 36,
    PscPmModuleId_Btm           = 37,
    PscPmModuleId_Nifm          = 38,
    PscPmModuleId_GpioLow       = 39,
    PscPmModuleId_Npns          = 40,
    PscPmModuleId_Lm            = 41,
    PscPmModuleId_Bcat          = 42,
    PscPmModuleId_Time          = 43,
    PscPmModuleId_Pctl          = 44,
    PscPmModuleId_Erpt          = 45,
    PscPmModuleId_Eupld         = 46,
    PscPmModuleId_Friends       = 47,
    PscPmModuleId_Bgtc          = 48,
    PscPmModuleId_Account       = 49,
    PscPmModuleId_Sasbus        = 50,
    PscPmModuleId_Ntc           = 51,
    PscPmModuleId_Idle          = 52,
    PscPmModuleId_Tcap          = 53,
    PscPmModuleId_PsmLow        = 54,
    PscPmModuleId_Ndd           = 55,
    PscPmModuleId_Olsc          = 56,

    PscPmModuleId_Ns            = 61,

    PscPmModuleId_Nvservices    = 101,

    PscPmModuleId_Spsm          = 127,
} PscPmModuleId;

typedef struct {
    Event event;
    Service srv;
    PscPmModuleId module_id;
} PscPmModule;

/// Initialize psc:m.
Result pscmInitialize(void);

/// Exit psc:m.
void pscmExit(void);

/// Gets the Service object for the actual psc:m service session.
Service* pscmGetServiceSession(void);

Result pscmGetPmModule(PscPmModule *out, PscPmModuleId module_id, const u16 *dependencies, size_t dependency_count, bool autoclear);

Result pscPmModuleGetRequest(PscPmModule *module, PscPmState *out_state, u32 *out_flags);
Result pscPmModuleAcknowledge(PscPmModule *module, PscPmState state);
Result pscPmModuleFinalize(PscPmModule *module);

void   pscPmModuleClose(PscPmModule *module);
