#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/sm.h"
#include "services/ns.h"

//TODO: >=3.0.0 support.

static Service g_nsSrv;
static u64 g_nsRefCnt;

Result nsInitialize(void)
{
    atomicIncrement64(&g_nsRefCnt);

    if (serviceIsActive(&g_nsSrv))
        return 0;

    return smGetService(&g_nsSrv, "ns:am");//This will fail on >=3.0.0.
}

void nsExit(void)
{
    if (atomicDecrement64(&g_nsRefCnt) == 0) {
        serviceClose(&g_nsSrv);
    }
}

Result nsGetApplicationControlData(u8 flag, u64 titleID, nsApplicationControlData* buffer, size_t size, size_t* actual_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
        u64 titleID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 400;
    raw->flag = flag;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_nsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 actual_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && actual_size) *actual_size = resp->actual_size;
    }

    return rc;
}
