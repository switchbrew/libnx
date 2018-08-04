#include "types.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/sm.h"
#include "services/hid_dbg.h"

static Service g_hidDbgSrv;
static u64 g_refCnt;

Result hidDbgInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_hidDbgSrv))
        return 0;

    return smGetService(&g_hidDbgSrv, "hid:dbg");
}

void hidDbgExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        serviceClose(&g_hidDbgSrv);
    }
}

Result _hidDbgDeactivateHomeButton() {
    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 110;

    rc = serviceIpcDispatch(&g_hidDbgSrv);

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

Result hidDbgDeactivateHomeButton() {
    int i = HID_DBG_DHB_MAX_TRIES;
    Result rc;
    
    do {
        i--;
        rc = _hidDbgDeactivateHomeButton();
    } while(rc == 0 && i > 0);
    
    if(rc == 0) {
        return HID_DBG_DHB_RC;
    } else if(rc == HID_DBG_DHB_RC) {
        return 0;
    }
    
    return rc;
}