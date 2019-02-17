/**
 * @file detect.h
 * @brief Kernel capability detection
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../runtime/hosversion.h"

/// Returns the lowest known kernel version that is compatible with observation.
u32 detectKernelVersion(void);

/// Returns the highest possible known kernel version that is compatible with observation.
u32 detectKernelVersionUpperBound(void);

/// Returns true if the process has a debugger attached.
bool detectDebugger(void);
/// Returns true if the kernel is patched to allow self-process-jit.
bool detectJitKernelPatch(void);
/// After this has been called, libnx will ignore the self-process-jit kernel patch. For testing purposes only.
void detectIgnoreJitKernelPatch(void);

/// Returns true if the kernel version is equal to or above 2.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove200(void) {
    return detectKernelVersion() >= MAKEHOSVERSION(2,0,0);
}

/// Returns true if the kernel version is equal to or above 3.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove300(void) {
    return detectKernelVersion() >= MAKEHOSVERSION(3,0,0);
}

/// Returns true if the kernel version is equal to or above 4.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove400(void) {
    return detectKernelVersion() >= MAKEHOSVERSION(4,0,0);
}

/// Returns true if the kernel version is equal to or above 5.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove500(void) {
    return detectKernelVersion() >= MAKEHOSVERSION(5,0,0);
}

/// Returns true if the kernel version is equal to or above 6.0.0. Generally, \ref hosversionAtLeast should be used instead of this function.
static inline bool kernelAbove600(void) {
    return detectKernelVersion() >= MAKEHOSVERSION(6,0,0);
}
