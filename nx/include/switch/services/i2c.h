/**
 * @file i2c.h
 * @brief I2C service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "sm.h"

typedef enum {
    I2cDevice_AudioCodec = 4,
} I2cDevice;

typedef struct {
    Service s;
} I2cSession;

typedef enum {
    I2cTransactionOption_Start = (1 << 0),
    I2cTransactionOption_Stop = (1 << 1),

    I2cTransactionOption_All = I2cTransactionOption_Start | I2cTransactionOption_Stop,
} I2cTransactionOption;

Result i2cInitialize(void);
void i2cExit(void);

Result i2cOpenSession(I2cSession *out, I2cDevice dev);

Result i2csessionSendAuto(I2cSession *s, void *buf, size_t size, I2cTransactionOption option);
void i2csessionClose(I2cSession *s);
