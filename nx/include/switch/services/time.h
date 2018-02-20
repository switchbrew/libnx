/**
 * @file time.h
 * @brief Time services IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../services/sm.h"

typedef enum {
    TimeType_UserSystemClock,
    TimeType_NetworkSystemClock,
    TimeType_LocalSystemClock,
    TimeType_Default = TimeType_NetworkSystemClock,
} TimeType;

Result timeInitialize(void);
void timeExit(void);

Service* timeGetSessionService(void);

Result timeGetCurrentTime(TimeType type, u64 *timestamp);
