/**
 * @file pwm.h
 * @author MasaGratoR
 * @copyright libnx Authors
 */

#pragma once

#include "../types.h"
#include "../sf/service.h"

typedef struct {
    Service s;
} PwmChannelSession;

typedef enum {
    PwmChannelDeviceCode_CpuFan       = 0x3D000001,
    PwmChannelDeviceCode_LcdBacklight = 0x3400003D,
    PwmChannelDeviceCode_Led          = 0x35000065
} PwmChannelDeviceCode;

/// Initialize pwm.
Result pwmInitialize(void);

/// Exit pwm.
void pwmExit(void);

/// Gets the Service for pwm.
Service* pwmGetServiceSession(void);

/// Takes a PwmChannelDeviceCode and returns a PwmChannelSession. Only available on [6.0.0+].
Result pwmOpenSession2(PwmChannelSession *out, PwmChannelDeviceCode device_code);

/// Takes a PwmChannelSession, returns an output double. Only available on [6.0.0+].
Result pwmChannelSessionGetDutyCycle(PwmChannelSession *c, double* out);

/// Closes a PwmChannelSession.
void pwmChannelSessionClose(PwmChannelSession *c);
