#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/pcv.h"
#include "runtime/hosversion.h"
#include "services/sm.h"
#include "services/clkrst.h"

static Service g_clkrstSrv;
static u64 g_refCnt;

Result clkrstInitialize(void) {
    if(hosversionBefore(8,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_clkrstSrv)) {
        return 0;
    }

    Result rc = smGetService(&g_clkrstSrv, "clkrst");

    if (R_FAILED(rc)) {
        clkrstExit();
    }

    return rc;
}


void clkrstExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_clkrstSrv);
    }
}

Result clkrstOpenSession(ClkrstSession* session_out, PcvModuleId module_id, u32 unk) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 module_id;
        u32 unk;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->module_id = module_id;
    raw->unk = unk;

    Result rc = serviceIpcDispatch(&g_clkrstSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            serviceCreate(&session_out->s, r.Handles[0]);
        }
    }

    return rc;
}


void clkrstCloseSession(ClkrstSession* session) {
    serviceClose(&session->s);
}

Result clkrstSetClockRate(ClkrstSession* session, u32 hz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 hz;
    } *raw;

    raw = serviceIpcPrepareHeader(&session->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;
    raw->hz = hz;

    Result rc = serviceIpcDispatch(&session->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&session->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result clkrstGetClockRate(ClkrstSession* session, u32 *out_hz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&session->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;

    Result rc = serviceIpcDispatch(&session->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 hz;
        } *resp;

        serviceIpcParse(&session->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_hz = resp->hz;
        }
    }

    return rc;
}
