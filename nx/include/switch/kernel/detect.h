/**
 * @file detect.h
 * @brief Kernel capability detection
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "svc.h"

/// Returns true if the process has a debugger attached.
NX_INLINE bool detectDebugger(void) {
    u64 tmp = 0;
    Result rc = svcGetInfo(&tmp, InfoType_DebuggerAttached, INVALID_HANDLE, 0);
    return R_SUCCEEDED(rc) && tmp != 0;
}
