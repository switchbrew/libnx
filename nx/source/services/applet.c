#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "arm/counter.h"
#include "kernel/ipc.h"
#include "kernel/tmem.h"
#include "kernel/event.h"
#include "services/fatal.h"
#include "services/applet.h"
#include "services/apm.h"
#include "services/caps.h"
#include "services/pm.h"
#include "services/sm.h"
#include "services/fs.h"
#include "runtime/env.h"
#include "runtime/hosversion.h"
#include "nacp.h"

__attribute__((weak)) u32 __nx_applet_type = AppletType_Default;
__attribute__((weak)) bool __nx_applet_auto_notifyrunning = true;
__attribute__((weak)) AppletAttribute __nx_applet_AppletAttribute;
/// When set, controls the PerformanceConfiguration passed to apmSetPerformanceConfiguration during app startup, where the array index is the PerformanceMode.
__attribute__((weak)) u32 __nx_applet_PerformanceConfiguration[2] = {/*0x92220008*//*0x20004*//*0x92220007*/0, 0};
//// Controls whether to use applet exit cmds during \ref appletExit.  0 (default): Only run exit cmds when running under a NSO. 1: Use exit cmds regardless. >1: Skip exit cmds.
__attribute__((weak)) u32 __nx_applet_exit_mode = 0;

static Service g_appletSrv;
static Service g_appletProxySession;
static u64 g_refCnt;
static bool g_appletExitProcessFlag;
static Result g_appletExitProcessResult;

// From Get*Functions.
static Service g_appletIAppletCommonFunctions;

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

static size_t g_appletIAppletCommonFunctions_ptrbufsize;
static size_t g_appletISelfController_ptrbufsize;

static Event g_appletMessageEvent;

static u64 g_appletResourceUserId = 0;
static u8  g_appletOperationMode;
static u32 g_appletPerformanceMode;
static u8  g_appletFocusState;

static bool g_appletNotifiedRunning = 0;

static AppletHookCookie g_appletFirstHook;

static TransferMemory g_appletRecordingTmem;
static u32 g_appletRecordingInitialized;

static TransferMemory g_appletCopyrightTmem;
static bool g_appletCopyrightInitialized;

static Event g_appletSuspendedTickEvent;
static u64 g_appletInitTickBase;
static u64 g_appletSuspendedTick;
static bool g_appletSuspendedTickInitialized;

static Event g_appletLibraryAppletLaunchableEvent;

static AppletThemeColorType g_appletThemeColorType = AppletThemeColorType_Default;

static ApmCpuBoostMode g_appletCpuBoostMode = ApmCpuBoostMode_Disabled;

static AppletInfo g_appletInfo;
static bool g_appletInfoInitialized;

static Result _appletGetHandle(Service* srv, Handle* handle_out, u64 cmd_id);
static Result _appletGetEvent(Service* srv, Event* out_event, u64 cmd_id, bool autoclear);
static Result _appletGetSession(Service* srv, Service* srv_out, u64 cmd_id);
static Result _appletGetSessionProxy(Service* srv_out, u64 cmd_id, Handle prochandle, const AppletAttribute *attr);

static Result _appletGetAppletResourceUserId(u64 *out);

static Result _appletGetCurrentFocusState(u8 *out);
static Result _appletSetFocusHandlingMode(bool inval0, bool inval1, bool inval2);
static Result _appletSetOutOfFocusSuspendingEnabled(bool inval);

static Result _appletReceiveMessage(u32 *out);
static Result _appletAcquireForegroundRights(void);

static Result _appletGetOperationMode(u8 *out);
static Result _appletGetPerformanceMode(u32 *out);

static Result _appletSetOperationModeChangedNotification(bool flag);
static Result _appletSetPerformanceModeChangedNotification(bool flag);

static Result _appletSelfExit(void);

static Result _appletExitProcessAndReturn(void);

static Result _appletGetAccumulatedSuspendedTickValue(u64 *tick);
static Result _appletGetAccumulatedSuspendedTickChangedEvent(Event *out_event);

static Result _appletGetLaunchReason(AppletProcessLaunchReason *reason);
static Result _appletOpenCallingLibraryApplet(AppletHolder *h);

static Result _appletHolderCreateState(AppletHolder *h, LibAppletMode mode, bool creating_self);
static Result _appletOpenExistingLibraryApplet(AppletHolder *h, Service* srv, u64 cmd_id);

Result appletInitialize(void)
{
    AppletAttribute *attr = NULL;

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

    g_appletInfoInitialized = 0;
    memset(&g_appletInfo, 0, sizeof(g_appletInfo));

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

            if (__nx_applet_type == AppletType_LibraryApplet && hosversionAtLeast(3,0,0)) {
                cmd_id = 201;
                attr = &__nx_applet_AppletAttribute;
            }

            rc = _appletGetSessionProxy(&g_appletProxySession, cmd_id, CUR_PROCESS_HANDLE, attr);

            if (rc == AM_BUSY_ERROR) {
                svcSleepThread(10000000);
            }

        } while (rc == AM_BUSY_ERROR);
    }

    // [7.0.0+] GetAppletCommonFunctions
    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) {
        if (__nx_applet_type == AppletType_SystemApplet || __nx_applet_type == AppletType_LibraryApplet || __nx_applet_type == AppletType_OverlayApplet) {
            rc = _appletGetSession(&g_appletProxySession, &g_appletIAppletCommonFunctions, __nx_applet_type == AppletType_SystemApplet ? 23 : 21);
            if (R_SUCCEEDED(rc)) rc = ipcQueryPointerBufferSize(g_appletIAppletCommonFunctions.handle, &g_appletIAppletCommonFunctions_ptrbufsize);
        }
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

    if (R_SUCCEEDED(rc))
        rc = ipcQueryPointerBufferSize(g_appletISelfController.handle, &g_appletISelfController_ptrbufsize);

    Result rc2 = _appletGetAccumulatedSuspendedTickChangedEvent(&g_appletSuspendedTickEvent);
    if (R_SUCCEEDED(rc2)) {
        g_appletInitTickBase = armGetSystemTick();
        g_appletSuspendedTick = 0;
        g_appletSuspendedTickInitialized = true;
    }

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
        u32 i;
        for (i=0; i<2; i++)
        {
            if (__nx_applet_PerformanceConfiguration[i])
                rc = apmSetPerformanceConfiguration(i, __nx_applet_PerformanceConfiguration[i]);

            if (R_FAILED(rc))
                break;
        }
    }

    if (R_SUCCEEDED(rc) && __nx_applet_type == AppletType_LibraryApplet) {
        AppletProcessLaunchReason launchreason={0};

        Result rc2 = appletGetLibraryAppletInfo(&g_appletInfo.info);

        if (R_SUCCEEDED(rc2)) rc2 = _appletGetLaunchReason(&launchreason);

        if (R_SUCCEEDED(rc2)) {
            g_appletInfo.caller_flag = launchreason.flag!=0;
            if (g_appletInfo.caller_flag) rc2 = _appletOpenCallingLibraryApplet(&g_appletInfo.caller);
        }

        if (R_SUCCEEDED(rc2)) g_appletInfoInitialized = true;
    }

    if (R_FAILED(rc))
        appletExit();

    return rc;
}

static void _appletInfiniteSleepLoop(void) {
    while(1) svcSleepThread(86400000000000ULL);
}

static void NORETURN _appletExitProcess(int result_code) {
    appletExit();

    if (R_SUCCEEDED(g_appletExitProcessResult)) _appletInfiniteSleepLoop();

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

            if (g_appletCpuBoostMode != ApmCpuBoostMode_Disabled) appletSetCpuBoostMode(ApmCpuBoostMode_Disabled);
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
                    if (_appletIsApplication())
                        g_appletExitProcessResult = _appletSelfExit();
                    if (__nx_applet_type == AppletType_LibraryApplet)
                        g_appletExitProcessResult = _appletExitProcessAndReturn();
                }
            }
        }

        if (g_appletInfoInitialized) {
            if (g_appletInfo.caller_flag) appletHolderClose(&g_appletInfo.caller);
            g_appletInfoInitialized = 0;
            memset(&g_appletInfo, 0, sizeof(g_appletInfo));
        }

        eventClose(&g_appletLibraryAppletLaunchableEvent);

        eventClose(&g_appletMessageEvent);

        if (g_appletSuspendedTickInitialized) {
            eventClose(&g_appletSuspendedTickEvent);
            g_appletSuspendedTickInitialized = false;
        }

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

        serviceClose(&g_appletIAppletCommonFunctions);

        serviceClose(&g_appletProxySession);
        serviceClose(&g_appletSrv);
        g_appletResourceUserId = 0;

        apmExit();

        if (g_appletRecordingInitialized > 0) {
            tmemClose(&g_appletRecordingTmem);
            g_appletRecordingInitialized = 0;
        }

        if (g_appletCopyrightInitialized) {
            tmemClose(&g_appletCopyrightTmem);
            g_appletCopyrightInitialized = 0;
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
    bool invals[4];

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

static Result _appletGetEvent(Service* srv, Event* out_event, u64 cmd_id, bool autoclear) {
    Handle tmp_handle=0;
    Result rc = 0;

    rc = _appletGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
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

static Result _appletGetSessionProxy(Service* srv_out, u64 cmd_id, Handle prochandle, const AppletAttribute *attr) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 reserved;
    } *raw;

    ipcSendPid(&c);
    ipcSendHandleCopy(&c, prochandle);
    if (attr) ipcAddSendBuffer(&c, attr, sizeof(AppletAttribute), BufferType_Normal);

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

static Result _appletGetSessionIn32(Service* srv, Service* srv_out, u64 cmd_id, u32 inval) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 inval;
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

static Result _appletCmdNoInOut32(Service* srv, u32 *out, u64 cmd_id) {
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
            u32 out;
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

static Result _appletCmdNoInOutU8(Service* srv, u8 *out, u64 cmd_id) {
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
            u8 out;
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

static Result _appletCmdNoInOutBool(Service* srv, bool *out, u64 cmd_id) {
    u8 tmp=0;
    Result rc = _appletCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp!=0;
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

static Result _appletCmdInU32(Service* srv, u32 inval, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 inval;
    } PACKED *raw;

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

static Result _appletCmdSendBufNoOut(Service* srv, const void* buffer, size_t size, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buffer, size, BufferType_Normal);

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

static Result _appletCmdNoInRecvBuf(Service* srv, void* buffer, size_t size, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, buffer, size, BufferType_Normal);

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

static Result _appletGetLibraryAppletInfo(Service* srv, LibAppletInfo *info, u64 cmd_id) {
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
            LibAppletInfo info;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && info) memcpy(info, &resp->info, sizeof(LibAppletInfo));
    }

    return rc;
}

static Result _appletGetIdentityInfo(Service* srv, AppletIdentityInfo *info, u64 cmd_id) {
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
            AppletIdentityInfo info;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && info) memcpy(info, &resp->info, sizeof(AppletIdentityInfo));
    }

    return rc;
}

static Result _appletGetResolution(Service* srv, s32 *width, s32 *height, u64 cmd_id) {
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
            s32 width;
            s32 height;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (width) *width = resp->width;
            if (height) *height = resp->height;
        }
    }

    return rc;
}

// ICommonStateGetter

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

Result appletGetCradleStatus(u8 *status) {
    return _appletCmdNoInOutU8(&g_appletICommonStateGetter, status, 7);
}

Result appletGetBootMode(PmBootMode *mode) {
    u8 tmp=0;
    Result rc = _appletCmdNoInOutU8(&g_appletICommonStateGetter, &tmp, 8);
    if (R_SUCCEEDED(rc) && mode) *mode = tmp;
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

static Result _appletGetAcquiredSleepLockEvent(Event *out_event) {
    return _appletGetEvent(&g_appletICommonStateGetter, out_event, 13, false);
}

static Result _appletWaitAcquiredSleepLockEvent(void) {
    Result rc=0;
    Event tmpevent={0};

    rc = _appletGetAcquiredSleepLockEvent(&tmpevent);
    if (R_SUCCEEDED(rc)) rc = eventWait(&tmpevent, U64_MAX);
    eventClose(&tmpevent);
    return rc;
}

Result appletRequestToAcquireSleepLock(void) {
    Result rc = _appletCmdNoIO(&g_appletICommonStateGetter, 10);
    if (R_SUCCEEDED(rc)) rc = _appletWaitAcquiredSleepLockEvent();
    return rc;
}

Result appletReleaseSleepLock(void) {
    return _appletCmdNoIO(&g_appletICommonStateGetter, 11);
}

Result appletReleaseSleepLockTransiently(void) {
    Result rc = _appletCmdNoIO(&g_appletICommonStateGetter, 12);
    if (R_SUCCEEDED(rc)) rc = _appletWaitAcquiredSleepLockEvent();
    return rc;
}

Result appletPushToGeneralChannel(AppletStorage *s) {
    return _appletCmdInStorage(&g_appletICommonStateGetter, s, 20);
}

static Result _appletGetHomeButtonRwLockAccessor(Service* srv, AppletLockAccessor *a, u64 cmd_id) {
    Result rc = _appletGetSession(srv, &a->s, cmd_id);
    if (R_FAILED(rc))
        return rc;

    rc = _appletGetEvent(&a->s, &a->event, 3, false);
    if (R_FAILED(rc)) serviceClose(&a->s);
    return rc;
}

Result appletGetHomeButtonReaderLockAccessor(AppletLockAccessor *a) {
    return _appletGetHomeButtonRwLockAccessor(&g_appletICommonStateGetter, a, 30);
}

static Result _appletGetRwLockAccessor(Service* srv, AppletLockAccessor *a, u64 cmd_id, s32 inval) {
    Result rc = _appletGetSessionIn32(srv, &a->s, cmd_id, inval);
    if (R_FAILED(rc))
        return rc;

    rc = _appletGetEvent(&a->s, &a->event, 3, false);
    if (R_FAILED(rc)) serviceClose(&a->s);
    return rc;
}

Result appletGetReaderLockAccessorEx(AppletLockAccessor *a, u32 inval) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetRwLockAccessor(&g_appletICommonStateGetter, a, 31, inval);
}

Result appletGetWriterLockAccessorEx(AppletLockAccessor *a, u32 inval) {
    if (hosversionBefore(7,0,0)) {
        if (__nx_applet_type == AppletType_SystemApplet && hosversionAtLeast(2,0,0))
            return _appletGetRwLockAccessor(&g_appletIFunctions, a, 31, inval);

        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    return _appletGetRwLockAccessor(&g_appletICommonStateGetter, a, 32, inval);
}

Result appletGetCradleFwVersion(u32 *out0, u32 *out1, u32 *out2, u32 *out3) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletICommonStateGetter, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 40;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out0;
            u32 out1;
            u32 out2;
            u32 out3;
        } *resp;

        serviceIpcParse(&g_appletICommonStateGetter, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out0) *out0 = resp->out0;
            if (out1) *out1 = resp->out1;
            if (out2) *out2 = resp->out2;
            if (out3) *out3 = resp->out3;
        }
    }

    return rc;
}

Result appletIsVrModeEnabled(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletICommonStateGetter, out, 50);
}

Result appletSetVrModeEnabled(bool flag) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (hosversionBefore(7,0,0))
        return _appletCmdInBool(&g_appletICommonStateGetter, flag, 51);

    return _appletCmdNoIO(&g_appletICommonStateGetter, flag ? 53 : 54);
}

Result appletSetLcdBacklightOffEnabled(bool flag) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletICommonStateGetter, flag, 52);
}

Result appletIsInControllerFirmwareUpdateSection(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletICommonStateGetter, out, 55);
}

Result appletGetDefaultDisplayResolution(s32 *width, s32 *height) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetResolution(&g_appletICommonStateGetter, width, height, 60);
}

Result appletGetDefaultDisplayResolutionChangeEvent(Event *out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetEvent(&g_appletICommonStateGetter, out_event, 61, true);
}

Result appletGetHdcpAuthenticationState(s32 *state) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut32(&g_appletICommonStateGetter, (u32*)state, 62);
}

Result appletGetHdcpAuthenticationStateChangeEvent(Event *out_event) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetEvent(&g_appletICommonStateGetter, out_event, 63, true);
}

Result appletSetTvPowerStateMatchingMode(AppletTvPowerStateMatchingMode mode) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletICommonStateGetter, mode, 64);
}

Result appletGetApplicationIdByContentActionName(u64 *titleID, const char *name) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, name, strlen(name)+1, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletICommonStateGetter, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65;

    Result rc = serviceIpcDispatch(&g_appletICommonStateGetter);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 titleID;
        } *resp;

        serviceIpcParse(&g_appletICommonStateGetter, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && titleID) *titleID = resp->titleID;
    }

    return rc;
}

Result appletSetCpuBoostMode(ApmCpuBoostMode mode) {
    Result rc=0;
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdInU32(&g_appletICommonStateGetter, mode, 66);
    if (R_SUCCEEDED(rc)) g_appletCpuBoostMode = mode;
    return rc;
}

Result appletPerformSystemButtonPressingIfInFocus(AppletSystemButtonType type) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletICommonStateGetter, type, 80);
}

Result appletSetPerformanceConfigurationChangedNotification(bool flag) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletICommonStateGetter, flag, 90);
}

Result appletGetCurrentPerformanceConfiguration(u32 *PerformanceConfiguration) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut32(&g_appletICommonStateGetter, PerformanceConfiguration, 91);
}

Result appletGetOperationModeSystemInfo(u32 *info) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut32(&g_appletICommonStateGetter, info, 200);
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

Result appletEnterFatalSection(void) {
    return _appletCmdNoIO(&g_appletISelfController, 3);
}

Result appletLeaveFatalSection(void) {
    return _appletCmdNoIO(&g_appletISelfController, 4);
}

static Result _appletWaitLibraryAppletLaunchableEvent(void) {
    Result rc=0;

    if (!eventActive(&g_appletLibraryAppletLaunchableEvent))
        rc = _appletGetEvent(&g_appletISelfController, &g_appletLibraryAppletLaunchableEvent, 9, false);

    if (R_SUCCEEDED(rc)) rc = eventWait(&g_appletLibraryAppletLaunchableEvent, U64_MAX);

    return rc;
}

Result appletSetScreenShotPermission(AppletScreenShotPermission permission) {
    return _appletCmdInU32(&g_appletISelfController, permission, 10);
}

static Result _appletSetOperationModeChangedNotification(bool flag) {
    return _appletCmdInBool(&g_appletISelfController, flag, 11);
}

static Result _appletSetPerformanceModeChangedNotification(bool flag) {
    return _appletCmdInBool(&g_appletISelfController, flag, 12);
}

static Result _appletSetFocusHandlingMode(bool inval0, bool inval1, bool inval2) {
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
    raw->inval0 = inval0!=0;
    raw->inval1 = inval1!=0;
    raw->inval2 = inval2!=0;

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

Result appletSetRestartMessageEnabled(bool flag) {
    return _appletCmdInBool(&g_appletISelfController, flag, 14);
}

Result appletSetScreenShotAppletIdentityInfo(AppletIdentityInfo *info) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        AppletIdentityInfo info;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
    memcpy(&raw->info, info, sizeof(AppletIdentityInfo));

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

static Result _appletSetOutOfFocusSuspendingEnabled(bool flag) {
    return _appletCmdInBool(&g_appletISelfController, flag, 16);
}

Result appletSetControllerFirmwareUpdateSection(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletISelfController, flag, 17);
}

Result appletSetRequiresCaptureButtonShortPressedMessage(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletISelfController, flag, 18);
}

Result appletSetAlbumImageOrientation(AlbumImageOrientation orientation) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletISelfController, orientation, 19);
}

Result appletSetDesirableKeyboardLayout(u32 layout) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletISelfController, layout, 20);
}

Result appletCreateManagedDisplayLayer(u64 *out) {
    return _appletCmdNoInOut64(&g_appletISelfController, out, 40);
}

Result appletIsSystemBufferSharingEnabled(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletISelfController, 41);
}

Result appletGetSystemSharedLayerHandle(u64 *SharedBufferHandle, u64 *SharedLayerHandle) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 42;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 SharedBufferHandle;
            u64 SharedLayerHandle;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && SharedBufferHandle) *SharedBufferHandle = resp->SharedBufferHandle;
        if (R_SUCCEEDED(rc) && SharedLayerHandle) *SharedLayerHandle = resp->SharedLayerHandle;
    }

    return rc;
}

Result appletGetSystemSharedBufferHandle(u64 *SharedBufferHandle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut64(&g_appletISelfController, SharedBufferHandle, 43);
}

Result appletSetHandlesRequestToDisplay(bool flag) {
    return _appletCmdInBool(&g_appletISelfController, flag, 50);
}

Result appletApproveToDisplay(void) {
    return _appletCmdNoIO(&g_appletISelfController, 51);
}

Result appletOverrideAutoSleepTimeAndDimmingTime(s32 inval0, s32 inval1, s32 inval2, s32 inval3) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 inval0;
        s32 inval1;
        s32 inval2;
        s32 inval3;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 60;
    raw->inval0 = inval0;
    raw->inval1 = inval1;
    raw->inval2 = inval2;
    raw->inval3 = inval3;

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

Result appletSetIdleTimeDetectionExtension(AppletIdleTimeDetectionExtension ext) {
    return _appletCmdInU32(&g_appletISelfController, ext, 62);
}

Result appletGetIdleTimeDetectionExtension(AppletIdleTimeDetectionExtension *ext) {
    return _appletCmdNoInOut32(&g_appletISelfController, ext, 63);
}

Result appletSetInputDetectionSourceSet(u32 val) {
    return _appletCmdInU32(&g_appletISelfController, val, 64);
}

Result appletReportUserIsActive(void) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletISelfController, 65);
}

Result appletGetCurrentIlluminance(float *fLux) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 66;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            float fLux;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && fLux) *fLux = resp->fLux;
    }

    return rc;
}

Result appletIsIlluminanceAvailable(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletISelfController, out, 67);
}

Result appletSetAutoSleepDisabled(bool flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletISelfController, flag, 68);
}

Result appletIsAutoSleepDisabled(bool *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletISelfController, out, 69);
}

Result appletGetCurrentIlluminanceEx(bool *bOverLimit, float *fLux) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 71;

    Result rc = serviceIpcDispatch(&g_appletISelfController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 bOverLimit;
            float fLux;
        } *resp;

        serviceIpcParse(&g_appletISelfController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && bOverLimit) *bOverLimit = resp->bOverLimit!=0;
        if (R_SUCCEEDED(rc) && fLux) *fLux = resp->fLux;
    }

    return rc;
}

Result appletSetWirelessPriorityMode(AppletWirelessPriorityMode mode) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletISelfController, mode, 80);
}

Result appletSetAlbumImageTakenNotificationEnabled(bool flag) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletISelfController, flag, 100);
}

static Result _appletGetAccumulatedSuspendedTickValue(u64 *tick) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut64(&g_appletISelfController, tick, 90);
}

static Result _appletGetAccumulatedSuspendedTickChangedEvent(Event *out_event) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetEvent(&g_appletISelfController, out_event, 91, true);
}

Result appletGetProgramTotalActiveTime(u64 *activeTime) {
    if (!g_appletSuspendedTickInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc=0;
    u64 suspendedTick = 0;

    while (R_SUCCEEDED(eventWait(&g_appletSuspendedTickEvent, 0))) {
        rc = _appletGetAccumulatedSuspendedTickValue(&suspendedTick);
        if (R_FAILED(rc)) return rc;

        if (suspendedTick > g_appletSuspendedTick) g_appletSuspendedTick = suspendedTick;
    }

    *activeTime = armTicksToNs(armGetSystemTick() - g_appletInitTickBase - g_appletSuspendedTick);

    return rc;
}

Result appletSetApplicationAlbumUserData(const void* buffer, size_t size) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendSmart(&c, g_appletISelfController_ptrbufsize, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletISelfController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 110;

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

// IWindowController

static Result _appletGetAppletResourceUserId(u64 *out) {
    return _appletCmdNoInOut64(&g_appletIWindowController, out, 1);
}

Result appletGetAppletResourceUserId(u64 *out) {
    if (!serviceIsActive(&g_appletSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    *out = g_appletResourceUserId;
    return 0;
}

Result appletGetAppletResourceUserIdOfCallerApplet(u64 *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_appletSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOut64(&g_appletIWindowController, out, 2);
}

static Result _appletAcquireForegroundRights(void) {
    return _appletCmdNoIO(&g_appletIWindowController, 10);
}

Result appletSetAppletWindowVisibility(bool flag) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletIWindowController, flag, 20);
}

Result appletSetAppletGpuTimeSlice(s64 val) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU64(&g_appletIWindowController, val, 21);
}

// IAudioController

Result appletSetExpectedMasterVolume(float mainAppletVolume, float libraryAppletVolume) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        float mainAppletVolume;
        float libraryAppletVolume;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIAudioController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->mainAppletVolume = mainAppletVolume;
    raw->libraryAppletVolume = libraryAppletVolume;

    Result rc = serviceIpcDispatch(&g_appletIAudioController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIAudioController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletGetExpectedMasterVolume(float *mainAppletVolume, float *libraryAppletVolume) {
    Result rc=0;

    if (mainAppletVolume) rc = _appletCmdNoInOut32(&g_appletIAudioController, (u32*)mainAppletVolume, 1); // GetMainAppletExpectedMasterVolume
    if (R_SUCCEEDED(rc) && libraryAppletVolume) rc = _appletCmdNoInOut32(&g_appletIAudioController, (u32*)libraryAppletVolume, 2); // GetLibraryAppletExpectedMasterVolume

    return rc;
}

Result appletChangeMainAppletMasterVolume(float volume, u64 unk) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        float volume;
        u64 unk;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIAudioController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->volume = volume;
    raw->unk = unk;

    Result rc = serviceIpcDispatch(&g_appletIAudioController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIAudioController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletSetTransparentVolumeRate(float val) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        float val;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIAudioController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->val = val;

    Result rc = serviceIpcDispatch(&g_appletIAudioController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIAudioController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

// IDisplayController

Result appletUpdateLastForegroundCaptureImage(void) {
    return _appletCmdNoIO(&g_appletIDisplayController, 1);
}

Result appletUpdateCallerAppletCaptureImage(void) {
    return _appletCmdNoIO(&g_appletIDisplayController, 4);
}

static Result _appletGetCaptureImageEx(void* buffer, size_t size, bool *flag, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, buffer, size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDisplayController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_appletIDisplayController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 flag;
        } *resp;

        serviceIpcParse(&g_appletIDisplayController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && flag) *flag = resp->flag!=0;
    }

    return rc;
}

Result appletGetLastForegroundCaptureImageEx(void* buffer, size_t size, bool *flag) {
    return _appletGetCaptureImageEx(buffer, size, flag, 5);
}

Result appletGetLastApplicationCaptureImageEx(void* buffer, size_t size, bool *flag) {
    return _appletGetCaptureImageEx(buffer, size, flag, 6);
}

Result appletGetCallerAppletCaptureImageEx(void* buffer, size_t size, bool *flag) {
    return _appletGetCaptureImageEx(buffer, size, flag, 7);
}

Result appletTakeScreenShotOfOwnLayer(bool flag, AppletCaptureSharedBuffer captureBuf) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
        s32 captureBuf;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDisplayController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->flag = flag!=0;
    raw->captureBuf = captureBuf;

    Result rc = serviceIpcDispatch(&g_appletIDisplayController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIDisplayController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletCopyBetweenCaptureBuffers(AppletCaptureSharedBuffer dstCaptureBuf, AppletCaptureSharedBuffer srcCaptureBuf) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 dstCaptureBuf;
        s32 srcCaptureBuf;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDisplayController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->dstCaptureBuf = dstCaptureBuf;
    raw->srcCaptureBuf = srcCaptureBuf;

    Result rc = serviceIpcDispatch(&g_appletIDisplayController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIDisplayController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletClearCaptureBuffer(bool flag, AppletCaptureSharedBuffer captureBuf, u32 color) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
        s32 captureBuf;
        u32 color;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDisplayController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;
    raw->flag = flag!=0;
    raw->captureBuf = captureBuf;
    raw->color = color;

    Result rc = serviceIpcDispatch(&g_appletIDisplayController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIDisplayController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletClearAppletTransitionBuffer(u32 color) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletIDisplayController, color, 21);
}

static Result _appletAcquireCaptureSharedBuffer(bool *flag, s32 *id, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDisplayController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_appletIDisplayController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 flag;
            s32 id;
        } *resp;

        serviceIpcParse(&g_appletIDisplayController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && flag) *flag = resp->flag!=0;
        if (R_SUCCEEDED(rc) && id) *id = resp->id;
    }

    return rc;
}

Result appletAcquireLastApplicationCaptureSharedBuffer(bool *flag, s32 *id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletAcquireCaptureSharedBuffer(flag, id, 22);
}

Result appletReleaseLastApplicationCaptureSharedBuffer(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletIDisplayController, 23);
}

Result appletAcquireLastForegroundCaptureSharedBuffer(bool *flag, s32 *id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletAcquireCaptureSharedBuffer(flag, id, 24);
}

Result appletReleaseLastForegroundCaptureSharedBuffer(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletIDisplayController, 25);
}

Result appletAcquireCallerAppletCaptureSharedBuffer(bool *flag, s32 *id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletAcquireCaptureSharedBuffer(flag, id, 26);
}

Result appletReleaseCallerAppletCaptureSharedBuffer(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletIDisplayController, 27);
}

Result appletTakeScreenShotOfOwnLayerEx(bool flag0, bool immediately, AppletCaptureSharedBuffer captureBuf) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag0;
        u8 immediately;
        s32 captureBuf;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDisplayController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 28;
    raw->flag0 = flag0!=0;
    raw->immediately = immediately!=0;
    raw->captureBuf = captureBuf;

    Result rc = serviceIpcDispatch(&g_appletIDisplayController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIDisplayController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

// IProcessWindingController

static Result _appletGetLaunchReason(AppletProcessLaunchReason *reason) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIProcessWindingController, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_appletIProcessWindingController);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            AppletProcessLaunchReason reason;
        } *resp;

        serviceIpcParse(&g_appletIProcessWindingController, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && reason) *reason = resp->reason;
    }

    return rc;
}

static Result _appletOpenCallingLibraryApplet(AppletHolder *h) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletOpenExistingLibraryApplet(h, &g_appletIProcessWindingController, 11);
}

Result appletPushContext(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInStorage(&g_appletIProcessWindingController, s, 21);
}

Result appletPopContext(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutStorage(&g_appletIProcessWindingController, s, 22);
}

static Result _appletWindAndDoReserved(void) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIProcessWindingController, 30);
}

static Result _appletReserveToStartAndWaitAndUnwindThis(AppletHolder *h) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInSession(&g_appletIProcessWindingController, &h->s, 40);
}

static Result _appletReserveToStartAndWait(AppletHolder *h) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInSession(&g_appletIProcessWindingController, &h->s, 41);
}

// LockAccessor
void appletLockAccessorClose(AppletLockAccessor *a) {
    eventClose(&a->event);
    serviceClose(&a->s);
}

static Result _appletLockAccessorTryLock(AppletLockAccessor *a, bool get_handle, Handle* handle_out, bool *outflag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 inflag;
    } *raw;

    raw = serviceIpcPrepareHeader(&a->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->inflag = get_handle!=0;

    Result rc = serviceIpcDispatch(&a->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 outflag;
        } *resp;

        serviceIpcParse(&a->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (handle_out) *handle_out = r.Handles[0];
            if (outflag) *outflag = resp->outflag!=0;
        }
    }

    return rc;
}

Result appletLockAccessorTryLock(AppletLockAccessor *a, bool *flag) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = eventWait(&a->event, 0);
    if (R_SUCCEEDED(rc)) rc = _appletLockAccessorTryLock(a, false, NULL, flag);
    return rc;
}

Result appletLockAccessorLock(AppletLockAccessor *a) {
    Result rc=0;
    bool flag=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    do {
        rc = eventWait(&a->event, U64_MAX);
        if (R_SUCCEEDED(rc)) rc = _appletLockAccessorTryLock(a, false, NULL, &flag);
        if (R_FAILED(rc)) break;
    } while(!flag);

    return rc;
}

Result appletLockAccessorUnlock(AppletLockAccessor *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&a->s, 2);
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

static Result _appletHolderCreateState(AppletHolder *h, LibAppletMode mode, bool creating_self) {
    Result rc=0;

    h->mode = mode;
    h->creating_self = creating_self;

    if (R_SUCCEEDED(rc)) rc = _appletGetEvent(&h->s, &h->StateChangedEvent, 0, false);//GetAppletStateChangedEvent

    if (R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0) && h->mode == LibAppletMode_BackgroundIndirect) rc = _appletGetIndirectLayerConsumerHandle(&h->s, &h->layer_handle);

    return rc;
}

static Result _appletHolderCreate(AppletHolder *h, AppletId id, LibAppletMode mode, bool creating_self) {
    Result rc=0;

    memset(h, 0, sizeof(AppletHolder));

    if (!h->creating_self) rc = _appletWaitLibraryAppletLaunchableEvent();

    if (R_SUCCEEDED(rc)) rc = _appletCreateLibraryApplet(&h->s, id, mode);

    if (R_SUCCEEDED(rc)) rc = _appletHolderCreateState(h, mode, creating_self);

    return rc;
}

static Result _appletOpenExistingLibraryApplet(AppletHolder *h, Service* srv, u64 cmd_id) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc=0;
    LibAppletInfo info={0};

    memset(h, 0, sizeof(AppletHolder));

    rc = _appletGetSession(srv, &h->s, cmd_id);

    if (R_SUCCEEDED(rc)) rc = appletHolderGetLibraryAppletInfo(h, &info);

    if (R_SUCCEEDED(rc)) rc = _appletHolderCreateState(h, info.mode, false);

    if (R_FAILED(rc)) appletHolderClose(h);

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
    if (h->mode!=LibAppletMode_BackgroundIndirect)
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

Result appletHolderJump(AppletHolder *h) {
    Result rc=0;

    if (!serviceIsActive(&h->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (!h->creating_self) rc = _appletWaitLibraryAppletLaunchableEvent();

    if (hosversionBefore(4,0,0))
        rc = _appletReserveToStartAndWaitAndUnwindThis(h);
    else
        rc = _appletReserveToStartAndWait(h);

    if (R_SUCCEEDED(rc)) rc = _appletWindAndDoReserved();

    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();

    return rc;
}

Result appletHolderRequestExit(AppletHolder *h) {
    Result rc=0;

    if (!serviceIsActive(&h->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (!appletHolderCheckFinished(h)) rc = _appletCmdNoIO(&h->s, 20);//RequestExit

    return rc;
}

static Result _appletAccessorRequestExitOrTerminate(Service* srv, u64 timeout) {
    Result rc=0;
    Event StateChangedEvent={0};

    if (!serviceIsActive(srv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletGetEvent(srv, &StateChangedEvent, 0, false);//GetAppletStateChangedEvent

    if (R_SUCCEEDED(rc)) rc = _appletCmdNoIO(srv, 20);//RequestExit

    if (R_SUCCEEDED(rc)) {
        rc = eventWait(&StateChangedEvent, timeout);

        if (R_FAILED(rc) && R_VALUE(rc) == KERNELRESULT(TimedOut))
            rc = _appletCmdNoIO(srv, 25);//Terminate
    }

    eventClose(&StateChangedEvent);

    return rc;
}

Result appletHolderRequestExitOrTerminate(AppletHolder *h, u64 timeout) {
    return _appletAccessorRequestExitOrTerminate(&h->s, timeout);
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

Result appletHolderSetOutOfFocusApplicationSuspendingEnabled(AppletHolder *h, bool flag) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInBool(&h->s, flag, 50);
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

Result appletHolderGetLibraryAppletInfo(AppletHolder *h, LibAppletInfo *info) {
    return _appletGetLibraryAppletInfo(&h->s, info, 120);
}

// (ILibraryAppletCreator ->) IStorage

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

// IApplicationFunctions

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

static Result _appletCreateApplicationAndRequestToStart(u64 titleID) { // [4.0.0+]
    return _appletCmdInU64(&g_appletIFunctions, titleID, 12);
}

static Result _appletCreateApplicationAndRequestToStartForQuest(u64 titleID, const AppletApplicationAttributeForQuest *attr) { // [4.0.0+]
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

static Result _appletCreateApplicationWithAttributeAndPushAndRequestToStartForQuest(u64 titleID, AppletStorage* s, const AppletApplicationAttribute *attr) { // [7.0.0+]
    IpcCommand c;
    ipcInitialize(&c);

    serviceSendObject(&s->s, &c);

    ipcAddSendBuffer(&c, attr, sizeof(*attr), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
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

static Result _appletCreateApplicationWithAttributeAndRequestToStartForQuest(u64 titleID, const AppletApplicationAttribute *attr) { // [7.0.0+]
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, attr, sizeof(*attr), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
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
    AppletApplicationAttribute appattr={0};
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

    if (hosversionAtLeast(7,0,0)) {
        appattr.unk_x0 = attr->unk_x0;
        appattr.unk_x4 = attr->unk_x4;
        appattr.volume = attr->volume;
    }

    if (hosversionAtLeast(4,0,0) && s==NULL) {
        if (hosversionAtLeast(7,0,0))
            rc = _appletCreateApplicationWithAttributeAndRequestToStartForQuest(titleID, &appattr);
        else
            rc = _appletCreateApplicationAndRequestToStartForQuest(titleID, attr);
    }
    else {
        if (hosversionBefore(3,0,0)) rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
        if (R_SUCCEEDED(rc)) {
            if (hosversionAtLeast(7,0,0))
                rc = _appletCreateApplicationWithAttributeAndPushAndRequestToStartForQuest(titleID, s, &appattr);
            else
                rc = _appletCreateApplicationAndPushAndRequestToStartForQuest(titleID, s, attr);
        }
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

Result appletGetDisplayVersion(char *displayVersion) {
    IpcCommand c;
    ipcInitialize(&c);

    if (displayVersion) memset(displayVersion, 0, 0x10);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            char displayVersion[0x10];
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && displayVersion) {
            strncpy(displayVersion, resp->displayVersion, 0x10);
            displayVersion[0xf] = 0;
        }
    }

    return rc;
}

Result appletBeginBlockingHomeButtonShortAndLongPressed(s64 val) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInU64(&g_appletIFunctions, val, 30);
}

Result appletEndBlockingHomeButtonShortAndLongPressed(void) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIFunctions, 31);
}

Result appletBeginBlockingHomeButton(s64 val) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInU64(&g_appletIFunctions, val, 32);
}

Result appletEndBlockingHomeButton(void) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIFunctions, 33);
}

void appletNotifyRunning(bool *out) {
    if (__nx_applet_type!=AppletType_Application || g_appletNotifiedRunning) return;
    g_appletNotifiedRunning = 1;

    Result rc = _appletCmdNoInOutBool(&g_appletIFunctions, out, 40);

    if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletNotifyRunning));
}

Result appletGetPseudoDeviceId(u128 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 50;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u128 out;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->out;
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

Result appletRequestToShutdown(void) {
    Result rc=0;

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 70);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

Result appletRequestToReboot(void) {
    Result rc=0;

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 71);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

Result appletExitAndRequestToShowThanksMessage(void) {
    Result rc=0;

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 80);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

static Result _appletInitializeApplicationCopyrightFrameBuffer(TransferMemory *tmem, s32 width, s32 height) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 width;
        s32 height;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 100;
    raw->width = width;
    raw->height = height;
    raw->size = tmem->size;

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

Result appletInitializeApplicationCopyrightFrameBuffer(void) {
    Result rc=0;
    s32 width = 1280;
    s32 height = 720;
    size_t size = 0x3C0000;

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (g_appletCopyrightInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    rc = tmemCreate(&g_appletCopyrightTmem, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = _appletInitializeApplicationCopyrightFrameBuffer(&g_appletCopyrightTmem, width, height);
    if (R_FAILED(rc)) {
        tmemClose(&g_appletCopyrightTmem);
        return rc;
    }

    g_appletCopyrightInitialized = 1;

    return rc;
}

Result appletSetApplicationCopyrightImage(const void* buffer, size_t size, s32 x, s32 y, s32 width, s32 height, s32 mode) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buffer, size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 x;
        s32 y;
        s32 width;
        s32 height;
        s32 mode;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 101;
    raw->x = x;
    raw->y = y;
    raw->width = width;
    raw->height = height;
    raw->mode = mode;

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

Result appletSetApplicationCopyrightVisibility(bool visible) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletIFunctions, visible, 102);
}

//Official sw has these under 'pdm'.
Result appletQueryApplicationPlayStatistics(PdmApplicationPlayStatistics *stats, const u64 *titleIDs, s32 count, s32 *total_out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    ipcAddSendBuffer(&c, titleIDs, count*sizeof(u64), BufferType_Normal);
    ipcAddRecvBuffer(&c, stats, count*sizeof(PdmApplicationPlayStatistics), BufferType_Normal);

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
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result appletQueryApplicationPlayStatisticsByUid(u128 userID, PdmApplicationPlayStatistics *stats, const u64 *titleIDs, s32 count, s32 *total_out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    ipcAddSendBuffer(&c, titleIDs, count*sizeof(u64), BufferType_Normal);
    ipcAddRecvBuffer(&c, stats, count*sizeof(PdmApplicationPlayStatistics), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u128 userID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 111;
    raw->userID = userID;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result appletGetGpuErrorDetectedSystemEvent(Event *out_event) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetEvent(&g_appletIFunctions, out_event, 130, false);
}

// IHomeMenuFunctions

Result appletRequestToGetForeground(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIFunctions, 10);
}

Result appletLockForeground(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIFunctions, 11);
}

Result appletUnlockForeground(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIFunctions, 12);
}

Result appletPopFromGeneralChannel(AppletStorage *s) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutStorage(&g_appletIFunctions, s, 20);
}

Result appletGetPopFromGeneralChannelEvent(Event *out_event) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetEvent(&g_appletIFunctions, out_event, 21, false);
}

Result appletGetHomeButtonWriterLockAccessor(AppletLockAccessor *a) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetHomeButtonRwLockAccessor(&g_appletIFunctions, a, 30);
}

Result appletPopRequestLaunchApplicationForDebug(u128 *userIDs, s32 count, u64 *titleID, s32 *total_out) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, userIDs, count*sizeof(u128), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 100;

    Result rc = serviceIpcDispatch(&g_appletIFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 titleID;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_appletIFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && titleID) *titleID = resp->titleID;
        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result appletLaunchDevMenu(void) {
    Result rc=0;

    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 200);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

// IGlobalStateController

Result appletStartSleepSequence(bool flag) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInBool(&g_appletIGlobalStateController, flag, 2);
}

Result appletStartShutdownSequence(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIGlobalStateController, 3);
}

Result appletStartRebootSequence(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIGlobalStateController, 4);
}

Result appletIsAutoPowerDownRequested(bool *out) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletIGlobalStateController, out, 9);
}

Result appletLoadAndApplyIdlePolicySettings(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletIGlobalStateController, 10);
}

Result appletNotifyCecSettingsChanged(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletIGlobalStateController, 11);
}

Result appletSetDefaultHomeButtonLongPressTime(s64 val) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU64(&g_appletIGlobalStateController, val, 12);
}

Result appletUpdateDefaultDisplayResolution(void) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletIGlobalStateController, 13);
}

Result appletShouldSleepOnBoot(bool *out) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletIGlobalStateController, out, 14);
}

Result appletGetHdcpAuthenticationFailedEvent(Event *out_event) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetEvent(&g_appletIGlobalStateController, out_event, 15, false);
}

// IApplicationCreator

static Result _appletApplicationCreateState(AppletApplication *a) {
    Result rc=0;

    rc = _appletGetEvent(&a->s, &a->StateChangedEvent, 0, false);//GetAppletStateChangedEvent

    return rc;
}

static Result _appletApplicationCreate(AppletApplication *a, Service* srv, u64 cmd_id) {
    Result rc=0;

    memset(a, 0, sizeof(AppletApplication));

    rc = _appletGetSession(srv, &a->s, cmd_id);

    if (R_SUCCEEDED(rc)) rc = _appletApplicationCreateState(a);

    return rc;
}

static Result _appletApplicationCreateIn64(AppletApplication *a, Service* srv, u64 cmd_id, u64 val) {
    Result rc=0;

    memset(a, 0, sizeof(AppletApplication));

    rc = _appletGetSessionIn64(srv, &a->s, cmd_id, val);

    if (R_SUCCEEDED(rc)) rc = _appletApplicationCreateState(a);

    return rc;
}

Result appletCreateApplication(AppletApplication *a, u64 titleID) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletApplicationCreateIn64(a, &g_appletIApplicationCreator, 0, titleID);
}

Result appletPopLaunchRequestedApplication(AppletApplication *a) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletApplicationCreate(a, &g_appletIApplicationCreator, 1);
}

Result appletCreateSystemApplication(AppletApplication *a, u64 titleID) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletApplicationCreateIn64(a, &g_appletIApplicationCreator, 10, titleID);
}

Result appletPopFloatingApplicationForDevelopment(AppletApplication *a) {
    if (__nx_applet_type != AppletType_SystemApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletApplicationCreate(a, &g_appletIApplicationCreator, 100);
}

void appletApplicationClose(AppletApplication *a) {
    eventClose(&a->StateChangedEvent);
    serviceClose(&a->s);
    memset(a, 0, sizeof(AppletApplication));
}

bool appletApplicationActive(AppletApplication *a) {
    return serviceIsActive(&a->s);
}

Result appletApplicationStart(AppletApplication *a) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdNoIO(&a->s, 10);//Start

    return rc;
}

Result appletApplicationRequestExit(AppletApplication *a) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdNoIO(&a->s, 20);//RequestExit

    return rc;
}

Result appletApplicationTerminate(AppletApplication *a) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdNoIO(&a->s, 25);//Terminate

    return rc;
}

void appletApplicationJoin(AppletApplication *a) {
    Result rc=0;
    AppletApplicationExitReason res = AppletApplicationExitReason_Normal;
    u32 desc=0;

    eventWait(&a->StateChangedEvent, U64_MAX);
    rc = _appletCmdNoIO(&a->s, 30);//GetResult

    if (R_FAILED(rc)) {
        res = AppletApplicationExitReason_Unexpected;

        if (R_MODULE(rc) == 128) {
            desc = R_DESCRIPTION(rc);

            if (desc >= 35 && desc < 40) res = AppletApplicationExitReason_Unknown5;
            else if (desc >= 31 && desc < 40) res = AppletApplicationExitReason_Unknown1;
            else if (desc == 23) res = AppletApplicationExitReason_Unknown2;
            else if (desc >= 40 && desc < 45) res = AppletApplicationExitReason_Unknown3;
            else if (desc == 51) res = AppletApplicationExitReason_Unknown4;
        }
    }

    a->exitreason = res;
}

bool appletApplicationCheckFinished(AppletApplication *a) {
    return R_SUCCEEDED(eventWait(&a->StateChangedEvent, 0));
}

AppletApplicationExitReason appletApplicationGetExitReason(AppletApplication *a) {
    return a->exitreason;
}

Result appletApplicationRequestForApplicationToGetForeground(AppletApplication *a) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdNoIO(&a->s, 101);//RequestForApplicationToGetForeground

    return rc;
}

Result appletApplicationRequestExitLibraryAppletOrTerminate(AppletApplication *a, u64 timeout) {
    Result rc=0;
    Service srv={0};

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletGetSession(&a->s, &srv, 112);//GetCurrentLibraryApplet
    if (R_SUCCEEDED(rc)) rc = _appletAccessorRequestExitOrTerminate(&srv, timeout);
    serviceClose(&srv);

    return rc;
}

Result appletApplicationGetApplicationId(AppletApplication *a, u64 *titleID) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoInOut64(&a->s, titleID, 120);//GetApplicationId

    return rc;
}

Result appletApplicationPushLaunchParameter(AppletApplication *a, AppletLaunchParameterKind kind, AppletStorage* s) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    serviceSendObject(&s->s, &c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 kind;
    } *raw;

    raw = serviceIpcPrepareHeader(&a->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 121;
    raw->kind = kind;

    Result rc = serviceIpcDispatch(&a->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&a->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    appletStorageClose(s);

    return rc;
}

Result appletApplicationGetApplicationControlProperty(AppletApplication *a, NacpStruct *nacp) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInRecvBuf(&a->s, nacp, sizeof(*nacp), 122);//GetApplicationControlProperty
}

Result appletApplicationGetApplicationLaunchProperty(AppletApplication *a, AppletApplicationLaunchProperty *out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInRecvBuf(&a->s, out, sizeof(*out), 123);//GetApplicationLaunchProperty
}

Result appletApplicationGetApplicationLaunchRequestInfo(AppletApplication *a, AppletApplicationLaunchRequestInfo *out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&a->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 124;

    Result rc = serviceIpcDispatch(&a->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            AppletApplicationLaunchRequestInfo out;
        } *resp;

        serviceIpcParse(&a->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result appletApplicationSetUsers(AppletApplication *a, const u128 *userIDs, s32 count, bool flag) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, userIDs, count*sizeof(u128), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = serviceIpcPrepareHeader(&a->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 130;
    raw->flag = flag!=0;

    Result rc = serviceIpcDispatch(&a->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&a->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletApplicationCheckRightsEnvironmentAvailable(AppletApplication *a, bool *out) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoInOutBool(&a->s, out, 131);//CheckRightsEnvironmentAvailable

    return rc;
}

Result appletApplicationGetNsRightsEnvironmentHandle(AppletApplication *a, u64 *handle) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoInOut64(&a->s, handle, 132);//GetNsRightsEnvironmentHandle

    return rc;
}

Result appletApplicationGetDesirableUids(AppletApplication *a, u128 *userIDs, s32 count, s32 *total_out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, userIDs, count*sizeof(u128), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&a->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 140;

    Result rc = serviceIpcDispatch(&a->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_out;
        } *resp;

        serviceIpcParse(&a->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result appletApplicationReportApplicationExitTimeout(AppletApplication *a) {
    Result rc=0;

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&a->s, 150);//ReportApplicationExitTimeout

    return rc;
}

Result appletApplicationSetApplicationAttribute(AppletApplication *a, const AppletApplicationAttribute *attr) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdSendBufNoOut(&a->s, attr, sizeof(*attr), 160);//SetApplicationAttribute
}

Result appletApplicationHasSaveDataAccessPermission(AppletApplication *a, u64 titleID, bool *out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
    } *raw;

    raw = serviceIpcPrepareHeader(&a->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 170;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&a->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&a->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out!=0;
    }

    return rc;
}

// ILibraryAppletSelfAccessor

Result appletPopInData(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutStorage(&g_appletILibraryAppletSelfAccessor, s, 0);
}

Result appletPushOutData(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInStorage(&g_appletILibraryAppletSelfAccessor, s, 1);
}

Result appletPopInteractiveInData(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutStorage(&g_appletILibraryAppletSelfAccessor, s, 2);
}

Result appletPushInteractiveOutData(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInStorage(&g_appletILibraryAppletSelfAccessor, s, 3);
}

Result appletGetPopInDataEvent(Event *out_event) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetEvent(&g_appletILibraryAppletSelfAccessor, out_event, 5, false);
}

Result appletGetPopInteractiveInDataEvent(Event *out_event) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetEvent(&g_appletILibraryAppletSelfAccessor, out_event, 6, false);
}

static Result _appletExitProcessAndReturn(void) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoIO(&g_appletILibraryAppletSelfAccessor, 10);
}

Result appletGetLibraryAppletInfo(LibAppletInfo *info) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetLibraryAppletInfo(&g_appletILibraryAppletSelfAccessor, info, 11);
}

Result appletGetMainAppletIdentityInfo(AppletIdentityInfo *info) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetIdentityInfo(&g_appletILibraryAppletSelfAccessor, info, 12);
}

Result appletCanUseApplicationCore(bool *out) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutBool(&g_appletILibraryAppletSelfAccessor, out, 13);
}

Result appletGetCallerAppletIdentityInfo(AppletIdentityInfo *info) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetIdentityInfo(&g_appletILibraryAppletSelfAccessor, info, 14);
}

Result appletGetMainAppletApplicationControlProperty(NacpStruct *nacp) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInRecvBuf(&g_appletILibraryAppletSelfAccessor, nacp, sizeof(*nacp), 15);
}

Result appletGetMainAppletStorageId(FsStorageId *storageId) {
    u8 tmp=0;
    Result rc=0;

    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoInOutU8(&g_appletILibraryAppletSelfAccessor, &tmp, 16);
    if (R_SUCCEEDED(rc) && storageId) *storageId = tmp;
    return rc;
}

Result appletGetCallerAppletIdentityInfoStack(AppletIdentityInfo *stack, s32 count, s32 *total_out) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, stack, count*sizeof(AppletIdentityInfo), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletILibraryAppletSelfAccessor, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 17;

    Result rc = serviceIpcDispatch(&g_appletILibraryAppletSelfAccessor);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_appletILibraryAppletSelfAccessor, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result appletGetNextReturnDestinationAppletIdentityInfo(AppletIdentityInfo *info) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletGetIdentityInfo(&g_appletILibraryAppletSelfAccessor, info, 18);
}

Result appletGetDesirableKeyboardLayout(u32 *layout) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut32(&g_appletILibraryAppletSelfAccessor, layout, 19);
}

Result appletPopExtraStorage(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOutStorage(&g_appletILibraryAppletSelfAccessor, s, 20);
}

Result appletGetPopExtraStorageEvent(Event *out_event) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletGetEvent(&g_appletILibraryAppletSelfAccessor, out_event, 25, false);
}

Result appletUnpopInData(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInStorage(&g_appletILibraryAppletSelfAccessor, s, 30);
}

Result appletUnpopExtraStorage(AppletStorage *s) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInStorage(&g_appletILibraryAppletSelfAccessor, s, 31);
}

Result appletGetIndirectLayerProducerHandle(u64 *out) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut64(&g_appletILibraryAppletSelfAccessor, out, 40);
}

Result appletGetMainAppletApplicationDesiredLanguage(u64 *LanguageCode) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut64(&g_appletILibraryAppletSelfAccessor, LanguageCode, 60);
}

Result appletGetCurrentApplicationId(u64 *titleID) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOut64(&g_appletILibraryAppletSelfAccessor, titleID, 70);
}

Result appletRequestExitToSelf(void) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletILibraryAppletSelfAccessor, 80);
}

Result appletGetMainAppletAvailableUsers(u128 *userIDs, s32 count, bool *flag, s32 *total_out) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, userIDs, count*sizeof(u128), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletILibraryAppletSelfAccessor, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 110;

    Result rc = serviceIpcDispatch(&g_appletILibraryAppletSelfAccessor);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 flag;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_appletILibraryAppletSelfAccessor, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && flag) *flag = resp->flag!=0;
        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
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

Result appletGetApplicationIdForLogo(u64 *titleID) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdNoInOut64(&g_appletIFunctions, titleID, 2);
}

Result appletSetGpuTimeSliceBoost(u64 val) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInU64(&g_appletIFunctions, val, 3);
}

Result appletSetAutoSleepTimeAndDimmingTimeEnabled(bool flag) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletIFunctions, flag, 4);
}

Result appletTerminateApplicationAndSetReason(Result reason) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInU32(&g_appletIFunctions, reason, 5);
}

Result appletSetScreenShotPermissionGlobally(bool flag) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletIFunctions, flag, 6);
}

Result appletStartShutdownSequenceForOverlay(void) {
    Result rc=0;

    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 10);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

Result appletStartRebootSequenceForOverlay(void) {
    Result rc=0;

    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 11);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

Result appletSetHandlingHomeButtonShortPressedEnabled(bool flag) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletIFunctions, flag, 20);
}

Result appletBeginToObserveHidInputForDevelop(void) {
    if (__nx_applet_type != AppletType_OverlayApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoIO(&g_appletIFunctions, 101);
}

// IAppletCommonFunctions

Result appletReadThemeStorage(void* buffer, size_t size, u64 offset, size_t *transfer_size) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvSmart(&c, g_appletIAppletCommonFunctions_ptrbufsize, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIAppletCommonFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->offset = offset;

    Result rc = serviceIpcDispatch(&g_appletIAppletCommonFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 transfer_size;
        } *resp;

        serviceIpcParse(&g_appletIAppletCommonFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && transfer_size) *transfer_size = resp->transfer_size;
    }

    return rc;
}

Result appletWriteThemeStorage(const void* buffer, size_t size, u64 offset) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendSmart(&c, g_appletIAppletCommonFunctions_ptrbufsize, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIAppletCommonFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->offset = offset;

    Result rc = serviceIpcDispatch(&g_appletIAppletCommonFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIAppletCommonFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletGetDisplayLogicalResolution(s32 *width, s32 *height) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

     return _appletGetResolution(&g_appletIAppletCommonFunctions, width, height, 40);
}

Result appletSetDisplayMagnification(float x, float y, float width, float height) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        float x;
        float y;
        float width;
        float height;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIAppletCommonFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 42;
    raw->x = x;
    raw->y = y;
    raw->width = width;
    raw->height = height;

    Result rc = serviceIpcDispatch(&g_appletIAppletCommonFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIAppletCommonFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletSetHomeButtonDoubleClickEnabled(bool flag) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBool(&g_appletIAppletCommonFunctions, flag, 50);
}

Result appletGetHomeButtonDoubleClickEnabled(bool *out) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdNoInOutBool(&g_appletIAppletCommonFunctions, out, 51);
}

// IDebugFunctions

Result appletOpenMainApplication(AppletApplication *a) {
    return _appletApplicationCreate(a, &g_appletIDebugFunctions, 1);
}

Result appletPerformSystemButtonPressing(AppletSystemButtonType type) {
    return _appletCmdInU32(&g_appletIDebugFunctions, type, 10);
}

Result appletInvalidateTransitionLayer(void) {
    return _appletCmdNoIO(&g_appletIDebugFunctions, 20);
}

Result appletRequestLaunchApplicationWithUserAndArgumentForDebug(u64 titleID, u128 *userIDs, size_t total_userIDs, bool flag, const void* buffer, size_t size) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, userIDs, total_userIDs*sizeof(u128), BufferType_Normal);
    ipcAddSendBuffer(&c, buffer, size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
        u64 titleID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDebugFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 30;
    raw->flag = flag!=0;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_appletIDebugFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_appletIDebugFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result appletGetAppletResourceUsageInfo(AppletResourceUsageInfo *info) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_appletIDebugFunctions, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 40;

    Result rc = serviceIpcDispatch(&g_appletIDebugFunctions);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            AppletResourceUsageInfo info;
        } *resp;

        serviceIpcParse(&g_appletIDebugFunctions, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && info) *info = resp->info;
    }

    return rc;
}

// State / other

u8 appletGetOperationMode(void) {
    return g_appletOperationMode;
}

u32 appletGetPerformanceMode(void) {
    return g_appletPerformanceMode;
}

AppletFocusState appletGetFocusState(void) {
    return (AppletFocusState)g_appletFocusState;
}

AppletInfo *appletGetAppletInfo(void) {
    if (!g_appletInfoInitialized) return NULL;
    return &g_appletInfo;
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
        case AppletMessage_ExitRequested:
            appletCallHook(AppletHookType_OnExitRequest);
            return false;
        break;

        case AppletMessage_FocusStateChanged:
            rc = _appletGetCurrentFocusState(&g_appletFocusState);
            if (R_FAILED(rc))
                fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetCurrentFocusState));

            appletCallHook(AppletHookType_OnFocusState);
        break;

        case AppletMessage_Restart:
            appletCallHook(AppletHookType_OnRestart);
        break;

        case AppletMessage_OperationModeChanged:
            rc = _appletGetOperationMode(&g_appletOperationMode);
            if (R_FAILED(rc))
                fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetOperationMode));

            appletCallHook(AppletHookType_OnOperationMode);
        break;

        case AppletMessage_PerformanceModeChanged:
            rc = _appletGetPerformanceMode(&g_appletPerformanceMode);
            if (R_FAILED(rc))
                fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetPerformanceMode));

            appletCallHook(AppletHookType_OnPerformanceMode);
        break;

        case AppletMessage_RequestToDisplay:
            appletCallHook(AppletHookType_RequestToDisplay);
        break;

        case AppletMessage_CaptureButtonShortPressed:
            appletCallHook(AppletHookType_OnCaptureButtonShortPressed);
        break;

        case AppletMessage_AlbumImageTaken:
            appletCallHook(AppletHookType_OnAlbumImageTaken);
        break;
    }

    return true;
}

bool appletMainLoop(void) {
    u32 msg = 0;

    if (R_FAILED(appletGetMessage(&msg))) return true;

    return appletProcessMessage(msg);
}
