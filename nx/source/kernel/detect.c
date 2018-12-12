// Copyright 2017 plutoo
#include "types.h"
#include "kernel/detect.h"
#include "kernel/mutex.h"
#include "kernel/svc.h"

static bool g_IsAbove200;
static bool g_IsAbove300;
static bool g_IsAbove400;
static bool g_IsAbove500;
static bool g_IsAbove600;
static bool g_HasCached = 0;
static Mutex g_Mutex;

static void _CacheValues(void)
{
    if (__atomic_load_n(&g_HasCached, __ATOMIC_SEQ_CST))
        return;

    mutexLock(&g_Mutex);

    if (g_HasCached) {
        mutexUnlock(&g_Mutex);
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

    __atomic_store_n(&g_HasCached, true, __ATOMIC_SEQ_CST);

    mutexUnlock(&g_Mutex);
}

bool kernelAbove200(void) {
    _CacheValues();
    return g_IsAbove200;
}

bool kernelAbove300(void) {
    _CacheValues();
    return g_IsAbove300;
}

bool kernelAbove400(void) {
    _CacheValues();
    return g_IsAbove400;
}

bool kernelAbove500(void) {
    _CacheValues();
    return g_IsAbove500;
}

bool kernelAbove600(void) {
    _CacheValues();
    return g_IsAbove600;
}

bool detectDebugger(void) {
    u64 tmp;
    svcGetInfo(&tmp, 8, 0, 0);
    return !!tmp;
}
