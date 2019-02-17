/**
 * @file detect.h
 * @brief Kernel capability detection
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Returns the kernel version that can be detected by checking kernel capabilities. This only goes from 1 (representing 1.0.0) up to 6 (representing 6.0.0 and above). Generally, \ref hosversionGet should be used instead of this function.
int detectKernelVersion(void);
/// Returns true if the process has a debugger attached.
bool detectDebugger(void);
/// Returns true if the kernel is patched to allow self-process-jit.
bool detectJitKernelPatch(void);
/// After this has been called, libnx will ignore the self-process-jit kernel patch. For testing purposes only.
void detectIgnoreJitKernelPatch(void);

/// Returns true if the kernel version is equal to or above 2.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove200(void) {
    return detectKernelVersion() >= 2;
}

/// Returns true if the kernel version is equal to or above 3.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove300(void) {
    return detectKernelVersion() >= 3;
}

/// Returns true if the kernel version is equal to or above 4.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove400(void) {
    return detectKernelVersion() >= 4;
}

/// Returns true if the kernel version is equal to or above 5.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove500(void) {
    return detectKernelVersion() >= 5;
}

/// Returns true if the kernel version is equal to or above 6.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove600(void) {
    return detectKernelVersion() >= 6;
}
