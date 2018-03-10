#include "types.h"
#include "arm/atomics.h"
#include "services/acc.h"
#include "services/sm.h"

static Service g_accSrv;
static u64 g_refCnt;

Result accountInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_accSrv))
        return 0;

    return smGetService(&g_accSrv, "acc:u1");
}

void accountExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
        serviceClose(&g_accSrv);
}

Service* accountGetService(void) {
    return &g_accSrv;
}

Result accountGetActiveUser(u128 *userID, bool *account_selected)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_accSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u128 userID;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && userID) {
            *userID = resp->userID;
            if (account_selected) {
                *account_selected = 0;
                if (*userID != 0) *account_selected = 1;
            }
        }
    }

    return rc;
}

