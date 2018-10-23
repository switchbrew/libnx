/**
 * @file detect.h
 * @brief Kernel version detection
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

extern bool g_IsAbove200;
extern bool g_IsAbove300;
extern bool g_IsAbove400;
extern bool g_IsAbove500;
extern bool g_IsAbove600;

/// Returns true if the kernel version is equal to or above 2.0.0.
static inline bool kernelAbove200(void) {
    return g_IsAbove200;
}

/// Returns true if the kernel version is equal to or above 3.0.0.
static inline bool kernelAbove300(void) {
    return g_IsAbove300;
}

/// Returns true if the kernel version is equal to or above 4.0.0.
static inline bool kernelAbove400(void) {
    return g_IsAbove400;
}

/// Returns true if the kernel version is equal to or above 5.0.0.
static inline bool kernelAbove500(void) {
    return g_IsAbove500;
}

/// Returns true if the kernel version is equal to or above 6.0.0.
static inline bool kernelAbove600(void) {
    return g_IsAbove600;
}

/// Returns true if the process has a debugger attached.
bool detectDebugger(void);

