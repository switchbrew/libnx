#include <string.h>
#include <switch.h>

static Handle g_hidServiceSession = INVALID_HANDLE;
static Handle g_hidIAppletResource = INVALID_HANDLE;
static SharedMemory g_hidSharedmem;

static Result _hidCreateAppletResource(Handle sessionhandle, Handle* handle_out, u64 AppletResourceUserId);
static Result _hidGetSharedMemoryHandle(Handle sessionhandle, Handle* handle_out);

Result hidInitialize(void) {
    if (g_hidServiceSession != INVALID_HANDLE) return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;
    u64 AppletResourceUserId = 0;
    Handle sharedmem_handle=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc)) return rc;

    rc = smGetService(&g_hidServiceSession, "hid");
    if (R_FAILED(rc)) return rc;

    rc = _hidCreateAppletResource(g_hidServiceSession, &g_hidIAppletResource, AppletResourceUserId);

    if (R_SUCCEEDED(rc)) rc = _hidGetSharedMemoryHandle(g_hidIAppletResource, &sharedmem_handle);

    if (R_SUCCEEDED(rc)) {
        shmemLoadRemote(&g_hidSharedmem, sharedmem_handle, 0x40000, PERM_R);

        rc = shmemMap(&g_hidSharedmem);

        if (R_FAILED(rc)) svcCloseHandle(sharedmem_handle);
    }

    if (R_FAILED(rc)) {
        if (g_hidServiceSession != INVALID_HANDLE) svcCloseHandle(g_hidServiceSession);
        if (g_hidIAppletResource != INVALID_HANDLE) svcCloseHandle(g_hidIAppletResource);

        g_hidServiceSession = INVALID_HANDLE;
        g_hidIAppletResource = INVALID_HANDLE;
    }

    return rc;
}

void hidExit(void)
{
    if (g_hidServiceSession == INVALID_HANDLE) return;

    if (g_hidServiceSession != INVALID_HANDLE) {
        svcCloseHandle(g_hidServiceSession);
        g_hidServiceSession = INVALID_HANDLE;
    }

    if (g_hidIAppletResource != INVALID_HANDLE) {
        svcCloseHandle(g_hidIAppletResource);
        g_hidIAppletResource = INVALID_HANDLE;
    }

    shmemClose(&g_hidSharedmem);
}

Handle hidGetSessionService(void) {
    return g_hidServiceSession;
}

void* hidGetSharedmemAddr(void) {
    return shmemGetAddr(&g_hidSharedmem);
}

static Result _hidCreateAppletResource(Handle sessionhandle, Handle* handle_out, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->AppletResourceUserId = AppletResourceUserId;

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

static Result _hidGetSharedMemoryHandle(Handle sessionhandle, Handle* handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

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

