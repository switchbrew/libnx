#include <switch.h>

static Handle g_smHandle = -1;

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

Result smGetService(const char* name) {
    u64 name_encoded = 0;

    size_t i;
    for (i=0; i<8; i++) {
        name_encoded = (name_encoded << 8) | name[i];

        if (name[i] == '\0')
            break;
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->service_name = name_encoded;

    return ipcDispatch(g_smHandle);
}
