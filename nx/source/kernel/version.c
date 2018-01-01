// Copyright 2017 plutoo
#include <switch.h>

bool kernelAbove200() {
    u64 tmp;
    return svcGetInfo(&tmp, 12, INVALID_HANDLE, 0) != 0xF001;
}

bool kernelAbove300() {
    u64 tmp;
    return svcGetInfo(&tmp, 18, INVALID_HANDLE, 0) != 0xF001;
}

bool kernelAbove400() {
    u64 tmp;
    return svcGetInfo(&tmp, 19, INVALID_HANDLE, 0) != 0xF001;
}
