// Copyright 2017 plutoo
#include "types.h"
#include "kernel/detect.h"
#include "kernel/svc.h"

static bool g_IsAbove200;
static bool g_IsAbove300;
static bool g_IsAbove400;
static bool g_HasCached = 0;

static void _CacheValues(void)
{
    // This is actually thread safe, might cache twice but that's fine.
    if (!g_HasCached)
    {
        u64 tmp;
        g_IsAbove200 = (svcGetInfo(&tmp, 12, INVALID_HANDLE, 0) != 0xF001);
        g_IsAbove300 = (svcGetInfo(&tmp, 18, INVALID_HANDLE, 0) != 0xF001);
        g_IsAbove400 = (svcGetInfo(&tmp, 19, INVALID_HANDLE, 0) != 0xF001);
        g_HasCached = true;
    }
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

bool detectDebugger(void) {
    u64 tmp;
    svcGetInfo(&tmp, 8, 0, 0);
    return !!tmp;
}
