#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/time.h"
#include "services/sm.h"

static Service g_timeSrv;
static Service g_timeUserSystemClock;
static Service g_timeNetworkSystemClock;
static Service g_timeTimeZoneService;
static Service g_timeLocalSystemClock;
static u64 g_refCnt;

static Result _timeGetSession(Service* srv_out, u64 cmd_id);

Result timeInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_timeSrv))
        return 0;

    Result rc;

    rc = smGetService(&g_timeSrv, "time:s");
    if (R_FAILED(rc))
        rc = smGetService(&g_timeSrv, "time:u");

    if (R_FAILED(rc))
        return rc;

    rc = _timeGetSession(&g_timeUserSystemClock, 0);

    if (R_SUCCEEDED(rc))
        rc = _timeGetSession(&g_timeNetworkSystemClock, 1);

    if (R_SUCCEEDED(rc))
        rc = _timeGetSession(&g_timeTimeZoneService, 3);

    if (R_SUCCEEDED(rc))
        rc = _timeGetSession(&g_timeLocalSystemClock, 4);

    if (R_FAILED(rc))
        timeExit();

    return rc;
}

void timeExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        serviceClose(&g_timeLocalSystemClock);
        serviceClose(&g_timeTimeZoneService);
        serviceClose(&g_timeNetworkSystemClock);
        serviceClose(&g_timeUserSystemClock);
        serviceClose(&g_timeSrv);
    }
}

Service* timeGetServiceSession(void) {
    return &g_timeSrv;
}

static Result _timeGetSession(Service* srv_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_timeSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(srv_out, r.Handles[0]);
        }
    }

    return rc;
}

static Service* _timeGetClockSession(TimeType type) {
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

Result timeGetCurrentTime(TimeType type, u64 *timestamp) {
    Service *srv = _timeGetClockSession(type);

    if (srv==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 timestamp;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && timestamp) *timestamp = resp->timestamp;
    }

    return rc;
}

Result timeSetCurrentTime(TimeType type, u64 timestamp) {
    Service *srv = _timeGetClockSession(type);

    if (srv==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 timestamp;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->timestamp = timestamp;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result timeGetDeviceLocationName(TimeLocationName *name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            TimeLocationName name;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && name) memcpy(name, &resp->name, sizeof(TimeLocationName));
    }

    return rc;
}

Result timeSetDeviceLocationName(const TimeLocationName *name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        TimeLocationName name;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    memcpy(&raw->name, name, sizeof(TimeLocationName));

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result timeGetTotalLocationNameCount(u32 *total_location_name_count) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 total_location_name_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_location_name_count) *total_location_name_count = resp->total_location_name_count;
    }

    return rc;
}

Result timeLoadLocationNameList(u32 index, TimeLocationName *location_name_array, size_t location_name_size, u32 *location_name_count) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, location_name_array, location_name_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 index;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->index = index;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 location_name_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && location_name_count) *location_name_count = resp->location_name_count;
    }

    return rc;
}

Result timeLoadTimeZoneRule(const TimeLocationName *name, TimeZoneRule *rule) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, rule, sizeof(TimeZoneRule), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        TimeLocationName name;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    memcpy(&raw->name, name, sizeof(TimeLocationName));

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result timeToCalendarTime(const TimeZoneRule *rule, u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, rule, sizeof(TimeZoneRule), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 timestamp;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 100;
    raw->timestamp = timestamp;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            TimeCalendarTime caltime;
            TimeCalendarAdditionalInfo info;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && caltime) memcpy(caltime, &resp->caltime, sizeof(TimeCalendarTime));
        if (R_SUCCEEDED(rc) && info) memcpy(info, &resp->info, sizeof(TimeCalendarAdditionalInfo));
    }

    return rc;
}

Result timeToCalendarTimeWithMyRule(u64 timestamp, TimeCalendarTime *caltime, TimeCalendarAdditionalInfo *info) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 timestamp;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 101;
    raw->timestamp = timestamp;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            TimeCalendarTime caltime;
            TimeCalendarAdditionalInfo info;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && caltime) memcpy(caltime, &resp->caltime, sizeof(TimeCalendarTime));
        if (R_SUCCEEDED(rc) && info) memcpy(info, &resp->info, sizeof(TimeCalendarAdditionalInfo));
    }

    return rc;
}

Result timeToPosixTime(const TimeZoneRule *rule, const TimeCalendarTime *caltime, u64 *timestamp_list, size_t timestamp_list_size, u32 *timestamp_count) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, rule, sizeof(TimeZoneRule), BufferType_Normal);
    ipcAddRecvStatic(&c, timestamp_list, timestamp_list_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        TimeCalendarTime caltime;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 201;
    raw->caltime = *caltime;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 timestamp_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && timestamp_count) *timestamp_count = resp->timestamp_count;
    }

    return rc;
}

Result timeToPosixTimeWithMyRule(const TimeCalendarTime *caltime, u64 *timestamp_list, size_t timestamp_list_size, u32 *timestamp_count) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, timestamp_list, timestamp_list_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        TimeCalendarTime caltime;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;
    raw->caltime = *caltime;

    Result rc = serviceIpcDispatch(&g_timeTimeZoneService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 timestamp_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && timestamp_count) *timestamp_count = resp->timestamp_count;
    }

    return rc;
}
