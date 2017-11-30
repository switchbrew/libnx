#include <string.h>
#include <switch.h>

static Handle g_accountServiceSession = INVALID_HANDLE;

Result accountInitialize(void) {
    if (g_accountServiceSession != INVALID_HANDLE) return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;

    rc = smGetService(&g_accountServiceSession, "acc:u1");
    if (R_FAILED(rc)) return rc;

    return rc;
}

void accountExit(void)
{
    if (g_accountServiceSession == INVALID_HANDLE) return;

    if (g_accountServiceSession != INVALID_HANDLE) {
        svcCloseHandle(g_accountServiceSession);
        g_accountServiceSession = INVALID_HANDLE;
    }
}

Handle accountGetSessionService(void) {
    return g_accountServiceSession;
}

Result accountGetActiveUser(u128 *userID, bool *account_selected) {
    if (g_accountServiceSession == INVALID_HANDLE) return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = ipcDispatch(g_accountServiceSession);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

