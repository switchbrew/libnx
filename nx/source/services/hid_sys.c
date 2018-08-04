#include "types.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/sm.h"
#include "services/applet.h"
#include "services/hid_sys.h"

static Service g_hidSysSrv;
static u64 g_refCnt;

Result hidSysInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_hidSysSrv))
        return 0;

    return smGetService(&g_hidSysSrv, "hid:sys");
}

void hidSysExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        serviceClose(&g_hidSysSrv);
    }
}

Result hidSysActivateHomeButton() {
    Result rc;

    u64 AppletResourceUserId;
    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc)) {
        AppletResourceUserId = 0;
    }
        
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 111;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSysSrv);

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