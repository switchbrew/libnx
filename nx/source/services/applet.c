#include <string.h>
#include <switch.h>

__attribute__((weak)) u32 __nx_applet_type = APPLET_TYPE_Default;
__attribute__((weak)) bool __nx_applet_auto_notifyrunning = true;

static Handle g_appletServiceSession = INVALID_HANDLE;
static Handle g_appletProxySession = INVALID_HANDLE;

static Handle g_appletIFunctions = INVALID_HANDLE;//From Get*Functions, for ILibraryAppletProxy this is GetLibraryAppletSelfAccessor

static Handle g_appletILibraryAppletCreator = INVALID_HANDLE;
static Handle g_appletICommonStateGetter = INVALID_HANDLE;
static Handle g_appletISelfController = INVALID_HANDLE;
static Handle g_appletIWindowController = INVALID_HANDLE;
static Handle g_appletIAudioController = INVALID_HANDLE;
static Handle g_appletIDisplayController = INVALID_HANDLE;
static Handle g_appletIDebugFunctions = INVALID_HANDLE;

static u64 g_appletResourceUserId = 0;

void appletExit(void);

static Result _appletGetSession(Handle sessionhandle, Handle* handle_out, u64 cmd_id);
static Result _appletGetSessionProxy(Handle sessionhandle, Handle* handle_out, u64 cmd_id, Handle prochandle);

static Result _appletGetAppletResourceUserId(u64 *out);
static Result _appletNotifyRunning(u8 *out);

Result appletInitialize(void) {
    if (g_appletServiceSession != INVALID_HANDLE) return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;
    Handle prochandle = CUR_PROCESS_HANDLE;

    if (__nx_applet_type==APPLET_TYPE_None) return 0;

    g_appletResourceUserId = 0;

    if (__nx_applet_type==APPLET_TYPE_Default) __nx_applet_type = APPLET_TYPE_Application;

    if (__nx_applet_type==APPLET_TYPE_Application) {
        rc = smGetService(&g_appletServiceSession, "appletOE");
    }

    if (__nx_applet_type!=APPLET_TYPE_Application) {
        rc = smGetService(&g_appletServiceSession, "appletAE");
    }

    if (R_SUCCEEDED(rc)) {
        do {
            switch(__nx_applet_type) {
                default:
                    rc = MAKERESULT(MODULE_LIBNX, LIBNX_NOTFOUND);
                break;

                case APPLET_TYPE_Application:
                    rc = _appletGetSessionProxy(g_appletServiceSession, &g_appletProxySession, 0, prochandle);
                break;

                case APPLET_TYPE_SystemApplet:
                    rc = _appletGetSessionProxy(g_appletServiceSession, &g_appletProxySession, 100, prochandle);
                break;

                case APPLET_TYPE_LibraryApplet:
                    rc = _appletGetSessionProxy(g_appletServiceSession, &g_appletProxySession, 201, prochandle);
                break;

                case APPLET_TYPE_OverlayApplet:
                    rc = _appletGetSessionProxy(g_appletServiceSession, &g_appletProxySession, 300, prochandle);
                break;

                case APPLET_TYPE_SystemApplication:
                    rc = _appletGetSessionProxy(g_appletServiceSession, &g_appletProxySession, 350, prochandle);
                break;
            }

            if ((rc & 0x3fffff) == 0x19280) svcSleepThread(10000000);
        } while((rc & 0x3fffff) == 0x19280);
    }

    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletIFunctions, 20);//Get*Functions, for ILibraryAppletProxy this is GetLibraryAppletSelfAccessor
    //TODO: Add non-application type-specific session init here.

    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletILibraryAppletCreator, 11);//GetLibraryAppletCreator
    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletICommonStateGetter, 0);//GetCommonStateGetter
    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletISelfController, 1);//GetSelfController
    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletIWindowController, 2);//GetWindowController (get GetAppletResourceUserId from this)
    if (R_SUCCEEDED(rc)) rc = _appletGetAppletResourceUserId(&g_appletResourceUserId);
    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletIAudioController, 3);//GetAudioController
    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletIDisplayController, 4);//GetDisplayController
    if (R_SUCCEEDED(rc)) rc = _appletGetSession(g_appletProxySession, &g_appletIDebugFunctions, 1000);//GetDebugFunctions

    if (R_SUCCEEDED(rc) && __nx_applet_auto_notifyrunning && __nx_applet_type==APPLET_TYPE_Application) rc = _appletNotifyRunning(NULL);

    if (R_FAILED(rc)) appletExit();

    return rc;
}

void appletExit(void)
{
    if (g_appletServiceSession == INVALID_HANDLE) return;

    if (g_appletServiceSession != INVALID_HANDLE) {
        svcCloseHandle(g_appletServiceSession);
        g_appletServiceSession = INVALID_HANDLE;
    }

    if (g_appletProxySession != INVALID_HANDLE) {
        svcCloseHandle(g_appletProxySession);
        g_appletProxySession = INVALID_HANDLE;
    }

    if (g_appletIFunctions != INVALID_HANDLE) {
        svcCloseHandle(g_appletIFunctions);
        g_appletIFunctions = INVALID_HANDLE;
    }

    if (g_appletILibraryAppletCreator != INVALID_HANDLE) {
        svcCloseHandle(g_appletILibraryAppletCreator);
        g_appletILibraryAppletCreator = INVALID_HANDLE;
    }

    if (g_appletICommonStateGetter != INVALID_HANDLE) {
        svcCloseHandle(g_appletICommonStateGetter);
        g_appletICommonStateGetter = INVALID_HANDLE;
    }

    if (g_appletISelfController != INVALID_HANDLE) {
        svcCloseHandle(g_appletISelfController);
        g_appletISelfController = INVALID_HANDLE;
    }

    if (g_appletIWindowController != INVALID_HANDLE) {
        svcCloseHandle(g_appletIWindowController);
        g_appletIWindowController = INVALID_HANDLE;
    }

    if (g_appletIAudioController != INVALID_HANDLE) {
        svcCloseHandle(g_appletIAudioController);
        g_appletIAudioController = INVALID_HANDLE;
    }

    if (g_appletIDisplayController != INVALID_HANDLE) {
        svcCloseHandle(g_appletIDisplayController);
        g_appletIDisplayController = INVALID_HANDLE;
    }

    if (g_appletIDebugFunctions != INVALID_HANDLE) {
        svcCloseHandle(g_appletIDebugFunctions);
        g_appletIDebugFunctions = INVALID_HANDLE;
    }

    g_appletResourceUserId = 0;
}

static Result _appletGetSession(Handle sessionhandle, Handle* handle_out, u64 cmd_id) {
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

static Result _appletGetSessionProxy(Handle sessionhandle, Handle* handle_out, u64 cmd_id, Handle prochandle) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 reserved;
    } *raw;

    ipcSendPid(&c);
    ipcSendHandleCopy(&c, prochandle);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->reserved = 0;

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

static Result _appletGetAppletResourceUserId(u64 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = ipcDispatch(g_appletIWindowController);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}

Result appletGetAppletResourceUserId(u64 *out) {
    if (g_appletServiceSession == INVALID_HANDLE) return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    *out = g_appletResourceUserId;
    return 0;
}

static Result _appletNotifyRunning(u8 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 40;

    Result rc = ipcDispatch(g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) {
            *out = resp->out;
        }
    }

    return rc;
}

Result appletCreateManagedDisplayLayer(u64 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 40;

    Result rc = ipcDispatch(g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}

