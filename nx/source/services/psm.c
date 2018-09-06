#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "services/psm.h"
#include "services/sm.h"

static Service g_psmSrv;
static u64 g_refCnt;

Result psmInitialize(void)
{
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_psmSrv))
        return 0;

    rc = smGetService(&g_psmSrv, "psm");

    return rc;
}

void psmExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
        serviceClose(&g_psmSrv);
}

Result psmGetBatteryChargePercentage(u32 *out)
{
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    
    Result rc = serviceIpcDispatch(&g_psmSrv);
    
    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);
        
        struct {
            u64 magic;
            u64 result;
            u32 percentage;
        } *resp = r.Raw;
        
        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out = resp->percentage;
        }
    }
    
    return rc;
}
