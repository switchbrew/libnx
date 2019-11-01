#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/time.h"
#include "runtime/hosversion.h"

__attribute__((weak)) TimeServiceType __nx_time_service_type = TimeServiceType_User;

static Service g_timeSrv;
static Service g_timeUserSystemClock;
static Service g_timeNetworkSystemClock;
static Service g_timeTimeZoneService;
static Service g_timeLocalSystemClock;

static Result _timeCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(time);

Result _timeInitialize(void) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    switch (__nx_time_service_type) {
        case TimeServiceType_User:
            rc = smGetService(&g_timeSrv, "time:u");
            break;
        case TimeServiceType_Menu:
            rc = smGetService(&g_timeSrv, "time:a");
            break;
        case TimeServiceType_System:
            rc = smGetService(&g_timeSrv, "time:s");
            break;
        case TimeServiceType_Repair:
            if (hosversionBefore(9,0,0))
                return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
            else
                rc = smGetService(&g_timeSrv, "time:r");
            break;
        case TimeServiceType_SystemUser:
            if (hosversionBefore(9,0,0))
                return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
            else
                rc = smGetService(&g_timeSrv, "time:su");
            break;
    }

    if (R_FAILED(rc))
        return rc;

    rc = _timeCmdGetSession(&g_timeSrv, &g_timeUserSystemClock, 0);

    if (R_SUCCEEDED(rc))
        rc = _timeCmdGetSession(&g_timeSrv, &g_timeNetworkSystemClock, 1);

    if (R_SUCCEEDED(rc))
        rc = _timeCmdGetSession(&g_timeSrv, &g_timeTimeZoneService, 3);

    if (R_SUCCEEDED(rc))
        rc = _timeCmdGetSession(&g_timeSrv, &g_timeLocalSystemClock, 4);

    return rc;
}

void _timeCleanup(void) {
    serviceClose(&g_timeLocalSystemClock);
    serviceClose(&g_timeTimeZoneService);
    serviceClose(&g_timeNetworkSystemClock);
    serviceClose(&g_timeUserSystemClock);
    serviceClose(&g_timeSrv);
}

Service* timeGetServiceSession(void) {
    return &g_timeSrv;
}

Service* timeGetServiceSession_SystemClock(TimeType type) {
    if (type==TimeType_UserSystemClock) {
        return &g_timeUserSystemClock;
    }
    else if (type==TimeType_NetworkSystemClock) {
        return &g_timeNetworkSystemClock;
    }
    else if (type==TimeType_LocalSystemClock) {
        return &g_timeLocalSystemClock;
    }
    else {
        return NULL;
    }
}

Service* timeGetServiceSession_TimeZoneService(void) {
    return &g_timeTimeZoneService;
}

static Result _timeCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _timeCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _timeCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _appletCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

Result timeGetCurrentTime(TimeType type, u64 *timestamp) {
    Service *srv = timeGetServiceSession_SystemClock(type);

    if (srv==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _timeCmdNoInOutU64(srv, timestamp, 0);
}

Result timeSetCurrentTime(TimeType type, u64 timestamp) {
    Service *srv = timeGetServiceSession_SystemClock(type);

    if (srv==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _timeCmdInU64NoOut(srv, timestamp, 1);
}

Result timeGetDeviceLocationName(TimeLocationName *name) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchOut(&g_timeTimeZoneService, 0, *name);
}

Result timeSetDeviceLocationName(const TimeLocationName *name) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchIn(&g_timeTimeZoneService, 1, *name);
}

Result timeGetTotalLocationNameCount(s32 *total_location_name_count) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutU32(&g_timeTimeZoneService, (u32*)total_location_name_count, 2);
}

Result timeLoadLocationNameList(s32 index, TimeLocationName *location_name_array, s32 location_name_max, s32 *location_name_count) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_timeTimeZoneService, 3, index, *location_name_count,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { location_name_array, sizeof(TimeLocationName)*location_name_max } },
    );
}

Result timeLoadTimeZoneRule(const TimeLocationName *name, TimeZoneRule *rule) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchIn(&g_timeTimeZoneService, 4, *name,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { rule, sizeof(TimeZoneRule) } },
    );
}

Result timeToCalendarTime(const TimeZoneRule *rule, u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        TimeCalendarTime caltime;
        TimeCalendarAdditionalInfo info;
    } out;

    Result rc = serviceDispatchInOut(&g_timeTimeZoneService, 100, timestamp, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { rule, sizeof(TimeZoneRule) } },
    );
    if (R_SUCCEEDED(rc) && caltime) *caltime = out.caltime;
    if (R_SUCCEEDED(rc) && info) *info = out.info;
    return rc;
}

Result timeToCalendarTimeWithMyRule(u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        TimeCalendarTime caltime;
        TimeCalendarAdditionalInfo info;
    } out;

    Result rc = serviceDispatchInOut(&g_timeTimeZoneService, 101, timestamp, out);
    if (R_SUCCEEDED(rc) && caltime) *caltime = out.caltime;
    if (R_SUCCEEDED(rc) && info) *info = out.info;
    return rc;
}

Result timeToPosixTime(const TimeZoneRule *rule, const TimeCalendarTime *caltime, u64 *timestamp_list, s32 timestamp_list_count, s32 *timestamp_count) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_timeTimeZoneService, 201, *caltime, *timestamp_count,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
        },
        .buffers = {
            { rule, sizeof(TimeZoneRule) },
            { timestamp_list, sizeof(u64)*timestamp_list_count },
        },
    );
}

Result timeToPosixTimeWithMyRule(const TimeCalendarTime *caltime, u64 *timestamp_list, s32 timestamp_list_count, s32 *timestamp_count) {
    if (!serviceIsActive(&g_timeTimeZoneService))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_timeTimeZoneService, 202, *caltime, *timestamp_count,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { timestamp_list, sizeof(u64)*timestamp_list_count } },
    );
}
