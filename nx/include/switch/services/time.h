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

typedef struct {
    u8 data[0x4000];
} TimeZoneRule;

typedef struct {
    char name[0x24];
} TimeLocationName;

Result timeInitialize(void);
void timeExit(void);

Service* timeGetServiceSession(void);

Result timeGetCurrentTime(TimeType type, u64 *timestamp);

/**
 * @brief Sets the time for the specified clock.
 * @param[in] type Clock to use.
 * @param[in] timestamp POSIX UTC timestamp.
 * @return Result code.
 */
Result timeSetCurrentTime(TimeType type, u64 timestamp);

Result timeGetDeviceLocationName(TimeLocationName *name);
Result timeSetDeviceLocationName(const TimeLocationName *name);
Result timeGetTotalLocationNameCount(u32 *total_location_name_count);
Result timeLoadLocationNameList(u32 index, TimeLocationName *location_name_array, size_t location_name_size, u32 *location_name_count);

Result timeLoadTimeZoneRule(const TimeLocationName *name, TimeZoneRule *rule);

Result timeToPosixTime(const TimeZoneRule *rule, const TimeCalendarTime *caltime, u64 *timestamp_list, size_t timestamp_list_size, u32 *timestamp_count);
Result timeToPosixTimeWithMyRule(const TimeCalendarTime *caltime, u64 *timestamp_list, size_t timestamp_list_size, u32 *timestamp_count);
Result timeToCalendarTime(const TimeZoneRule *rule, u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info);
Result timeToCalendarTimeWithMyRule(u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info);

