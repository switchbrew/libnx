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

Service* timeGetSessionService(void) {
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

