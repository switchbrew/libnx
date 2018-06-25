/**
 * @file fatal.h
 * @brief Fatal error (fatal:u) service IPC wrapper.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum {
    FatalType_ErrorReportAndErrorScreen = 0,
    FatalType_ErrorReport = 1,
    FatalType_ErrorScreen = 2
} FatalType;

/**
 * @brief Triggers a system fatal error.
 * @param err[in] Result code to throw.
 * @note This function does not return.
 */
void NORETURN fatalSimple(Result err);

/**
 * @brief Triggers a system fatal error with a custom FatalType.
 * @param err[in] Result code to throw.
 * @param err[in] Type of fatal error to throw.
 * @note This function does not return.
 */
void NORETURN fatalWithType(Result err, FatalType type);
