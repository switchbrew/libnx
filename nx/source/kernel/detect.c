// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "kernel/detect.h"
#include "kernel/mutex.h"
#include "kernel/svc.h"
#include <malloc.h>

static bool g_VersionCached = 0;
static Mutex g_VersionMutex;
static bool g_IsAbove200;
static bool g_IsAbove300;
static bool g_IsAbove400;
static bool g_IsAbove500;
static bool g_IsAbove600;

static bool g_CfwJitCached = 0;
static Mutex g_CfwJitMutex;
static bool g_CfwJitPatchDetected;

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
    g_IsAbove200 = (svcGetInfo(&tmp, 12, INVALID_HANDLE, 0) != 0xF001);
    g_IsAbove300 = (svcGetInfo(&tmp, 18, INVALID_HANDLE, 0) != 0xF001);
    g_IsAbove400 = (svcGetInfo(&tmp, 19, INVALID_HANDLE, 0) != 0xF001);
    g_IsAbove500 = (svcGetInfo(&tmp, 20, INVALID_HANDLE, 0) != 0xF001);
    g_IsAbove600 = (svcGetInfo(&tmp, 21, INVALID_HANDLE, 0) != 0xF001);

    g_IsAbove500 |= g_IsAbove600;
    g_IsAbove400 |= g_IsAbove500;
    g_IsAbove300 |= g_IsAbove400;
    g_IsAbove200 |= g_IsAbove300;

    __atomic_store_n(&g_VersionCached, true, __ATOMIC_SEQ_CST);

    mutexUnlock(&g_VersionMutex);
}

static void _CacheCfwJit(void)
{
    if (__atomic_load_n(&g_CfwJitCached, __ATOMIC_SEQ_CST))
        return;

    mutexLock(&g_CfwJitMutex);

    if (g_CfwJitCached) {
        mutexUnlock(&g_CfwJitMutex);
        return;
    }

    void* heap = memalign(0x1000, 0x1000);

    if (heap != NULL)
    {
	Handle code;
	Result rc;
	rc = svcCreateCodeMemory(&code, heap, 0x1000);

	if (R_SUCCEEDED(rc))
	{
	    // On an unpatched kernel on 5.0.0 and above, this would return 0xD401.
	    // It is not allowed for the creator-process of a CodeMemory object to use svcControlCodeMemory on it.
	    // If the patch is present, the function should return 0xF001, because -1 is not a valid enum CodeOperation.
	    rc = svcControlCodeMemory(code, -1, 0, 0x1000, 0);

	    g_CfwJitPatchDetected = (rc == 0xF001);
	    __atomic_store_n(&g_CfwJitCached, true, __ATOMIC_SEQ_CST);

	    svcCloseHandle(code);
	}

	free(heap);
    }

    mutexUnlock(&g_CfwJitMutex);
}

bool kernelAbove200(void) {
    _CacheVersion();
    return g_IsAbove200;
}

bool kernelAbove300(void) {
    _CacheVersion();
    return g_IsAbove300;
}

bool kernelAbove400(void) {
    _CacheVersion();
    return g_IsAbove400;
}

bool kernelAbove500(void) {
    _CacheVersion();
    return g_IsAbove500;
}

bool kernelAbove600(void) {
    _CacheVersion();
    return g_IsAbove600;
}

bool detectDebugger(void) {
    u64 tmp;
    svcGetInfo(&tmp, 8, 0, 0);
    return !!tmp;
}

bool detectCfwJitPatch(void) {
    _CacheCfwJit();
    return g_CfwJitPatchDetected;
}

void detectPretendNotCfwForTesting(void) {
    mutexLock(&g_CfwJitMutex);
    g_CfwJitPatchDetected = false;
    __atomic_store_n(&g_CfwJitCached, true, __ATOMIC_SEQ_CST);
    mutexUnlock(&g_CfwJitMutex);
}
