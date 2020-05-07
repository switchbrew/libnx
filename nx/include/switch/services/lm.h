/**
 * @file lm.h
 * @brief Log manager (lm) service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"

/// LmLogDestination
typedef enum {
    LmLogDestination_TMA = BIT(0),           ///< Logs to TMA.
    LmLogDestination_UART = BIT(1),          ///< Logs to UART.
    LmLogDestination_UARTSleeping = BIT(2),  ///< Logs to UART (when sleeping).
    LmLogDestination_All = 0xFFFF,           ///< Logs to all locations.
} LmLogDestination;

/// Initialize lm. This is stubbed on retail LogManager, always succeeding. This service is automatically initialized and exited by diag.
Result lmInitialize(void);

/// Exit lm.
void lmExit(void);

/// Gets the Service object for the actual lm service session.
Service* lmGetServiceSession(void);

/// Gets the Service object for ILogger.
Service* lmGetServiceSession_Logger(void);

/**
 * @brief Logs sent data.
 * @note Check diag for a logging implementation over this service.
 * @param[in] buf Log buffer.
 * @param[in] buf_size Log buffer size.
 */
Result lmLog(const void *buf, size_t buf_size);

/**
 * @brief Sets the log destination.
 * @param[in] destination Log destination.
 */
Result lmSetDestination(LmLogDestination destination);
