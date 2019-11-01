/**
 * @file i2c.h
 * @brief I2C service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    I2cDevice_DebugPad          = 0,
    I2cDevice_TouchPanel        = 1,
    I2cDevice_Tmp451            = 2,
    I2cDevice_Nct72             = 3,
    I2cDevice_Alc5639           = 4,
    I2cDevice_Max77620Rtc       = 5,
    I2cDevice_Max77620Pmic      = 6,
    I2cDevice_Max77621Cpu       = 7,
    I2cDevice_Max77621Gpu       = 8,
    I2cDevice_Bq24193           = 9,
    I2cDevice_Max17050          = 10,
    I2cDevice_Bm92t30mwv        = 11,
    I2cDevice_Ina226Vdd15v0Hb   = 12,
    I2cDevice_Ina226VsysCpuDs   = 13,
    I2cDevice_Ina226VsysGpuDs   = 14,
    I2cDevice_Ina226VsysDdrDs   = 15,
    I2cDevice_Ina226VsysAp      = 16,
    I2cDevice_Ina226VsysBlDs    = 17,
    I2cDevice_Bh1730            = 18,
    I2cDevice_Ina226VsysCore    = 19,
    I2cDevice_Ina226Soc1V8      = 20,
    I2cDevice_Ina226Lpddr1V8    = 21,
    I2cDevice_Ina226Reg1V32     = 22,
    I2cDevice_Ina226Vdd3V3Sys   = 23,
    I2cDevice_HdmiDdc           = 24,
    I2cDevice_HdmiScdc          = 25,
    I2cDevice_HdmiHdcp          = 26,
    I2cDevice_Fan53528          = 27,
    I2cDevice_Max77812_3        = 28,
    I2cDevice_Max77812_2        = 29,
    I2cDevice_Ina226VddDdr0V6   = 30,

    I2cDevice_Count,
} I2cDevice;

typedef struct {
    Service s;
} I2cSession;

typedef enum {
    I2cTransactionOption_Start = (1 << 0),
    I2cTransactionOption_Stop = (1 << 1),

    I2cTransactionOption_All = I2cTransactionOption_Start | I2cTransactionOption_Stop,
} I2cTransactionOption;

/// Initialize i2c.
Result i2cInitialize(void);

/// Exit i2c.
void i2cExit(void);

/// Gets the Service object for the actual i2c service session.
Service* i2cGetServiceSession(void);

Result i2cOpenSession(I2cSession *out, I2cDevice dev);

Result i2csessionSendAuto(I2cSession *s, const void *buf, size_t size, I2cTransactionOption option);
Result i2csessionReceiveAuto(I2cSession *s, void *buf, size_t size, I2cTransactionOption option);
Result i2csessionExecuteCommandList(I2cSession *s, void *dst, size_t dst_size, const void *cmd_list, size_t cmd_list_size);
void i2csessionClose(I2cSession *s);
