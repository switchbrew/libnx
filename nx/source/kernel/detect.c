// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "kernel/detect.h"
#include "kernel/mutex.h"
#include "kernel/svc.h"
#include <malloc.h>

static bool g_VersionCached;
static Mutex g_VersionMutex;
static int g_Version;

static bool g_JitKernelPatchCached;
static Mutex g_JitKernelPatchMutex;
static bool g_JitKernelPatchDetected;

static void _CacheVersion(void)
{
    if (__atomic_load_n(&g_VersionCached, __ATOMIC_SEQ_CST))
        return;

    mutexLock(&g_VersionMutex);

    if (g_VersionCached) {
        mutexUnlock(&g_VersionMutex);
        return;
    }

    u64 tmp;
    g_Version = 1;
    if (R_VALUE(svcGetInfo(&tmp, InfoType_AslrRegionAddress, INVALID_HANDLE, 0)) != KERNELRESULT(InvalidEnumValue))
        g_Version = 2;
    if (R_VALUE(svcGetInfo(&tmp, InfoType_TitleId, INVALID_HANDLE, 0)) != KERNELRESULT(InvalidEnumValue))
        g_Version = 3;
    if (R_VALUE(svcGetInfo(&tmp, InfoType_InitialProcessIdRange, INVALID_HANDLE, 0)) != KERNELRESULT(InvalidEnumValue))
        g_Version = 4;
    if (R_VALUE(svcGetInfo(&tmp, InfoType_UserExceptionContextAddress, INVALID_HANDLE, 0)) != KERNELRESULT(InvalidEnumValue))
        g_Version = 5;
    if (R_VALUE(svcGetInfo(&tmp, InfoType_TotalNonSystemMemorySize, INVALID_HANDLE, 0)) != KERNELRESULT(InvalidEnumValue))
        g_Version = 6;

    __atomic_store_n(&g_VersionCached, true, __ATOMIC_SEQ_CST);

    mutexUnlock(&g_VersionMutex);
}

static void _CacheJitKernelPatch(void)
{
    if (__atomic_load_n(&g_JitKernelPatchCached, __ATOMIC_SEQ_CST))
        return;

    mutexLock(&g_JitKernelPatchMutex);

    if (g_JitKernelPatchCached) {
        mutexUnlock(&g_JitKernelPatchMutex);
        return;
    }

    void* heap = memalign(0x1000, 0x1000);

    if (heap != NULL) {
        Handle code;
        Result rc;
        rc = svcCreateCodeMemory(&code, heap, 0x1000);

        if (R_SUCCEEDED(rc)) {
            // On an unpatched kernel on 5.0.0 and above, this would return InvalidMemoryState (0xD401).
            // It is not allowed for the creator-process of a CodeMemory object to use svcControlCodeMemory on it.
            // If the patch is present, the function should return InvalidEnumValue (0xF001), because -1 is not a valid enum CodeOperation.
            rc = svcControlCodeMemory(code, -1, 0, 0x1000, 0);

            g_JitKernelPatchDetected = R_VALUE(rc) == KERNELRESULT(InvalidEnumValue);
            __atomic_store_n(&g_JitKernelPatchCached, true, __ATOMIC_SEQ_CST);

            svcCloseHandle(code);
        }

        free(heap);
    }

    mutexUnlock(&g_JitKernelPatchMutex);
}

int detectKernelVersion(void) {
    _CacheVersion();
    return g_Version;
}

bool detectDebugger(void) {
    u64 tmp;
    svcGetInfo(&tmp, InfoType_DebuggerAttached, INVALID_HANDLE, 0);
    return !!tmp;
}

bool detectJitKernelPatch(void) {
    _CacheJitKernelPatch();
    return g_JitKernelPatchDetected;
}

void detectIgnoreJitKernelPatch(void) {
    mutexLock(&g_JitKernelPatchMutex);
    g_JitKernelPatchDetected = false;
    __atomic_store_n(&g_JitKernelPatchCached, true, __ATOMIC_SEQ_CST);
    mutexUnlock(&g_JitKernelPatchMutex);
}
