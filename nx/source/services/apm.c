#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/apm.h"
#include "services/sm.h"

static Service g_apmSrv;
static Service g_apmISession;
static u64 g_refCnt;

static Result _apmGetSession(Service* srv, Service* srv_out, u64 cmd_id);

Result apmInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_apmSrv))
        return 0;

    Result rc = 0;

    rc = smGetService(&g_apmSrv, "apm:p");

    if (R_FAILED(rc))
        rc = smGetService(&g_apmSrv, "apm");

    // OpenSession.
    // Official sw doesn't open this until using commands which need it, when it wasn't already opened.
    if (R_SUCCEEDED(rc))
        rc = _apmGetSession(&g_apmSrv, &g_apmISession, 0);

    if (R_FAILED(rc))
        apmExit();

    return rc;
}

void apmExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_apmISession);
        serviceClose(&g_apmSrv);
    }
}

static Result _apmGetSession(Service* srv, Service* srv_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

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

Result apmSetPerformanceConfiguration(u32 PerformanceMode, u32 PerformanceConfiguration) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PerformanceMode;
        u32 PerformanceConfiguration;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->PerformanceMode = PerformanceMode;
    raw->PerformanceConfiguration = PerformanceConfiguration;

    Result rc = serviceIpcDispatch(&g_apmISession);

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

Result apmGetPerformanceConfiguration(u32 PerformanceMode, u32 *PerformanceConfiguration) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PerformanceMode;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->PerformanceMode = PerformanceMode;

    Result rc = serviceIpcDispatch(&g_apmISession);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 PerformanceConfiguration;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && PerformanceConfiguration) *PerformanceConfiguration = resp->PerformanceConfiguration;
    }

    return rc;
}

