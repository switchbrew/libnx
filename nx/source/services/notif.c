#include <string.h>
#include "service_guard.h"
#include "services/notif.h"

#include "runtime/hosversion.h"

static NotifServiceType g_notifServiceType = NotifServiceType_Application;

static Service g_notifSrv;

NX_GENERATE_SERVICE_GUARD_PARAMS(notif, (NotifServiceType service_type), (service_type));

Result _notifInitialize(NotifServiceType service_type) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    g_notifServiceType = service_type;
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    switch (g_notifServiceType) {
        case NotifServiceType_Application:
            rc = smGetService(&g_notifSrv, "notif:a");
            break;
        case NotifServiceType_System:
            rc = smGetService(&g_notifSrv, "notif:s");
            break;
    }

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_notifSrv);
    }

    if (R_SUCCEEDED(rc) && g_notifServiceType == NotifServiceType_Application) { // Initialize cmd
        u64 pid_placeholder = 0;
        serviceAssumeDomain(&g_notifSrv);
        rc = serviceDispatchIn(&g_notifSrv, 1000, pid_placeholder, .in_send_pid = true);
    }

    return rc;
}

void _notifCleanup(void) {
    serviceClose(&g_notifSrv);
}

Service* notifGetServiceSession(void) {
    return &g_notifSrv;
}

void notifAlarmSettingCreate(NotifAlarmSetting *alarm_setting) {
    memset(alarm_setting, 0, sizeof(*alarm_setting));
    memset(alarm_setting->schedule.settings, 0xFF, sizeof(alarm_setting->schedule.settings));
}

Result notifAlarmSettingIsEnabled(NotifAlarmSetting *alarm_setting, u32 day_of_week, bool *out) {
    Result rc=0;

    *out = false;

    if (day_of_week >= 7)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (R_SUCCEEDED(rc)) {
        s16 tmp = alarm_setting->schedule.settings[day_of_week];
        *out = ((u8)(tmp>>8)) < 24 && (((u8)tmp) < 60);//hour<24 && minute<60
    }

    return rc;
}

Result notifAlarmSettingGet(NotifAlarmSetting *alarm_setting, u32 day_of_week, NotifAlarmTime *out) {
    Result rc=0;

    memset(out, 0, sizeof(*out));

    if (day_of_week >= 7)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (R_SUCCEEDED(rc)) {
        s16 tmp = alarm_setting->schedule.settings[day_of_week];
        out->hour = (u8)(tmp>>8);
        out->minute = (u8)tmp;
    }

    return rc;
}

Result notifAlarmSettingEnable(NotifAlarmSetting *alarm_setting, u32 day_of_week, s32 hour, s32 minute) {
    Result rc=0;

    if (day_of_week >= 7)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (R_SUCCEEDED(rc))
        alarm_setting->schedule.settings[day_of_week] = (((u8)hour)<<8) | ((u8)minute);

    return rc;
}

Result notifAlarmSettingDisable(NotifAlarmSetting *alarm_setting, u32 day_of_week) {
    Result rc=0;

    if (day_of_week >= 7)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (R_SUCCEEDED(rc))
        alarm_setting->schedule.settings[day_of_week] = -1;

    return rc;
}

Result notifRegisterAlarmSetting(u16 *alarm_setting_id, const NotifAlarmSetting *alarm_setting, const void* buffer, size_t size) {
    serviceAssumeDomain(&g_notifSrv);
    return serviceDispatchOut(&g_notifSrv, 500, *alarm_setting_id,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { alarm_setting, sizeof(*alarm_setting) },
            { buffer, size },
        },
    );
}

Result notifUpdateAlarmSetting(const NotifAlarmSetting *alarm_setting, const void* buffer, size_t size) {
    serviceAssumeDomain(&g_notifSrv);
    return serviceDispatch(&g_notifSrv, 510,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { alarm_setting, sizeof(*alarm_setting) },
            { buffer, size },
        },
    );
}

Result notifListAlarmSettings(NotifAlarmSetting *alarm_settings, s32 count, s32 *total_out) {
    serviceAssumeDomain(&g_notifSrv);
    return serviceDispatchOut(&g_notifSrv, 520, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { alarm_settings, count*sizeof(NotifAlarmSetting) } },
    );
}

Result notifLoadApplicationParameter(u16 alarm_setting_id, void* buffer, size_t size, u32 *actual_size) {
    serviceAssumeDomain(&g_notifSrv);
    return serviceDispatchInOut(&g_notifSrv, 530, alarm_setting_id, *actual_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result notifDeleteAlarmSetting(u16 alarm_setting_id) {
    serviceAssumeDomain(&g_notifSrv);
    return serviceDispatchIn(&g_notifSrv, 540, alarm_setting_id);
}

Result notifTryPopNotifiedApplicationParameter(void* buffer, u64 size, u64 *out_size) {
    Result rc=0;
    AppletStorage storage;
    s64 storage_size=0;
    u64 data_size = size;

    rc = appletTryPopFromNotificationStorageChannel(&storage);
    if (R_SUCCEEDED(rc)) rc = appletStorageGetSize(&storage, &storage_size);
    if (R_SUCCEEDED(rc)) {
        if (data_size > storage_size) data_size = storage_size;
        if (data_size) rc = appletStorageRead(&storage, 0, buffer, data_size);
        if (R_SUCCEEDED(rc) && out_size) *out_size = data_size;
    }

    appletStorageClose(&storage);
    return rc;
}

