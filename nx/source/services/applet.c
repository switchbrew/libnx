#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "services/fatal.h"
#include "services/applet.h"
#include "services/apm.h"
#include "services/sm.h"

__attribute__((weak)) u32 __nx_applet_type = AppletType_Default;
__attribute__((weak)) bool __nx_applet_auto_notifyrunning = true;
__attribute__((weak)) u8 __nx_applet_AppletAttribute[0x80];
__attribute__((weak)) u32 __nx_applet_PerformanceConfiguration[2] = {/*0x92220008*//*0x20004*//*0x92220007*/0, 0};

static Service g_appletSrv;
static Service g_appletProxySession;
static u64 g_refCnt;

// From Get*Functions.
static Service g_appletIFunctions;

static Service g_appletILibraryAppletSelfAccessor;
static Service g_appletIProcessWindingController;

static Service g_appletILibraryAppletCreator;
static Service g_appletICommonStateGetter;
static Service g_appletISelfController;
static Service g_appletIWindowController;
static Service g_appletIAudioController;
static Service g_appletIDisplayController;
static Service g_appletIDebugFunctions;

static Handle g_appletMessageEventHandle = INVALID_HANDLE;

static u64 g_appletResourceUserId = 0;
static u8  g_appletOperationMode;
static u32 g_appletPerformanceMode;
static u8  g_appletFocusState;

static bool g_appletNotifiedRunning = 0;

static AppletHookCookie g_appletFirstHook;

static Result _appletGetHandle(Service* srv, Handle* handle_out, u64 cmd_id);
static Result _appletGetSession(Service* srv, Service* srv_out, u64 cmd_id);
static Result _appletGetSessionProxy(Service* srv_out, u64 cmd_id, Handle prochandle, u8 *AppletAttribute);

static Result _appletGetAppletResourceUserId(u64 *out);

static Result appletSetFocusHandlingMode(u32 mode);
static Result _appletGetCurrentFocusState(u8 *out);
static Result _appletSetFocusHandlingMode(u8 inval0, u8 inval1, u8 inval2);
static Result _appletSetOutOfFocusSuspendingEnabled(u8 inval);

static Result _appletReceiveMessage(u32 *out);
static Result _appletAcquireForegroundRights(void);

static Result _appletGetOperationMode(u8 *out);
static Result _appletGetPerformanceMode(u32 *out);

static Result _appletSetOperationModeChangedNotification(u8 flag);
static Result _appletSetPerformanceModeChangedNotification(u8 flag);

//static Result _appletSelfExit(void);
//static Result _appletLockExit(void);
//static Result _appletUnlockExit(void);

//static Result _appletExitProcessAndReturn(void);

Result appletInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_appletSrv))
        return 0;

    if (__nx_applet_type == AppletType_None)
        return 0;

    Result rc = 0;

    g_appletResourceUserId = 0;
    g_appletNotifiedRunning = 0;

    switch (__nx_applet_type) {
    case AppletType_Default:
        __nx_applet_type = AppletType_Application;
        // Fallthrough.
    case AppletType_Application:
        rc = smGetService(&g_appletSrv, "appletOE");
        break;
    default:
        rc = smGetService(&g_appletSrv, "appletAE");
        break;
    }

    if (R_SUCCEEDED(rc)) {
        #define AM_BUSY_ERROR 0x19280

        do {
            u32 cmd_id;

            switch(__nx_applet_type) {
            case AppletType_Application:       cmd_id = 0;   break;
            case AppletType_SystemApplet:      cmd_id = 100; break;
            case AppletType_LibraryApplet:     cmd_id = 200; break;
            case AppletType_OverlayApplet:     cmd_id = 300; break;
            case AppletType_SystemApplication: cmd_id = 350; break;
            // TODO: Replace error code
            default: fatalSimple(MAKERESULT(Module_Libnx, LibnxError_AppletCmdidNotFound));
            }

            rc = _appletGetSessionProxy(&g_appletProxySession, cmd_id, CUR_PROCESS_HANDLE, NULL);

            if (rc == AM_BUSY_ERROR) {
                svcSleepThread(10000000);
            }

        } while (rc == AM_BUSY_ERROR);
    }

    // Get*Functions
    if (R_SUCCEEDED(rc) && __nx_applet_type != AppletType_LibraryApplet)
        rc = _appletGetSession(&g_appletProxySession, &g_appletIFunctions, 20);

    // TODO: Add non-{Application/LibraryApplet} type-specific session init here.

    if (R_SUCCEEDED(rc) && __nx_applet_type == AppletType_LibraryApplet) {
        //GetLibraryAppletSelfAccessor
        rc = _appletGetSession(&g_appletProxySession, &g_appletILibraryAppletSelfAccessor, 20);

        //GetProcessWindingController
        if (R_SUCCEEDED(rc))
            rc = _appletGetSession(&g_appletProxySession, &g_appletIProcessWindingController, 10);
    }

    // GetLibraryAppletCreator
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletILibraryAppletCreator, 11);
    // GetCommonStateGetter
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletICommonStateGetter, 0);
    // GetSelfController
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletISelfController, 1);
    // GetWindowController
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletIWindowController, 2);
    // Get AppletResourceUserId.
    if (R_SUCCEEDED(rc))
        rc = _appletGetAppletResourceUserId(&g_appletResourceUserId);
    // GetAudioController
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletIAudioController, 3);
    // GetDisplayController
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletIDisplayController, 4);
    // GetDebugFunctions
    if (R_SUCCEEDED(rc))
        rc = _appletGetSession(&g_appletProxySession, &g_appletIDebugFunctions, 1000);

    // ICommonStateGetter::GetEventHandle
    if (R_SUCCEEDED(rc))
        rc = _appletGetHandle(&g_appletICommonStateGetter, &g_appletMessageEventHandle, 0);

    if (R_SUCCEEDED(rc) && (__nx_applet_type == AppletType_Application))
    {
        do {
            svcWaitSynchronizationSingle(g_appletMessageEventHandle, U64_MAX);

            u32 msg;
            rc = _appletReceiveMessage(&msg);

            if (R_FAILED(rc))
            {
                if ((rc & 0x3fffff) == 0x680)
                    continue;

                break;
            }

            if (msg != 0xF)
                continue;

            rc = _appletGetCurrentFocusState(&g_appletFocusState);

            if (R_FAILED(rc))
                break;

        } while(g_appletFocusState!=1);

        if (R_SUCCEEDED(rc))
            rc = _appletAcquireForegroundRights();

        if (R_SUCCEEDED(rc))
            rc = appletSetFocusHandlingMode(0);
    }

    if (R_SUCCEEDED(rc) && __nx_applet_auto_notifyrunning)
        appletNotifyRunning(NULL);

    if (R_SUCCEEDED(rc))
        rc = _appletGetOperationMode(&g_appletOperationMode);
    if (R_SUCCEEDED(rc))
        rc = _appletGetPerformanceMode(&g_appletPerformanceMode);
    if (R_SUCCEEDED(rc) && __nx_applet_type!=AppletType_Application)
        rc = _appletGetCurrentFocusState(&g_appletFocusState);

    if (R_SUCCEEDED(rc))
        rc = _appletSetOperationModeChangedNotification(1);
    if (R_SUCCEEDED(rc))
        rc = _appletSetPerformanceModeChangedNotification(1);

    if (R_SUCCEEDED(rc))
        rc = apmInitialize();

    // Official apps aren't known to use apmSetPerformanceConfiguration with mode=1.
    if (R_SUCCEEDED(rc)) {
        // This is broken with the regular "apm" service.
        u32 i;
        for (i=0; i<2; i++)
        {
            if (__nx_applet_PerformanceConfiguration[i])
                rc = apmSetPerformanceConfiguration(i, __nx_applet_PerformanceConfiguration[i]);

            if (R_FAILED(rc))
                break;
        }
    }

    if (R_FAILED(rc))
        appletExit();

    return rc;
}

void appletExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        apmExit();

        //TODO: Enable this somehow later with more condition(s)?
        /*if (__nx_applet_type == AppletType_LibraryApplet)
            _appletExitProcessAndReturn();*/

        if (g_appletMessageEventHandle != INVALID_HANDLE) {
            svcCloseHandle(g_appletMessageEventHandle);
            g_appletMessageEventHandle = INVALID_HANDLE;
        }

        serviceClose(&g_appletIDebugFunctions);
        serviceClose(&g_appletIDisplayController);
        serviceClose(&g_appletIAudioController);
        serviceClose(&g_appletIWindowController);
        serviceClose(&g_appletISelfController);
        serviceClose(&g_appletICommonStateGetter);
        serviceClose(&g_appletILibraryAppletCreator);

        if (__nx_applet_type != AppletType_LibraryApplet)
            serviceClose(&g_appletIFunctions);

        if (__nx_applet_type == AppletType_LibraryApplet) {
            serviceClose(&g_appletIProcessWindingController);
            serviceClose(&g_appletILibraryAppletSelfAccessor);
        }

        serviceClose(&g_appletProxySession);
        serviceClose(&g_appletSrv);
        g_appletResourceUserId = 0;
    }
}

AppletType appletGetAppletType(void) {
    return __nx_applet_type;
}

static void appletCallHook(AppletHookType hookType)
{
    AppletHookCookie* c;
    for (c = &g_appletFirstHook; c && c->callback; c = c->next)
        c->callback(hookType, c->param);
}

void appletHook(AppletHookCookie* cookie, AppletHookFn callback, void* param)
{
    if (callback == NULL)
        return;

    AppletHookCookie* hook = &g_appletFirstHook;
    *cookie = *hook; // Structure copy.
    hook->next = cookie;
    hook->callback = callback;
    hook->param = param;
}

void appletUnhook(AppletHookCookie* cookie)
{
    AppletHookCookie* hook;
    for (hook = &g_appletFirstHook; hook; hook = hook->next)
    {
        if (hook->next == cookie)
        {
            *hook = *cookie; // Structure copy.
            break;
        }
    }
}

static Result appletSetFocusHandlingMode(u32 mode) {
    Result rc;
    u8 invals[4];

    if (mode > 3)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(invals, 0, sizeof(invals));

    if ((mode == 0) || (mode == 3)) {
        invals[0] = 0;
        invals[1] = 0;
        invals[2] = 1;
    }

    if (mode != 3) {
        invals[3] = 0;

        if (mode == 1) {
            invals[0] = 1;
            invals[1] = 1;
            invals[2] = 0;
        }
        else if (mode == 2) {
            invals[0] = 1;
            invals[1] = 0;
            invals[2] = 1;
        }
    }
    else {
        invals[3] = 1;
    }

    rc = _appletSetFocusHandlingMode(invals[0], invals[1], invals[2]);

    if (R_SUCCEEDED(rc) && kernelAbove200())
        rc = _appletSetOutOfFocusSuspendingEnabled(invals[3]);

    return rc;
}

static Result _appletGetHandle(Service* srv, Handle* handle_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

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

static Result _appletGetSession(Service* srv, Service* srv_out, u64 cmd_id) {
    Result rc;
    Handle handle;

    rc = _appletGetHandle(srv, &handle, cmd_id);

    if (R_SUCCEEDED(rc)) {
        serviceCreate(srv_out, handle);
    }

    return rc;
}

static Result _appletGetSessionProxy(Service* srv_out, u64 cmd_id, Handle prochandle, u8 *AppletAttribute) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 reserved;
    } *raw;

    ipcSendPid(&c);
    ipcSendHandleCopy(&c, prochandle);
    if (AppletAttribute) ipcAddSendBuffer(&c, AppletAttribute, 0x80, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->reserved = 0;

    Result rc = serviceIpcDispatch(&g_appletSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(srv_out, r.Handles[0]);
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

    Result rc = serviceIpcDispatch(&g_appletIWindowController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

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

static Result _appletAcquireForegroundRights(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&g_appletIWindowController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletGetAppletResourceUserId(u64 *out) {
    if (!serviceIsActive(&g_appletSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    *out = g_appletResourceUserId;
    return 0;
}

Result appletGetDesiredLanguage(u64 *LanguageCode) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || __nx_applet_type != AppletType_Application)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 LanguageCode;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && LanguageCode) {
            *LanguageCode = resp->LanguageCode;
        }
    }

    return rc;
}

void appletNotifyRunning(u8 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (__nx_applet_type!=AppletType_Application || g_appletNotifiedRunning) return;
    g_appletNotifiedRunning = 1;

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 40;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

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

    if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletNotifyRunning));
}

static Result _appletReceiveMessage(u32 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}

static Result _appletGetOperationMode(u8 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}
static Result _appletGetPerformanceMode(u32 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}

static Result _appletGetCurrentFocusState(u8 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}

/*static Result _appletCmdNoIO(Service* session, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}*/

/*static Result _appletSelfExit(void) {
    return _appletCmdNoIO(&g_appletISelfController, 0);
}*/

/*static Result _appletLockExit(void) {
    return _appletCmdNoIO(&g_appletISelfController, 1);
}

static Result _appletUnlockExit(void) {
    return _appletCmdNoIO(&g_appletISelfController, 2);
}*/

Result appletSetScreenShotPermission(s32 val) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 val;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->val = val;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletSetOperationModeChangedNotification(u8 flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->flag = flag;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletSetPerformanceModeChangedNotification(u8 flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->flag = flag;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletSetFocusHandlingMode(u8 inval0, u8 inval1, u8 inval2) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 inval0;
        u8 inval1;
        u8 inval2;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;
    raw->inval0 = inval0;
    raw->inval1 = inval1;
    raw->inval2 = inval2;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletSetOutOfFocusSuspendingEnabled(u8 inval) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 inval;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 16;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletSetScreenShotImageOrientation(s32 val) {
    if (!kernelAbove300())
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 val;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 19;
    raw->val = val;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
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

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

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

/*static Result _appletExitProcessAndReturn(void) {
    return _appletCmdNoIO(&g_appletILibraryAppletSelfAccessor, 10);
}*/

u8 appletGetOperationMode(void) {
    return g_appletOperationMode;
}

u32 appletGetPerformanceMode(void) {
    return g_appletPerformanceMode;
}

u8 appletGetFocusState(void) {
    return g_appletFocusState;
}

bool appletMainLoop(void) {
    Result rc;
    u32    msg = 0;

    if (R_FAILED(svcWaitSynchronizationSingle(g_appletMessageEventHandle, 0)))
        return true;

    rc = _appletReceiveMessage(&msg);

    if (R_FAILED(rc))
    {
        if ((rc & 0x3fffff) == 0x680)
            return true;

        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletReceiveMessage));
    }

    switch(msg) {
        case 0xF:
            rc = _appletGetCurrentFocusState(&g_appletFocusState);
            if (R_FAILED(rc))
                fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetCurrentFocusState));

            appletCallHook(AppletHookType_OnFocusState);
        break;

        case 0x1E:
            rc = _appletGetOperationMode(&g_appletOperationMode);
            if (R_FAILED(rc))
                fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetOperationMode));

            appletCallHook(AppletHookType_OnOperationMode);
        break;

        case 0x1F:
            rc = _appletGetPerformanceMode(&g_appletPerformanceMode);
            if (R_FAILED(rc))
                fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetPerformanceMode));

            appletCallHook(AppletHookType_OnPerformanceMode);
        break;
    }

    return true;
}
