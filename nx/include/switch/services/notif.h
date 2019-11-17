/**
 * @file notif.h
 * @brief Alarm notification (notif:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/applet.h"
#include "../services/acc.h"
#include "../sf/service.h"

/// ServiceType for \ref notifInitialize.
typedef enum {
    NotifServiceType_Application    = 0, ///< Initializes notif:a, for Application.
    NotifServiceType_System         = 1, ///< Initializes notif:s, for System.
} NotifServiceType;

/// Data extracted from NotifWeeklyScheduleAlarmSetting::settings. This uses local-time.
typedef struct {
    s32 hour;                                  ///< Hour.
    s32 minute;                                ///< Minute.
} NotifAlarmTime;

/// WeeklyScheduleAlarmSetting
typedef struct {
    u8 unk_x0[0xa];                            ///< Unknown.
    s16 settings[7];                           ///< Schedule settings for each day of the week, Sun-Sat. High byte is the hour, low byte is the minute. This uses local-time.
} NotifWeeklyScheduleAlarmSetting;

/// AlarmSetting
typedef struct {
    u16 alarm_setting_id;                      ///< AlarmSettingId
    u8 kind;                                   ///< Kind: 0 = WeeklySchedule.
    u8 muted;                                  ///< u8 bool flag for whether this AlarmSetting is muted (non-zero = AlarmSetting turned off, zero = on).
    u8 pad[4];                                 ///< Padding.
    AccountUid uid;                            ///< \ref AccountUid. User account associated with this AlarmSetting. Used for the preselected_user (\ref accountGetPreselectedUser) when launching the Application when the system was previously in sleep-mode, instead of launching the applet for selecting the user.
    u64 application_id;                        ///< ApplicationId
    u64 unk_x20;                               ///< Unknown.
    NotifWeeklyScheduleAlarmSetting schedule;  ///< \ref NotifWeeklyScheduleAlarmSetting
} NotifAlarmSetting;

/// Maximum alarms that can be registered at the same time by the host Application.
#define NOTIF_MAX_ALARMS 8

/// Initialize notif. Only available on [9.0.0+]. Note that using Alarms also requires the [9.0.0+] firmware update for controllers to be installed.
Result notifInitialize(NotifServiceType service_type);

/// Exit notif.
void notifExit(void);

/// Gets the Service object for the actual notif:* service session.
Service* notifGetServiceSession(void);

/**
 * @brief Creates a \ref NotifAlarmSetting.
 * @note This clears the struct, with all schedule settings set the same as \ref notifAlarmSettingDisable.
 * @param[out] alarm_setting \ref NotifAlarmSetting
 */
void notifAlarmSettingCreate(NotifAlarmSetting *alarm_setting);

/**
 * @brief Sets whether the \ref NotifAlarmSetting is muted.
 * @note By default (\ref notifAlarmSettingCreate) this is false.
 * @param alarm_setting \ref NotifAlarmSetting
 * @param[in] flag Whether the alarm is muted (true = Alarm turned off, false = on).
 */
NX_INLINE void notifAlarmSettingSetIsMuted(NotifAlarmSetting *alarm_setting, bool flag) {
    alarm_setting->muted = flag!=0;
}

/**
 * @brief Sets the \ref AccountUid for the \ref NotifAlarmSetting, see NotifAlarmSetting::uid.
 * @param alarm_setting \ref NotifAlarmSetting
 * @param[in] uid \ref AccountUid. If want to clear the uid after it was previously set, you can use an all-zero uid to reset to the default (\ref notifAlarmSettingCreate).
 */
NX_INLINE void notifAlarmSettingSetUid(NotifAlarmSetting *alarm_setting, AccountUid uid) {
    alarm_setting->uid = uid;
}

/**
 * @brief Gets whether the schedule setting for the specified day_of_week is enabled, for the \ref NotifAlarmSetting.
 * @param alarm_setting \ref NotifAlarmSetting
 * @param[in] day_of_week Day-of-week, must be 0-6 (Sun-Sat).
 * @param[out] out Whether the setting is enabled.
 */
Result notifAlarmSettingIsEnabled(NotifAlarmSetting *alarm_setting, u32 day_of_week, bool *out);

/**
 * @brief Gets the schedule setting for the specified day_of_week, for the \ref NotifAlarmSetting.
 * @note Should not be used if the output from \ref notifAlarmSettingIsEnabled is false.
 * @param alarm_setting \ref NotifAlarmSetting
 * @param[in] day_of_week Day-of-week, must be 0-6 (Sun-Sat).
 * @param[out] out \ref NotifAlarmTime
 */
Result notifAlarmSettingGet(NotifAlarmSetting *alarm_setting, u32 day_of_week, NotifAlarmTime *out);

/**
 * @brief Enables the schedule setting for the specified day_of_week, for the \ref NotifAlarmSetting. This uses local-time.
 * @param alarm_setting \ref NotifAlarmSetting
 * @param[in] day_of_week Day-of-week, must be 0-6 (Sun-Sat).
 * @param[in] hour Hour.
 * @param[in] minute Minute.
 */
Result notifAlarmSettingEnable(NotifAlarmSetting *alarm_setting, u32 day_of_week, s32 hour, s32 minute);

/**
 * @brief Disables the schedule setting for the specified day_of_week, for the \ref NotifAlarmSetting.
 * @note Schedule settings are disabled by default (\ref notifAlarmSettingCreate).
 * @param alarm_setting \ref NotifAlarmSetting
 * @param[in] day_of_week Day-of-week, must be 0-6 (Sun-Sat).
 */
Result notifAlarmSettingDisable(NotifAlarmSetting *alarm_setting, u32 day_of_week);

/**
 * @brief Registers the specified AlarmSetting.
 * @note See \ref NOTIF_MAX_ALARMS for the maximum alarms.
 * @param[out] alarm_setting_id AlarmSettingId
 * @param[in] alarm_setting \ref NotifAlarmSetting
 * @param[in] buffer Input buffer containing the ApplicationParameter. Optional, can be NULL.
 * @param[in] size Input buffer size, must be <=0x400. Optional, can be 0.
 */
Result notifRegisterAlarmSetting(u16 *alarm_setting_id, const NotifAlarmSetting *alarm_setting, const void* buffer, size_t size);

/**
 * @brief Updates the specified AlarmSetting.
 * @param[in] alarm_setting \ref NotifAlarmSetting
 * @param[in] buffer Input buffer containing the ApplicationParameter. Optional, can be NULL.
 * @param[in] size Input buffer size, must be <=0x400. Optional, can be 0.
 */
Result notifUpdateAlarmSetting(const NotifAlarmSetting *alarm_setting, const void* buffer, size_t size);

/**
 * @brief Gets a listing of AlarmSettings.
 * @param[out] alarm_settings Output \ref NotifAlarmSetting array.
 * @param[in] count Total entries in the alarm_settings array.
 * @param[out] total_out Total output entries.
 */
Result notifListAlarmSettings(NotifAlarmSetting *alarm_settings, s32 count, s32 *total_out);

/**
 * @brief Loads the ApplicationParameter for the specified AlarmSetting.
 * @param[in] alarm_setting_id AlarmSettingId
 * @param[out] buffer Output buffer containing the ApplicationParameter.
 * @param[in] size Output buffer size.
 * @param[out] actual_size Actual output size.
 */
Result notifLoadApplicationParameter(u16 alarm_setting_id, void* buffer, size_t size, u32 *actual_size);

/**
 * @brief Deletes the specified AlarmSetting.
 * @param[in] alarm_setting_id AlarmSettingId
 */
Result notifDeleteAlarmSetting(u16 alarm_setting_id);

/**
 * @brief Gets an Event which is signaled when data is available with \ref notifTryPopNotifiedApplicationParameter.
 * @note This is a wrapper for \ref appletGetNotificationStorageChannelEvent, see that for the usage requirements.
 * @note Some official apps don't use this.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
NX_INLINE Result notifGetNotificationSystemEvent(Event *out_event) {
    return appletGetNotificationStorageChannelEvent(out_event);
}

/**
 * @brief Uses \ref appletTryPopFromNotificationStorageChannel then reads the data from there into the output params.
 * @note This is a wrapper for \ref appletTryPopFromNotificationStorageChannel, see that for the usage requirements.
 * @note The system will only push data for this when launching the Application when the Alarm was triggered, where the system was previously in sleep-mode.
 * @note Some official apps don't use this.
 * @param[out] buffer Output buffer.
 * @param[out] size Output buffer size.
 * @param[out] out_size Size of the data which was written into the output buffer. Optional, can be NULL.
 */
Result notifTryPopNotifiedApplicationParameter(void* buffer, u64 size, u64 *out_size);

