/**
 * @file detect.h
 * @brief Kernel version detection
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Returns true if the kernel version is equal to or above 2.0.0.
bool kernelAbove200(void);
/// Returns true if the kernel version is equal to or above 3.0.0.
bool kernelAbove300(void);
/// Returns true if the kernel version is equal to or above 2.0.0.
bool kernelAbove400(void);
/// Returns true if code is running under a debugger.
bool detectDebugger(void);
