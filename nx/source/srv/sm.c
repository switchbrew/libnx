#include <switch.h>

static Handle g_smHandle;

Result smInitialize() {
    Result rc = svcConnectToNamedPort(&g_smHandle, "sm:");

    if (R_SUCCEEDED(rc)) {
        IpcCommand c;
        ipcInitialize(&c);
        ipcSendPid(&c);

        struct {
            u64 magic;
            u64 cmd_id;
        } *raw;

        raw = ipcPrepareHeader(&c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;

        rc = ipcDispatch(g_smHandle);
    }

    return rc;
}
