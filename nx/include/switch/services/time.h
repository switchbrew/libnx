/**
 * @file time.h
 * @brief Time services IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../sf/service.h"

/// Values for __nx_time_service_type.
typedef enum {
    TimeServiceType_User           = 0, ///< Default. Initializes time:u.
    TimeServiceType_Menu           = 1, ///< Initializes time:a
    TimeServiceType_System         = 2, ///< Initializes time:s.
    TimeServiceType_Repair         = 3, ///< Initializes time:r. Only available with [9.0.0+].
    TimeServiceType_SystemUser     = 4, ///< Initializes time:su. Only available with [9.0.0+].
} TimeServiceType;

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
    u32 wday;              ///< 0-based day-of-week.
    u32 yday;              ///< 0-based day-of-year.
    char timezoneName[8];  ///< Timezone name string.
    u32 DST;               ///< 0 = no DST, 1 = DST.
    s32 offset;            ///< Seconds relative to UTC for this timezone.
} TimeCalendarAdditionalInfo;

typedef struct {
    u8 data[0x4000];
} TimeZoneRule;

typedef struct {
    char name[0x24];
} TimeLocationName;

typedef struct {
    s64 time_point;        ///< A point in time.
    Uuid source_id;        ///< An ID representing the clock source.
} TimeSteadyClockTimePoint;

/// Initialize time. Used automatically during app startup.
Result timeInitialize(void);

/// Exit time. Used automatically during app startup.
void timeExit(void);

/// Gets the Service object for the actual time service session.
Service* timeGetServiceSession(void);

/// Gets the Service object for ISystemClock with the specified \ref TimeType. This will return NULL when the type is invalid.
Service* timeGetServiceSession_SystemClock(TimeType type);

/// Gets the Service object for ITimeZoneService.
Service* timeGetServiceSession_TimeZoneService(void);

/**
 * @brief Gets the time for the specified clock.
 * @param[in] type Clock to use.
 * @param[out] timestamp POSIX UTC timestamp.
 * @return Result code.
 */
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
Result timeGetTotalLocationNameCount(s32 *total_location_name_count);
Result timeLoadLocationNameList(s32 index, TimeLocationName *location_name_array, s32 location_name_max, s32 *location_name_count);

Result timeLoadTimeZoneRule(const TimeLocationName *name, TimeZoneRule *rule);

Result timeToCalendarTime(const TimeZoneRule *rule, u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info);
Result timeToCalendarTimeWithMyRule(u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info);
Result timeToPosixTime(const TimeZoneRule *rule, const TimeCalendarTime *caltime, u64 *timestamp_list, s32 timestamp_list_count, s32 *timestamp_count);
Result timeToPosixTimeWithMyRule(const TimeCalendarTime *caltime, u64 *timestamp_list, s32 timestamp_list_count, s32 *timestamp_count);

