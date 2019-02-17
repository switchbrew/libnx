#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/bpc.h"
#include "services/sm.h"

static Service g_bpcSrv;
static u64 g_refCnt;

Result bpcInitialize(void)
{
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_bpcSrv))
        return 0;

    rc = smGetService(&g_bpcSrv, hosversionAtLeast(2,0,0) ? "bpc" : "bpc:c");

    return rc;
}

void bpcExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
        serviceClose(&g_bpcSrv);
}

Result bpcShutdownSystem(void)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_bpcSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_bpcSrv);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_bpcSrv, &r, sizeof(*resp));
        resp = r.Raw;
        
        rc = resp->result;
    }

    return rc;
}

Result bpcRebootSystem(void)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_bpcSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&g_bpcSrv);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_bpcSrv, &r, sizeof(*resp));
        resp = r.Raw;
        
        rc = resp->result;
    }

    return rc;
}

Result bpcGetSleepButtonState(BpcSleepButtonState *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_bpcSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&g_bpcSrv);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 state;
        } *resp;

        serviceIpcParse(&g_bpcSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = (BpcSleepButtonState)resp->state;
        }
    }

    return rc;
}
