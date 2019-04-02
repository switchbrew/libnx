#include "services/pctl.h"
#include "arm/atomics.h"
#include "runtime/hosversion.h"

static Service g_pctlSrv;
static Service g_pctlSession;
static u64 g_refCnt;

static Result _pctlCreateService(Service* out, u64 cmd_id);
static Result _pctlNoIO(u64 cmd_id);

Result pctlInitialize(void) {
    Result rc=0;
    bool sysverflag = hosversionBefore(4,0,0);

    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_pctlSrv))
        return 0;

    rc = smGetService(&g_pctlSrv, "pctl:a");
    if (R_FAILED(rc)) rc = smGetService(&g_pctlSrv, "pctl:s");
    if (R_FAILED(rc)) rc = smGetService(&g_pctlSrv, "pctl:r");
    if (R_FAILED(rc)) rc = smGetService(&g_pctlSrv, "pctl");

    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_pctlSrv);

    if (R_SUCCEEDED(rc)) rc = _pctlCreateService(&g_pctlSession, sysverflag ? 0 : 1);

    if (R_SUCCEEDED(rc) && !sysverflag) rc = _pctlNoIO(1);

    if (R_FAILED(rc))
        pctlExit();

    return rc;
}

void pctlExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_pctlSession);
        serviceClose(&g_pctlSrv);
    }
}

static Result _pctlCreateService(Service* out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid_reserved;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pctlSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->pid_reserved = 0;

    Result rc = serviceIpcDispatch(&g_pctlSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pctlSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreateSubservice(out, &g_pctlSrv, &r, 0);
    }

    return rc;
}

static Result _pctlNoIO(u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pctlSession, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_pctlSession);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pctlSession, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _pctlNoInputOutBool(u64 cmd_id, bool *flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pctlSession, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_pctlSession);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } PACKED *resp;

        serviceIpcParse(&g_pctlSession, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && flag) *flag = resp->out!=0;
    }

    return rc;
}

Result pctlIsRestrictionEnabled(bool *flag) {
    return _pctlNoInputOutBool(1031, flag);
}

