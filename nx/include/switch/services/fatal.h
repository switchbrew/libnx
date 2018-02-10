/**
 * @file fatal.h
 * @brief Fatal error (fatal:u) service IPC wrapper.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Triggers a system fatal error.
 * @param err[in] Result code to throw.
 * @note This function does not return.
 */
void NORETURN fatalSimple(Result err);
