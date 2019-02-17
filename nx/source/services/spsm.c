#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/spsm.h"
#include "services/sm.h"

static Service g_spsmSrv;
static u64 g_refCnt;

Result spsmInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_spsmSrv))
        return 0;

    rc = smGetService(&g_spsmSrv, "spsm");

    if (R_FAILED(rc)) spsmExit();

    return rc;
}

void spsmExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_spsmSrv);
    }
}

Result spsmShutdown(bool reboot) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 reboot;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_spsmSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->reboot = reboot;

    Result rc = serviceIpcDispatch(&g_spsmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_spsmSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result spsmPutErrorState(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_spsmSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&g_spsmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_spsmSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
