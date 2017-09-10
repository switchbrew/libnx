// Copyright 2017 plutoo
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
            u64 zero;
            u64 reserved[2];
        } *raw;

        raw = ipcPrepareHeader(&c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;
        raw->zero = 0;

        rc = ipcDispatch(g_smHandle);

        if (R_SUCCEEDED(rc)) {
            IpcCommandResponse r;
            ipcParseResponse(&r);

            struct {
                u64 magic;
                u64 result;
            } *resp = r.Raw;

            rc = resp->result;
        }
    }

    return rc;
}

Result smGetService(Handle* handle_out, const char* name) {
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
        u64 reserved[2];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->service_name = name_encoded;

    Result rc = ipcDispatch(g_smHandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}
