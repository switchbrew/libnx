/**
 * @file fatal.h
 * @brief Fatal error (fatal:u) service IPC wrapper.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Type of thrown fatal error.
typedef enum {
    FatalType_ErrorReportAndErrorScreen = 0,
    FatalType_ErrorReport = 1,
    FatalType_ErrorScreen = 2 ///< Only available with 3.0.0+. If specified, FatalType_ErrorReportAndErrorScreen will be used instead on pre-3.0.0.
} FatalType;

/**
 * @brief Triggers a system fatal error.
 * @param err[in] Result code to throw.
 * @note This function does not return.
 * @note This uses \ref fatalWithType with \ref FatalType_ErrorScreen internally.
 */
void NORETURN fatalSimple(Result err);

/**
 * @brief Triggers a system fatal error with a custom \ref FatalType.
 * @param err[in] Result code to throw.
 * @param err[in] Type of fatal error to throw.
 * @note This function may not return, depending on \ref FatalType.
 */
void fatalWithType(Result err, FatalType type);
