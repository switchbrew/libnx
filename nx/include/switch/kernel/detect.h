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
/// Returns true if the kernel version is equal to or above 4.0.0.
bool kernelAbove400(void);
/// Returns true if the kernel version is equal to or above 5.0.0.
bool kernelAbove500(void);
/// Returns true if the process has a debugger attached.
bool detectDebugger(void);
