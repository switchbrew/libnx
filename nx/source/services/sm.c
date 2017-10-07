// Copyright 2017 plutoo
#include <switch.h>

static Handle g_smHandle = -1;

bool smHasInitialized() {
    return g_smHandle != -1;
}

Result smInitialize() {
    if(smHasInitialized())return 0;

    Result rc = svcConnectToNamedPort(&g_smHandle, "sm:");
    Handle tmp;

    if (R_SUCCEEDED(rc) && smGetService(&tmp, "") == 0x415) {
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

void smExit(void) {
    if(smHasInitialized()) {
        svcCloseHandle(g_smHandle);
        g_smHandle = -1;
    }
}

static u64 _EncodeName(const char* name) {
    u64 name_encoded = 0;

    size_t i;
    for (i=0; i<8; i++) {
        if (name[i] == '\0')
            break;

        name_encoded |= ((u64) name[i]) << (8*i);
    }

    return name_encoded;
}

Result smGetService(Handle* handle_out, const char* name) {
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
    raw->service_name = _EncodeName(name);

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

Result smRegisterService(Handle* handle_out, const char* name, bool is_light, int max_sessions) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u32 is_light;
        u32 max_sessions;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->service_name = _EncodeName(name);
    raw->is_light = !!is_light;
    raw->max_sessions = max_sessions;

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

Result smUnregisterService(const char* name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u64 reserved;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->service_name = _EncodeName(name);

    Result rc = ipcDispatch(g_smHandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
