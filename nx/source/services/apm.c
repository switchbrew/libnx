#include <string.h>
#include <switch.h>

static Handle g_apmServiceSession = INVALID_HANDLE;
static Handle g_apmISession = INVALID_HANDLE;

static Result _apmGetSession(Handle sessionhandle, Handle* handle_out, u64 cmd_id);

Result apmInitialize(void) {
    if (g_apmServiceSession != INVALID_HANDLE) return 0;

    Result rc = 0;

    rc = smGetService(&g_apmServiceSession, "apm:p");
    if (R_FAILED(rc)) rc = smGetService(&g_apmServiceSession, "apm");

    if (R_SUCCEEDED(rc)) rc = _apmGetSession(g_apmServiceSession, &g_apmISession, 0);//OpenSession. Official sw doesn't open this until using commands which need it, when it wasn't already opened.

    if (R_FAILED(rc)) apmExit();

    return rc;
}

void apmExit(void)
{
    if (g_apmServiceSession == INVALID_HANDLE) return;

    if (g_apmServiceSession != INVALID_HANDLE) {
        svcCloseHandle(g_apmServiceSession);
        g_apmServiceSession = INVALID_HANDLE;
    }

    if (g_apmISession != INVALID_HANDLE) {
        svcCloseHandle(g_apmISession);
        g_apmISession = INVALID_HANDLE;
    }
}

static Result _apmGetSession(Handle sessionhandle, Handle* handle_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = ipcDispatch(sessionhandle);

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

Result apmSetPerformanceConfiguration(u32 PerformanceMode, u32 PerformanceConfiguration) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PerformanceMode;
        u32 PerformanceConfiguration;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->PerformanceMode = PerformanceMode;
    raw->PerformanceConfiguration = PerformanceConfiguration;

    Result rc = ipcDispatch(g_apmISession);

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

Result apmGetPerformanceConfiguration(u32 PerformanceMode, u32 *PerformanceConfiguration) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PerformanceMode;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->PerformanceMode = PerformanceMode;

    Result rc = ipcDispatch(g_apmISession);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 PerformanceConfiguration;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && PerformanceConfiguration) *PerformanceConfiguration = resp->PerformanceConfiguration;
    }

    return rc;
}

