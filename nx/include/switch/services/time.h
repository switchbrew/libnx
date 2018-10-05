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
    TimeType_Default = TimeType_UserSystemClock,
} TimeType;

typedef struct {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 pad;
} TimeCalendarTime;

typedef struct {
    u32 wday; ///< 0-based day-of-week.
    u32 yday; ///< 0-based day-of-year.
    char timezoneName[8]; ///< Timezone name string.
    u32 DST; ///< 0 = no DST, 1 = DST.
    s32 offset; ///< Seconds relative to UTC for this timezone.
} TimeCalendarAdditionalInfo;

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

Result timeToCalendarTimeWithMyRule(u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info);

