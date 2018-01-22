#include "types.h"
#include "services/acc.h"
#include "services/sm.h"

static Service g_accSrv;

Result accountInitialize(void)
{
    if (serviceIsActive(&g_accSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    return smGetService(&g_accSrv, "acc:u1");
}

void accountExit(void) {
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

