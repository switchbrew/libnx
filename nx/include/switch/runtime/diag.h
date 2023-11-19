/**
 * @file diag.h
 * @brief Debugging and diagnostics utilities
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../result.h"

/**
 * @brief Aborts program execution with a result code.
 * @param[in] res Result code.
 */
void NX_NORETURN diagAbortWithResult(Result res);
