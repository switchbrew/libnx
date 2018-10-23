// Copyright 2017 plutoo
#include "types.h"
#include "kernel/detect.h"
#include "kernel/mutex.h"
#include "kernel/svc.h"

bool g_IsAbove200;
bool g_IsAbove300;
bool g_IsAbove400;
bool g_IsAbove500;
bool g_IsAbove600;

void detectSetup(void)
{
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
}

bool detectDebugger(void) {
    u64 tmp;
    svcGetInfo(&tmp, 8, 0, 0);
    return !!tmp;
}
