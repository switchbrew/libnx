#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/tmem.h"
#include "kernel/event.h"
#include "services/fatal.h"
#include "services/applet.h"
#include "services/apm.h"
#include "services/sm.h"
#include "runtime/env.h"
#include "runtime/hosversion.h"

__attribute__((weak)) u32 __nx_applet_type = AppletType_Default;
__attribute__((weak)) bool __nx_applet_auto_notifyrunning = true;
__attribute__((weak)) u8 __nx_applet_AppletAttribute[0x80];
__attribute__((weak)) u32 __nx_applet_PerformanceConfiguration[2] = {/*0x92220008*//*0x20004*//*0x92220007*/0, 0};
//// Controls whether to use applet exit cmds during \ref appletExit.  0 (default): Only run exit cmds when running under a NSO. 1: Use exit cmds regardless. >1: Skip exit cmds.
__attribute__((weak)) u32 __nx_applet_exit_mode = 0;

static Service g_appletSrv;
static Service g_appletProxySession;
static u64 g_refCnt;
static bool g_appletExitProcessFlag;

// From Get*Functions.
static Service g_appletIFunctions;

static Service g_appletIGlobalStateController;
static Service g_appletIApplicationCreator;

static Service g_appletILibraryAppletSelfAccessor;
static Service g_appletIProcessWindingController;

static Service g_appletILibraryAppletCreator;
static Service g_appletICommonStateGetter;
static Service g_appletISelfController;
static Service g_appletIWindowController;
static Service g_appletIAudioController;
static Service g_appletIDisplayController;
static Service g_appletIDebugFunctions;

static Event g_appletMessageEvent;

static u64 g_appletResourceUserId = 0;
static u8  g_appletOperationMode;
static u32 g_appletPerformanceMode;
static u8  g_appletFocusState;

static bool g_appletNotifiedRunning = 0;

static AppletHookCookie g_appletFirstHook;

static TransferMemory g_appletRecordingTmem;
static u32 g_appletRecordingInitialized;

static Event g_appletLibraryAppletLaunchableEvent;

static AppletThemeColorType g_appletThemeColorType = AppletThemeColorType_Default;

static Result _appletGetHandle(Service* srv, Handle* handle_out, u64 cmd_id);
static Result _appletGetEvent(Service* srv, Event* event_out, u64 cmd_id, bool autoclear);
static Result _appletGetSession(Service* srv, Service* srv_out, u64 cmd_id);
static Result _appletGetSessionProxy(Service* srv_out, u64 cmd_id, Handle prochandle, u8 *AppletAttribute);

static Result _appletGetAppletResourceUserId(u64 *out);

static Result _appletGetCurrentFocusState(u8 *out);
static Result _appletSetFocusHandlingMode(u8 inval0, u8 inval1, u8 inval2);
static Result _appletSetOutOfFocusSuspendingEnabled(u8 inval);

static Result _appletReceiveMessage(u32 *out);
static Result _appletAcquireForegroundRights(void);

static Result _appletGetOperationMode(u8 *out);
static Result _appletGetPerformanceMode(u32 *out);

static Result _appletSetOperationModeChangedNotification(u8 flag);
static Result _appletSetPerformanceModeChangedNotification(u8 flag);

static Result _appletSelfExit(void);

static Result _appletExitProcessAndReturn(void);

Result appletInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_appletSrv))
        return 0;

    if (__nx_applet_type == AppletType_None)
        return 0;

    if (R_FAILED(apmInitialize()))
        return MAKERESULT(Module_Libnx, LibnxError_ApmFailedToInitialize);

    Result rc = 0;

    g_appletResourceUserId = 0;
    g_appletNotifiedRunning = 0;
    g_appletExitProcessFlag = 0;
    g_appletRecordingInitialized = 0;

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
        rc = serviceConvertToDomain(&g_appletSrv);
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

    if (R_SUCCEEDED(rc) && __nx_applet_type == AppletType_SystemApplet) {
        //GetGlobalStateController
        rc = _appletGetSession(&g_appletProxySession, &g_appletIGlobalStateController, 21);

        //GetApplicationCreator
        if (R_SUCCEEDED(rc))
            rc = _appletGetSession(&g_appletProxySession, &g_appletIApplicationCreator, 22);
    }

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
        rc = _appletGetEvent(&g_appletICommonStateGetter, &g_appletMessageEvent, 0, false);

    if (R_SUCCEEDED(rc) && (__nx_applet_type == AppletType_Application))
    {
        rc = _appletGetCurrentFocusState(&g_appletFocusState);

        //Don't enter this msg-loop when g_appletFocusState is already 1, it will hang when applet was previously initialized in the context of the current process for AppletType_Application.
        if (R_SUCCEEDED(rc) && g_appletFocusState != AppletFocusState_Focused) {
            do {
                eventWait(&g_appletMessageEvent, U64_MAX);

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

            } while(g_appletFocusState != AppletFocusState_Focused);
        }

        if (R_SUCCEEDED(rc))
            rc = _appletAcquireForegroundRights();

        if (R_SUCCEEDED(rc))
            rc = appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleep);
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

static void NORETURN _appletExitProcess(int result_code) {
    appletExit();

    svcExitProcess();
    __builtin_unreachable();
}

static bool _appletIsApplication(void) {
    return __nx_applet_type == AppletType_Application || __nx_applet_type == AppletType_SystemApplication;
}

static bool _appletIsRegularApplication(void) {
    return __nx_applet_type == AppletType_Application;
}

void appletExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        if (!g_appletExitProcessFlag) {
            if (g_appletRecordingInitialized > 0) {
                if (g_appletRecordingInitialized == 2) appletSetGamePlayRecordingState(0);
            }

            if (__nx_applet_type == AppletType_Application) appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);
        }

        if ((envIsNso() && __nx_applet_exit_mode==0) || __nx_applet_exit_mode==1) {
            if (_appletIsApplication() ||
                __nx_applet_type == AppletType_LibraryApplet) {
                if (!g_appletExitProcessFlag) {
                    g_appletExitProcessFlag = 1;
                    atomicIncrement64(&g_refCnt);
                    envSetExitFuncPtr(_appletExitProcess);
                    return;
                }
                else {
                    if (_appletIsApplication()) {
                        //appletSetTerminateResult(0);
                        _appletSelfExit();
                    }
                    if (__nx_applet_type == AppletType_LibraryApplet)
                        _appletExitProcessAndReturn();
                }
            }
        }

        eventClose(&g_appletLibraryAppletLaunchableEvent);

        eventClose(&g_appletMessageEvent);

        serviceClose(&g_appletIDebugFunctions);
        serviceClose(&g_appletIDisplayController);
        serviceClose(&g_appletIAudioController);
        serviceClose(&g_appletIWindowController);
        serviceClose(&g_appletISelfController);
        serviceClose(&g_appletICommonStateGetter);
        serviceClose(&g_appletILibraryAppletCreator);

        if (__nx_applet_type == AppletType_SystemApplet) {
            serviceClose(&g_appletIApplicationCreator);
            serviceClose(&g_appletIGlobalStateController);
        }

        if (__nx_applet_type != AppletType_LibraryApplet)
            serviceClose(&g_appletIFunctions);

        if (__nx_applet_type == AppletType_LibraryApplet) {
            serviceClose(&g_appletIProcessWindingController);
            serviceClose(&g_appletILibraryAppletSelfAccessor);
        }

        serviceClose(&g_appletProxySession);
        serviceClose(&g_appletSrv);
        g_appletResourceUserId = 0;

        apmExit();

        if (g_appletRecordingInitialized > 0) {
            tmemClose(&g_appletRecordingTmem);
            g_appletRecordingInitialized = 0;
        }
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

void appletSetThemeColorType(AppletThemeColorType theme) {
    g_appletThemeColorType = theme;
}

AppletThemeColorType appletGetThemeColorType(void) {
    return g_appletThemeColorType;
}

Result appletSetFocusHandlingMode(AppletFocusHandlingMode mode) {
    Result rc;
    u8 invals[4];

    if (mode >= AppletFocusHandlingMode_Max)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(invals, 0, sizeof(invals));

    if ((mode == AppletFocusHandlingMode_SuspendHomeSleep) || (mode == AppletFocusHandlingMode_AlwaysSuspend)) {
        invals[0] = 0;
        invals[1] = 0;
        invals[2] = 1;
    }

    if (mode != AppletFocusHandlingMode_AlwaysSuspend) {
        invals[3] = 0;

        if (mode == AppletFocusHandlingMode_NoSuspend) {
            invals[0] = 1;
            invals[1] = 1;
            invals[2] = 0;
        }
        else if (mode == AppletFocusHandlingMode_SuspendHomeSleepNotify) {
            invals[0] = 1;
            invals[1] = 0;
            invals[2] = 1;
        }
    }
    else {
        invals[3] = 1;
    }

    rc = _appletSetFocusHandlingMode(invals[0], invals[1], invals[2]);

    if (R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0))
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

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

static Result _appletGetEvent(Service* srv, Event* event_out, u64 cmd_id, bool autoclear) {
    Handle tmp_handle=0;
    Result rc = 0;

    rc = _appletGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(event_out, tmp_handle, autoclear);
    return rc;
}

static Result _appletGetSession(Service* srv, Service* srv_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(srv_out, srv, &r, 0);
        }
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
    if (AppletAttribute) ipcAddSendBuffer(&c, AppletAttribute, 0x80, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_appletSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->reserved = 0;

    Result rc = serviceIpcDispatch(&g_appletSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(srv_out, &g_appletSrv, &r, 0);
        }
    }

    return rc;
}

static Result _appletGetSessionIn64(Service* srv, Service* srv_out, u64 cmd_id, u64 inval) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(srv_out, srv, &r, 0);
        }
    }

    return rc;
}

static Result _appletCmdNoIO(Service* session, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(session, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(session, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletCmdNoInOut64(Service* srv, u64 *out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 out;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) {
            *out = resp->out;
        }
    }

    return rc;
}

static Result _appletCmdInU8(Service* srv, u8 inval, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletCmdInU64(Service* srv, u64 inval, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletCmdInBool(Service* srv, bool inval, u64 cmd_id) {
    return _appletCmdInU8(srv, inval!=0, cmd_id);
}

static Result _appletCmdInHandle64(Service* srv, Service* srv_out, u64 cmd_id, Handle handle, u64 inval) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, handle);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && srv_out) {
            serviceCreateSubservice(srv_out, srv, &r, 0);
        }
    }

    return rc;
}

static Result _appletCmdInTmem(Service* srv, Service* srv_out, u64 cmd_id, TransferMemory *tmem) {
    return _appletCmdInHandle64(srv, srv_out, cmd_id, tmem->handle, tmem->size);
}

static Result _appletCmdInSession(Service* srv, Service* srv_in, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    serviceSendObject(srv_in, &c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletCmdInStorage(Service* srv, AppletStorage* s, u64 cmd_id) {
    Result rc=0;

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc =_appletCmdInSession(srv, &s->s, cmd_id);

    appletStorageClose(s);
    return rc;
}

static Result _appletCmdNoInOutStorage(Service* srv, AppletStorage* s, u64 cmd_id) {
    memset(s, 0, sizeof(AppletStorage));
    return _appletGetSession(srv, &s->s, cmd_id);
}

// IWindowController

static Result _appletGetAppletResourceUserId(u64 *out) {
    return _appletCmdNoInOut64(&g_appletIWindowController, out, 1);
}

static Result _appletAcquireForegroundRights(void) {
    return _appletCmdNoIO(&g_appletIWindowController, 10);
}

Result appletGetAppletResourceUserId(u64 *out) {
    if (!serviceIsActive(&g_appletSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    *out = g_appletResourceUserId;
    return 0;
}

// IFunctions

Result appletPopLaunchParameter(AppletStorage *s, AppletLaunchParameterKind kind) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 kind;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->kind = kind;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            memset(s, 0, sizeof(AppletStorage));
            serviceCreateSubservice(&s->s, &g_appletIFunctions, &r, 0);
        }
    }

    return rc;
}

static Result _appletCreateApplicationAndPushAndRequestToStart(Service* srv, u64 cmd_id, u64 titleID, AppletStorage* s) {
    IpcCommand c;
    ipcInitialize(&c);

    serviceSendObject(&s->s, &c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletCreateApplicationAndPushAndRequestToStartForQuest(u64 titleID, AppletStorage* s, const AppletApplicationAttributeForQuest *attr) { //2.0.0+
    IpcCommand c;
    ipcInitialize(&c);

    serviceSendObject(&s->s, &c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 val0, val1;
        u64 titleID;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->val0 = attr->unk_x0;
    raw->val1 = attr->unk_x4;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _appletCreateApplicationAndRequestToStart(u64 titleID) { //4.0.0+
    return _appletCmdInU64(&g_appletIFunctions, titleID, 12);
}

static Result _appletCreateApplicationAndRequestToStartForQuest(u64 titleID, const AppletApplicationAttributeForQuest *attr) { //4.0.0+
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 val0, val1;
        u64 titleID;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;
    raw->val0 = attr->unk_x0;
    raw->val1 = attr->unk_x4;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletRequestLaunchApplication(u64 titleID, AppletStorage* s) {
    AppletStorage tmpstorage={0};
    Result rc=0;
    bool is_libraryapplet = hosversionAtLeast(5,0,0) && __nx_applet_type == AppletType_LibraryApplet;

    if (!serviceIsActive(&g_appletSrv) || (!_appletIsApplication() && !is_libraryapplet))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (s && !serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if ((hosversionBefore(4,0,0) || is_libraryapplet) && s==NULL) {
        s = &tmpstorage;
        rc = appletCreateStorage(&tmpstorage, 0);
        if (R_FAILED(rc)) return rc;
    }

    if (is_libraryapplet) {
        rc = _appletCreateApplicationAndPushAndRequestToStart(&g_appletILibraryAppletSelfAccessor, 90, titleID, s);
    }
    else {
        if (hosversionAtLeast(4,0,0) && s==NULL) {
            rc = _appletCreateApplicationAndRequestToStart(titleID);
        }
        else {
            rc = _appletCreateApplicationAndPushAndRequestToStart(&g_appletIFunctions, 10, titleID, s);
        }
    }

    if (s) appletStorageClose(s);

    return rc;
}

Result appletRequestLaunchApplicationForQuest(u64 titleID, AppletStorage* s, const AppletApplicationAttributeForQuest *attr) {
    AppletStorage tmpstorage={0};
    Result rc=0;

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (s && !serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(4,0,0) && s==NULL) {
        s = &tmpstorage;
        rc = appletCreateStorage(&tmpstorage, 0);
        if (R_FAILED(rc)) return rc;
    }

    if (hosversionAtLeast(4,0,0) && s==NULL) {
        rc = _appletCreateApplicationAndRequestToStartForQuest(titleID, attr);
    }
    else {
        if (hosversionBefore(3,0,0)) rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
        if (R_SUCCEEDED(rc)) rc = _appletCreateApplicationAndPushAndRequestToStartForQuest(titleID, s, attr);
    }

    if (s) appletStorageClose(s);

    return rc;
}

Result appletGetDesiredLanguage(u64 *LanguageCode) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOut64(&g_appletIFunctions, LanguageCode, 21);
}

Result appletSetTerminateResult(Result res) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        Result res;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 22;
    raw->res = res;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletSetMediaPlaybackState(bool state) {
    if (!serviceIsActive(&g_appletSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (!_appletIsApplication())
        return _appletCmdInBool(&g_appletISelfController, state, 61);//SetMediaPlaybackState

    return _appletCmdInBool(&g_appletIFunctions, state, 60);//SetMediaPlaybackStateForApplication
}

Result appletBeginBlockingHomeButton(s64 val) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        s64 val;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 32;
    raw->val = val;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletEndBlockingHomeButton(void) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIFunctions, 33);
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

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 40;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) {
            *out = resp->out;
        }
    }

    if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletNotifyRunning));
}

Result appletIsGamePlayRecordingSupported(bool *flag) {
    IpcCommand c;
    ipcInitialize(&c);

    if (flag) *flag = 0;

    if (!serviceIsActive(&g_appletSrv) || !_appletIsRegularApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 flag;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && flag) *flag = resp->flag & 1;
    }

    return rc;
}

static Result _appletInitializeGamePlayRecording(TransferMemory *tmem) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsRegularApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInTmem(&g_appletIFunctions, NULL, 66, tmem);
}

Result appletSetGamePlayRecordingState(bool state) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsRegularApplication() || g_appletRecordingInitialized==0)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 state;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 67;
    raw->state = state==0 ? 0 : 1;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletInitializeGamePlayRecording(void) {
    Result rc=0;
    size_t size = 0x6000000;

    g_appletRecordingInitialized = 0;

    //These checks are done in the called applet funcs, but do it here too so that tmemCreate() doesn't run when it's not needed.
    if (!serviceIsActive(&g_appletSrv) || !_appletIsRegularApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = tmemCreate(&g_appletRecordingTmem, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = _appletInitializeGamePlayRecording(&g_appletRecordingTmem);
    if (R_FAILED(rc)) {
        tmemClose(&g_appletRecordingTmem);
        return rc;
    }

    g_appletRecordingInitialized = 1;

    rc = appletSetGamePlayRecordingState(1);
    if (R_SUCCEEDED(rc)) g_appletRecordingInitialized = 2;

    return rc;
}

//Official sw has this under 'pdm'.
Result appletQueryApplicationPlayStatistics(AppletApplicationPlayStatistics *stats, const u64 *titleIDs, s32 count, s32 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsRegularApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    ipcAddRecvBuffer(&c, stats, count*sizeof(AppletApplicationPlayStatistics), BufferType_Normal);
    ipcAddSendBuffer(&c, titleIDs, count*sizeof(u64), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 110;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 out;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

// IOverlayFunctions

Result appletBeginToWatchShortHomeButtonMessage(void) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _appletCmdNoIO(&g_appletIFunctions, 0);
}

Result appletEndToWatchShortHomeButtonMessage(void) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _appletCmdNoIO(&g_appletIFunctions, 1);
}

// ICommonStateGetter

Result appletHomeButtonReaderLockAccessorGetEvent(Event *out_event) {
    Service ILockAccessor = {0};
    Result rc = _appletGetSession(&g_appletICommonStateGetter, &ILockAccessor, 30);
    if (R_FAILED(rc))
        return rc;
    
    rc = _appletGetEvent(&ILockAccessor, out_event, 3, false);
    serviceClose(&ILockAccessor);
    return rc;
}

static Result _appletReceiveMessage(u32 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletICommonStateGetter, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_appletICommonStateGetter, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletICommonStateGetter, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&g_appletICommonStateGetter, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletICommonStateGetter, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_appletICommonStateGetter, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletICommonStateGetter, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&g_appletICommonStateGetter, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }

    return rc;
}

Result appletPushToGeneralChannel(AppletStorage *s) {
    return _appletCmdInStorage(&g_appletICommonStateGetter, s, 20);
}

// ISelfController

static Result _appletSelfExit(void) {
    return _appletCmdNoIO(&g_appletISelfController, 0);
}

Result appletLockExit(void) {
    return _appletCmdNoIO(&g_appletISelfController, 1);
}

Result appletUnlockExit(void) {
    return _appletCmdNoIO(&g_appletISelfController, 2);
}

static Result _appletWaitLibraryAppletLaunchableEvent(void) {
    Result rc=0;

    if (!eventActive(&g_appletLibraryAppletLaunchableEvent))
        rc = _appletGetEvent(&g_appletISelfController, &g_appletLibraryAppletLaunchableEvent, 9, false);

    if (R_SUCCEEDED(rc)) rc = eventWait(&g_appletLibraryAppletLaunchableEvent, U64_MAX);

    return rc;
}

Result appletSetScreenShotPermission(s32 val) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 val;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->val = val;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->flag = flag;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->flag = flag;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;
    raw->inval0 = inval0;
    raw->inval1 = inval1;
    raw->inval2 = inval2;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 16;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletSetScreenShotImageOrientation(s32 val) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 val;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 19;
    raw->val = val;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletCreateManagedDisplayLayer(u64 *out) {
    return _appletCmdNoInOut64(&g_appletISelfController, out, 40);
}

// ILibraryAppletSelfAccessor

static Result _appletExitProcessAndReturn(void) {
    return _appletCmdNoIO(&g_appletILibraryAppletSelfAccessor, 10);
}

// ILibraryAppletCreator

static Result _appletCreateLibraryApplet(Service* srv_out, AppletId id, LibAppletMode mode) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 id;
        u32 mode;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletILibraryAppletCreator, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->id = id;
    raw->mode = mode;

    Result rc = serviceIpcDispatch(&g_appletILibraryAppletCreator);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletILibraryAppletCreator, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && srv_out) {
            serviceCreateSubservice(srv_out, &g_appletILibraryAppletCreator, &r, 0);
        }
    }

    return rc;
}

static Result _appletGetIndirectLayerConsumerHandle(Service* srv, u64 *out) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc)) return rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 160;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 out;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

static Result _appletHolderCreate(AppletHolder *h, AppletId id, LibAppletMode mode, bool creating_self) {
    Result rc=0;

    memset(h, 0, sizeof(AppletHolder));
    h->mode = mode;
    h->creating_self = creating_self;

    if (!h->creating_self) rc = _appletWaitLibraryAppletLaunchableEvent();

    if (R_SUCCEEDED(rc)) rc = _appletCreateLibraryApplet(&h->s, id, mode);

    if (R_SUCCEEDED(rc)) rc = _appletGetEvent(&h->s, &h->StateChangedEvent, 0, false);//GetAppletStateChangedEvent

    if (R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0) && h->mode == LibAppletMode_Unknown3) rc = _appletGetIndirectLayerConsumerHandle(&h->s, &h->layer_handle);

    return rc;
}

Result appletCreateLibraryApplet(AppletHolder *h, AppletId id, LibAppletMode mode) {
    return _appletHolderCreate(h, id, mode, false);
}

Result appletCreateLibraryAppletSelf(AppletHolder *h, AppletId id, LibAppletMode mode) {
    return _appletHolderCreate(h, id, mode, true);
}

void appletHolderClose(AppletHolder *h) {
    eventClose(&h->PopInteractiveOutDataEvent);

    eventClose(&h->StateChangedEvent);
    serviceClose(&h->s);
    memset(h, 0, sizeof(AppletHolder));
}

bool appletHolderActive(AppletHolder *h) {
    return serviceIsActive(&h->s);
}

Result appletHolderGetIndirectLayerConsumerHandle(AppletHolder *h, u64 *out) {
    if (!serviceIsActive(&h->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (h->mode!=LibAppletMode_Unknown3)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (out) *out = h->layer_handle;

    return 0;
}

Result appletHolderStart(AppletHolder *h) {
    Result rc=0;

    if (!serviceIsActive(&h->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (!h->creating_self) rc = _appletWaitLibraryAppletLaunchableEvent();

    if (R_SUCCEEDED(rc)) rc = _appletCmdNoIO(&h->s, 10);//Start

    return rc;
}

Result appletHolderRequestExit(AppletHolder *h) {
    Result rc=0;

    if (!serviceIsActive(&h->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (!appletHolderCheckFinished(h)) rc = _appletCmdNoIO(&h->s, 20);//RequestExit

    return rc;
}

void appletHolderJoin(AppletHolder *h) {
    Result rc=0;
    LibAppletExitReason res = LibAppletExitReason_Normal;
    u32 desc=0;

    eventWait(&h->StateChangedEvent, U64_MAX);
    rc = _appletCmdNoIO(&h->s, 30);//GetResult

    if (R_FAILED(rc)) {
        res = LibAppletExitReason_Unexpected;
        if (R_MODULE(rc) == 128) {
            desc = R_DESCRIPTION(rc);
            if (desc == 22) res = LibAppletExitReason_Canceled;
            else {
                if (desc >= 0x14 && desc < 0x32)res = LibAppletExitReason_Abnormal;
            }
        }
    }

    h->exitreason = res;
}

bool appletHolderCheckFinished(AppletHolder *h) {
    return R_SUCCEEDED(eventWait(&h->StateChangedEvent, 0));
}

u32 appletHolderGetExitReason(AppletHolder *h) {
    return h->exitreason;
}

static Result _appletHolderGetPopInteractiveOutDataEvent(AppletHolder *h) {
    Result rc=0;

    if (eventActive(&h->PopInteractiveOutDataEvent)) return 0;

    rc = _appletGetEvent(&h->s, &h->PopInteractiveOutDataEvent, 106, false);

    return rc;
}

bool appletHolderWaitInteractiveOut(AppletHolder *h) {
    Result rc=0;
    s32 idx = 0;

    rc = _appletHolderGetPopInteractiveOutDataEvent(h);
    if (R_FAILED(rc)) return false;

    rc = waitMulti(&idx, U64_MAX, waiterForEvent(&h->PopInteractiveOutDataEvent), waiterForEvent(&h->StateChangedEvent));
    if (R_FAILED(rc)) return false;

    return idx==0;
}

Result appletHolderPushInData(AppletHolder *h, AppletStorage *s) {
    return _appletCmdInStorage(&h->s, s, 100);
}

Result appletHolderPopOutData(AppletHolder *h, AppletStorage *s) {
    return _appletCmdNoInOutStorage(&h->s, s, 101);
}

Result appletHolderPushExtraStorage(AppletHolder *h, AppletStorage *s) {
    return _appletCmdInStorage(&h->s, s, 102);
}

Result appletHolderPushInteractiveInData(AppletHolder *h, AppletStorage *s) {
    return _appletCmdInStorage(&h->s, s, 103);
}

Result appletHolderPopInteractiveOutData(AppletHolder *h, AppletStorage *s) {
    return _appletCmdNoInOutStorage(&h->s, s, 104);
}

Result appletCreateStorage(AppletStorage *s, s64 size) {
    memset(s, 0, sizeof(AppletStorage));

    return _appletGetSessionIn64(&g_appletILibraryAppletCreator, &s->s, 10, size);
}

static Result _appletCreateTransferMemoryStorage(Service* srv_out, TransferMemory *tmem, bool writable) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 writable;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletILibraryAppletCreator, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->writable = writable!=0;
    raw->size = tmem->size;

    Result rc = serviceIpcDispatch(&g_appletILibraryAppletCreator);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletILibraryAppletCreator, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && srv_out) {
            serviceCreateSubservice(srv_out, &g_appletILibraryAppletCreator, &r, 0);
        }
    }

    return rc;
}

Result appletCreateTransferMemoryStorage(AppletStorage *s, void* buffer, s64 size, bool writable) {
    Result rc=0;

    memset(s, 0, sizeof(AppletStorage));

    if (buffer==NULL) rc = tmemCreate(&s->tmem, size, Perm_None);
    else rc = tmemCreateFromMemory(&s->tmem, buffer, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = _appletCreateTransferMemoryStorage(&s->s, &s->tmem, writable);
    if (R_FAILED(rc)) tmemClose(&s->tmem);

    return rc;
}

Result appletCreateHandleStorage(AppletStorage *s, s64 inval, Handle handle) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInHandle64(&g_appletILibraryAppletCreator, &s->s, 12, handle, inval);
}

Result appletCreateHandleStorageTmem(AppletStorage *s, void* buffer, s64 size) {
    Result rc=0;

    memset(s, 0, sizeof(AppletStorage));

    if (buffer==NULL) rc = tmemCreate(&s->tmem, size, Perm_None);
    else rc = tmemCreateFromMemory(&s->tmem, buffer, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = appletCreateHandleStorage(s, s->tmem.size, s->tmem.handle);
    if (R_FAILED(rc)) tmemClose(&s->tmem);

    return rc;
}

void appletStorageClose(AppletStorage *s) {
    serviceClose(&s->s);
}

void appletStorageCloseTmem(AppletStorage *s) {
    tmemClose(&s->tmem);
}

static Result _appletStorageAccessorRW(Service* srv, size_t ipcbufsize, s64 offset, void* buffer, size_t size, bool rw) {
    IpcCommand c;
    ipcInitialize(&c);

    if(!rw)ipcAddRecvSmart(&c, ipcbufsize, buffer, size, 0);
    if(rw)ipcAddSendSmart(&c, ipcbufsize, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        s64 offset;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = rw!=0 ? 10 : 11;
    raw->offset = offset;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletStorageGetSize(AppletStorage *s, s64 *size) {
    Result rc=0;
    Service tmp_srv;//IStorageAccessor

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletGetSession(&s->s, &tmp_srv, 0);//Open
    if (R_FAILED(rc)) return rc;

    rc = _appletCmdNoInOut64(&tmp_srv, (u64*)size, 0);
    serviceClose(&tmp_srv);

    return rc;
}

static Result _appletStorageRW(AppletStorage *s, s64 offset, void* buffer, size_t size, bool rw) {
    Result rc=0;
    size_t ipcbufsize=0;
    Service tmp_srv;//IStorageAccessor

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletGetSession(&s->s, &tmp_srv, 0);//Open
    if (R_FAILED(rc)) return rc;

    rc = ipcQueryPointerBufferSize(tmp_srv.handle, &ipcbufsize);

    if (R_SUCCEEDED(rc)) rc = _appletStorageAccessorRW(&tmp_srv, ipcbufsize, offset, buffer, size, rw);
    serviceClose(&tmp_srv);

    return rc;
}

Result appletStorageWrite(AppletStorage *s, s64 offset, const void* buffer, size_t size) {
    return _appletStorageRW(s, offset, (void*)buffer, size, true);
}

Result appletStorageRead(AppletStorage *s, s64 offset, void* buffer, size_t size) {
    return _appletStorageRW(s, offset, buffer, size, false);
}

static Result _appletStorageGetHandle(Service* srv, s64 *out, Handle *handle) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s64 out;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->out;
            if (handle) *handle = r.Handles[0];
        }
    }

    return rc;
}

Result appletStorageGetHandle(AppletStorage *s, s64 *out, Handle *handle) {
    Result rc=0;
    Service tmp_srv;//ITransferStorageAccessor

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletGetSession(&s->s, &tmp_srv, 1);//OpenTransferStorage
    if (R_FAILED(rc)) return rc;

    rc = _appletStorageGetHandle(&tmp_srv, out, handle);
    serviceClose(&tmp_srv);

    return rc;
}

Result appletStorageMap(AppletStorage *s, void** addr, size_t *size) {
    Result rc=0;
    s64 tmpsize=0;
    Handle tmphandle=0;

    rc = appletStorageGetHandle(s, &tmpsize, &tmphandle);
    if (R_FAILED(rc)) return rc;

    tmemLoadRemote(&s->tmem, tmphandle, tmpsize, Perm_None);
    rc = tmemMap(&s->tmem);
    if (R_FAILED(rc)) tmemClose(&s->tmem);

    if (R_SUCCEEDED(rc)) {
        if (addr) *addr = s->tmem.map_addr;
        if (size) *size = s->tmem.size;
    }

    return rc;
}

// state / other

u8 appletGetOperationMode(void) {
    return g_appletOperationMode;
}

u32 appletGetPerformanceMode(void) {
    return g_appletPerformanceMode;
}

AppletFocusState appletGetFocusState(void) {
    return (AppletFocusState)g_appletFocusState;
}

Result appletGetMessage(u32 *msg) {
    Result rc=0;
    if (msg==NULL) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    rc = eventWait(&g_appletMessageEvent, 0);
    if (R_FAILED(rc))
        return rc;

    rc = _appletReceiveMessage(msg);

    if (R_FAILED(rc)) {
        if (R_VALUE(rc) == MAKERESULT(128, 3))
            return rc;

        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletReceiveMessage));
    }

    return 0;
}

bool appletProcessMessage(u32 msg) {
    Result rc;

    switch(msg) {
        case 0x4:
            appletCallHook(AppletHookType_OnExitRequest);
            return false;
        break;

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

bool appletMainLoop(void) {
    u32 msg = 0;

    if (R_FAILED(appletGetMessage(&msg))) return true;

    return appletProcessMessage(msg);
}
