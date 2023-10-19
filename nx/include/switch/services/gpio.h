/**
 * @file gpio.h
 * @brief GPIO service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

typedef enum {
    GpioPadName_AudioCodec    = 1,
    GpioPadName_ButtonVolUp   = 25,
    GpioPadName_ButtonVolDown = 26,
    GpioPadName_SdCd          = 56,
} GpioPadName;

typedef struct {
    Service s;
} GpioPadSession;

typedef enum {
    GpioDirection_Input  = 0,
    GpioDirection_Output = 1,
} GpioDirection;

typedef enum {
    GpioValue_Low  = 0,
    GpioValue_High = 1,
} GpioValue;

typedef enum {
    GpioInterruptMode_LowLevel    = 0,
    GpioInterruptMode_HighLevel   = 1,
    GpioInterruptMode_RisingEdge  = 2,
    GpioInterruptMode_FallingEdge = 3,
    GpioInterruptMode_AnyEdge     = 4,
} GpioInterruptMode;

typedef enum {
    GpioInterruptStatus_Inactive = 0,
    GpioInterruptStatus_Active   = 1,
} GpioInterruptStatus;

/// Initialize gpio.
Result gpioInitialize(void);

/// Exit gpio.
void gpioExit(void);

/// Gets the Service object for the actual gpio service session.
Service* gpioGetServiceSession(void);

Result gpioOpenSession(GpioPadSession *out, GpioPadName name);
Result gpioOpenSession2(GpioPadSession *out, u32 device_code, u32 access_mode);

Result gpioIsWakeEventActive(bool *out, GpioPadName name);
Result gpioIsWakeEventActive2(bool *out, u32 device_code);

Result gpioPadSetDirection(GpioPadSession *p, GpioDirection dir);
Result gpioPadGetDirection(GpioPadSession *p, GpioDirection *out);
Result gpioPadSetInterruptMode(GpioPadSession *p, GpioInterruptMode mode);
Result gpioPadGetInterruptMode(GpioPadSession *p, GpioInterruptMode *out);
Result gpioPadSetInterruptEnable(GpioPadSession *p, bool en);
Result gpioPadGetInterruptEnable(GpioPadSession *p, bool *out);
Result gpioPadGetInterruptStatus(GpioPadSession *p, GpioInterruptStatus *out); ///< [1.0.0-16.1.0]
Result gpioPadClearInterruptStatus(GpioPadSession *p); ///< [1.0.0-16.1.0]
Result gpioPadSetValue(GpioPadSession *p, GpioValue val);
Result gpioPadGetValue(GpioPadSession *p, GpioValue *out);
Result gpioPadBindInterrupt(GpioPadSession *p, Event *out);
Result gpioPadUnbindInterrupt(GpioPadSession *p);
Result gpioPadSetDebounceEnabled(GpioPadSession *p, bool en);
Result gpioPadGetDebounceEnabled(GpioPadSession *p, bool *out);
Result gpioPadSetDebounceTime(GpioPadSession *p, s32 ms);
Result gpioPadGetDebounceTime(GpioPadSession *p, s32 *out);
void gpioPadClose(GpioPadSession *p);
