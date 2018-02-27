/**
 * @file time.h
 * @brief Time services IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../services/sm.h"

/// Time clock type.
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

/**
 * @brief Sets the time for the specified clock.
 * @param[in] type Clock to use.
 * @param[in] timestamp POSIX UTC timestamp.
 * @return Result code.
 */
Result timeSetCurrentTime(TimeType type, u64 timestamp);
