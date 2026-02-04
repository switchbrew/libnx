#include <string.h>
#include "service_guard.h"
#include "arm/counter.h"
#include "services/applet.h"
#include "runtime/env.h"
#include "runtime/hosversion.h"
#include "runtime/diag.h"

__attribute__((weak)) u32 __nx_applet_type = AppletType_Default;
__attribute__((weak)) bool __nx_applet_auto_notifyrunning = true;
__attribute__((weak)) AppletAttribute __nx_applet_AppletAttribute;
/// When set, controls the PerformanceConfiguration passed to apmSetPerformanceConfiguration during app startup, where the array index is the PerformanceMode.
__attribute__((weak)) u32 __nx_applet_PerformanceConfiguration[2] = {/*0x92220008*//*0x20004*//*0x92220007*/0, 0};
//// Controls whether to use applet exit cmds during \ref appletExit.  0 (default): Only run exit cmds when running under a NSO. 1: Use exit cmds regardless. >1: Skip exit cmds.
__attribute__((weak)) u32 __nx_applet_exit_mode = 0;
/// Controls the timeout in nanoseconds to use during LibraryApplet initialization in appletInitialize, with diagAbortWithResult being used if the timeout is reached.
__attribute__((weak)) u64 __nx_applet_init_timeout = 864000000000000000ULL;

static Service g_appletSrv;
static Service g_appletProxySession;
static bool g_appletLeftInitialized;
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

static ApmCpuBoostMode g_appletCpuBoostMode = ApmCpuBoostMode_Normal;

static AppletInfo g_appletInfo;
static bool g_appletInfoInitialized;

static Result _appletCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id);
static Result _appletCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id);
static Result _appletCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id);

static Result _appletOpenLibraryAppletProxy(Service* srv_out, Handle prochandle, const AppletAttribute *attr);
static Result _appletGetSessionProxy(Service* srv_out, Handle prochandle, u32 cmd_id);

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
static Result _appletOpenExistingLibraryApplet(AppletHolder *h, Service* srv, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(applet);

Result _appletInitialize(void) {
    AppletAttribute *attr = NULL;

    if (g_appletLeftInitialized) {
        g_appletLeftInitialized = false;
        return 0;
    }

    if (__nx_applet_type == AppletType_None)
        return 0;

    if (__nx_applet_type == AppletType_Default || __nx_applet_type == AppletType_Application) {
        if (R_FAILED(apmInitialize()))
            return MAKERESULT(Module_Libnx, LibnxError_ApmFailedToInitialize);
    }

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

        u64 start_tick = 0;
        u64 timeout = __nx_applet_init_timeout;
        if (__nx_applet_type == AppletType_LibraryApplet)
            start_tick = armGetSystemTick();

        do {
            u32 cmd_id;

            switch(__nx_applet_type) {
                case AppletType_Application:       cmd_id = 0;   break;
                case AppletType_SystemApplet:      cmd_id = 100; break;
                case AppletType_LibraryApplet:     cmd_id = 200; break;
                case AppletType_OverlayApplet:     cmd_id = 300; break;
                case AppletType_SystemApplication: cmd_id = 350; break;
                // TODO: Replace error code
                default: diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_AppletCmdidNotFound));
            }

            if (__nx_applet_type == AppletType_LibraryApplet && hosversionAtLeast(3,0,0)) {
                cmd_id = 201;
                attr = &__nx_applet_AppletAttribute;
            }

            if (attr==NULL)
                rc = _appletGetSessionProxy(&g_appletProxySession, CUR_PROCESS_HANDLE, cmd_id);
            else
                rc = _appletOpenLibraryAppletProxy(&g_appletProxySession, CUR_PROCESS_HANDLE, attr);

            if (R_VALUE(rc) == AM_BUSY_ERROR) {
                svcSleepThread(100000000);

                if (__nx_applet_type == AppletType_LibraryApplet && armTicksToNs(armGetSystemTick() - start_tick) >= timeout)
                    diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_Timeout));

                // Official sw also has code for calling a funcptr on the first AM_BUSY_ERROR (instead of sleep) where the elapsed time since start_tick is >=1 second, but we don't impl that.
            }

        } while (R_VALUE(rc) == AM_BUSY_ERROR);
    }

    // [7.0.0+] GetAppletCommonFunctions
    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) {
        if (__nx_applet_type == AppletType_SystemApplet || __nx_applet_type == AppletType_LibraryApplet || __nx_applet_type == AppletType_OverlayApplet) {
            rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIAppletCommonFunctions, __nx_applet_type == AppletType_SystemApplet ? 23 : 21);
        }
    }

    // Get*Functions
    if (R_SUCCEEDED(rc) && __nx_applet_type != AppletType_LibraryApplet)
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIFunctions, 20);

    if (R_SUCCEEDED(rc) && __nx_applet_type == AppletType_SystemApplet) {
        //GetGlobalStateController
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIGlobalStateController, 21);

        //GetApplicationCreator
        if (R_SUCCEEDED(rc))
            rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIApplicationCreator, 22);
    }

    if (R_SUCCEEDED(rc) && __nx_applet_type == AppletType_LibraryApplet) {
        //GetLibraryAppletSelfAccessor
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletILibraryAppletSelfAccessor, 20);

        //GetProcessWindingController
        if (R_SUCCEEDED(rc))
            rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIProcessWindingController, 10);
    }

    if (R_SUCCEEDED(rc) && hosversionAtLeast(15,0,0)) { // [15.0.0+]
        // GetHomeMenuFunctions
        if (__nx_applet_type == AppletType_LibraryApplet)
            rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIFunctions, 22);

        // GetGlobalStateController
        if (R_SUCCEEDED(rc) && (__nx_applet_type == AppletType_LibraryApplet || __nx_applet_type == AppletType_OverlayApplet))
            rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIGlobalStateController, 23);
    }

    // GetLibraryAppletCreator
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletILibraryAppletCreator, 11);
    // GetCommonStateGetter
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletICommonStateGetter, 0);
    // GetSelfController
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletISelfController, 1);
    // GetWindowController
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIWindowController, 2);
    // Get AppletResourceUserId.
    if (R_SUCCEEDED(rc))
        rc = _appletGetAppletResourceUserId(&g_appletResourceUserId);
    // GetAudioController
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIAudioController, 3);
    // GetDisplayController
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIDisplayController, 4);
    // GetDebugFunctions
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetSession(&g_appletProxySession, &g_appletIDebugFunctions, 1000);

    Result rc2 = _appletGetAccumulatedSuspendedTickChangedEvent(&g_appletSuspendedTickEvent);
    if (R_SUCCEEDED(rc2)) {
        g_appletInitTickBase = armGetSystemTick();
        g_appletSuspendedTick = 0;
        g_appletSuspendedTickInitialized = true;
    }

    // ICommonStateGetter::GetEventHandle
    if (R_SUCCEEDED(rc))
        rc = _appletCmdGetEvent(&g_appletICommonStateGetter, &g_appletMessageEvent, false, 0);

    if (R_SUCCEEDED(rc) && (__nx_applet_type == AppletType_Application))
    {
        rc = _appletGetCurrentFocusState(&g_appletFocusState);

        //Don't enter this msg-loop when g_appletFocusState is already 1, it will hang when applet was previously initialized in the context of the current process for AppletType_Application.
        if (R_SUCCEEDED(rc) && g_appletFocusState != AppletFocusState_InFocus) {
            do {
                eventWait(&g_appletMessageEvent, UINT64_MAX);

                u32 msg;
                rc = _appletReceiveMessage(&msg);

                if (R_FAILED(rc))
                {
                    if (R_VALUE(rc) == 0x680)
                        continue;

                    break;
                }

                if (msg != AppletMessage_FocusStateChanged)
                    continue;

                rc = _appletGetCurrentFocusState(&g_appletFocusState);

                if (R_FAILED(rc))
                    break;

            } while(g_appletFocusState != AppletFocusState_InFocus);
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
    if (R_SUCCEEDED(rc) && __nx_applet_type == AppletType_Application) {
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

    return rc;
}

static void _appletInfiniteSleepLoop(void) {
    while(1) svcSleepThread(86400000000000ULL);
}

static void NX_NORETURN _appletExitProcess(int result_code) {
    appletInitialize();
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

void _appletCleanup(void) {
    if (!g_appletExitProcessFlag) {
        if (g_appletRecordingInitialized > 0) {
            if (g_appletRecordingInitialized == 2) appletSetGamePlayRecordingState(0);
        }

        if (__nx_applet_type == AppletType_Application) appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);

        if (g_appletCpuBoostMode != ApmCpuBoostMode_Normal) appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
    }

    if ((envIsNso() && __nx_applet_exit_mode==0) || __nx_applet_exit_mode==1) {
        if (_appletIsApplication() ||
            __nx_applet_type == AppletType_LibraryApplet) {
            if (!g_appletExitProcessFlag) {
                g_appletExitProcessFlag = 1;
                g_appletLeftInitialized = true;
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

    if (__nx_applet_type == AppletType_SystemApplet)
        serviceClose(&g_appletIApplicationCreator);

    if (__nx_applet_type == AppletType_LibraryApplet) {
        serviceClose(&g_appletIProcessWindingController);
        serviceClose(&g_appletILibraryAppletSelfAccessor);
    }

    if (__nx_applet_type == AppletType_SystemApplet || __nx_applet_type == AppletType_LibraryApplet || __nx_applet_type == AppletType_OverlayApplet)
        serviceClose(&g_appletIGlobalStateController);

    serviceClose(&g_appletIFunctions);

    serviceClose(&g_appletIAppletCommonFunctions);

    serviceClose(&g_appletProxySession);
    serviceClose(&g_appletSrv);
    g_appletResourceUserId = 0;

    if (g_appletRecordingInitialized > 0) {
        tmemClose(&g_appletRecordingTmem);
        g_appletRecordingInitialized = 0;
    }

    if (g_appletCopyrightInitialized) {
        tmemClose(&g_appletCopyrightTmem);
        g_appletCopyrightInitialized = 0;
    }

    if (_appletIsRegularApplication()) apmExit();
}

Service* appletGetServiceSession_Proxy(void) {
    return &g_appletProxySession;
}

Service* appletGetServiceSession_AppletCommonFunctions(void) {
    return &g_appletIAppletCommonFunctions;
}

Service* appletGetServiceSession_Functions(void) {
    return &g_appletIFunctions;
}

Service* appletGetServiceSession_GlobalStateController(void) {
    return &g_appletIGlobalStateController;
}

Service* appletGetServiceSession_ApplicationCreator(void) {
    return &g_appletIApplicationCreator;
}

Service* appletGetServiceSession_LibraryAppletSelfAccessor(void) {
    return &g_appletILibraryAppletSelfAccessor;
}

Service* appletGetServiceSession_ProcessWindingController(void) {
    return &g_appletIProcessWindingController;
}

Service* appletGetServiceSession_LibraryAppletCreator(void) {
    return &g_appletILibraryAppletCreator;
}

Service* appletGetServiceSession_CommonStateGetter(void) {
    return &g_appletICommonStateGetter;
}

Service* appletGetServiceSession_SelfController(void) {
    return &g_appletISelfController;
}

Service* appletGetServiceSession_WindowController(void) {
    return &g_appletIWindowController;
}

Service* appletGetServiceSession_AudioController(void) {
    return &g_appletIAudioController;
}

Service* appletGetServiceSession_DisplayController(void) {
    return &g_appletIDisplayController;
}

Service* appletGetServiceSession_DebugFunctions(void) {
    return &g_appletIDebugFunctions;
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

    if (__nx_applet_type != AppletType_Application)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
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

// Helper macros for use with the below ipc helper funcs.
#define IPC_MAKE_CMD_IMPL(proto,_s,_rid,func,...) \
proto { \
    if (!serviceIsActive((_s))) \
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized); \
    return func((_s), ##__VA_ARGS__, (_rid)); \
}

#define IPC_MAKE_CMD_IMPL_HOSVER(proto,_s,_rid,func,_hosver,...) \
proto { \
    if (!serviceIsActive((_s))) \
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized); \
    if (hosversionBefore _hosver) \
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer); \
    return func((_s), ##__VA_ARGS__, (_rid)); \
}

#define IPC_MAKE_CMD_IMPL_INITEXPR(proto,_s,_rid,func,initexpr,...) \
proto { \
    if (!serviceIsActive((_s)) || (initexpr)) \
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized); \
    return func((_s), ##__VA_ARGS__, (_rid)); \
}

#define IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(proto,_s,_rid,func,initexpr,_hosver,...) \
proto { \
    if (!serviceIsActive((_s)) || (initexpr)) \
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized); \
    if (hosversionBefore _hosver) \
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer); \
    return func((_s), ##__VA_ARGS__, (_rid)); \
}

static Result _appletCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _appletCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _appletCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _appletCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletOpenLibraryAppletProxy(Service* srv_out, Handle prochandle, const AppletAttribute *attr) {
    u64 reserved=0;
    serviceAssumeDomain(&g_appletSrv);
    return serviceDispatchIn(&g_appletSrv, 201, reserved,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { attr, sizeof(*attr) } },
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { prochandle },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletGetSessionProxy(Service* srv_out, Handle prochandle, u32 cmd_id) {
    u64 reserved=0;
    serviceAssumeDomain(&g_appletSrv);
    return serviceDispatchIn(&g_appletSrv, cmd_id, reserved,
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { prochandle },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletCmdGetSessionInU64(Service* srv, Service* srv_out, u64 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletCmdGetSessionInU32(Service* srv, Service* srv_out, u32 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _appletCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _appletCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _appletCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _appletCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _appletCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _appletCmdInU8NoOut(Service* srv, u8 inval, u64 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _appletCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _appletCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _appletCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    return _appletCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _appletCmdInHandleU64NoOut(Service* srv, Handle handle, u64 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval,
        .in_num_handles = 1,
        .in_handles = { handle },
    );
}

static Result _appletCmdInHandleU64OutSession(Service* srv, Service* srv_out, Handle handle, u64 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval,
        .in_num_handles = 1,
        .in_handles = { handle },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletCmdInTmemNoOut(Service* srv, TransferMemory *tmem, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return _appletCmdInHandleU64NoOut(srv, tmem->handle, tmem->size, cmd_id);
}

static Result _appletCmdInTmemOutSession(Service* srv, Service* srv_out, TransferMemory *tmem, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return _appletCmdInHandleU64OutSession(srv, srv_out, tmem->handle, tmem->size, cmd_id);
}

static Result _appletCmdInSession(Service* srv, Service* srv_in, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .in_num_objects = 1,
        .in_objects = { srv_in },
    );
}

static Result _appletCmdInStorage(Service* srv, AppletStorage* s, u32 cmd_id) {
    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc =_appletCmdInSession(srv, &s->s, cmd_id);
    appletStorageClose(s);
    return rc;
}

static Result _appletCmdInStorageU32(Service* srv, AppletStorage* s, u32 inval, u32 cmd_id) {
    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(srv);
    Result rc = serviceDispatchIn(srv, cmd_id, inval,
        .in_num_objects = 1,
        .in_objects = { &s->s },
    );
    appletStorageClose(s);
    return rc;
}

static Result _appletCmdInStorageU64(Service* srv, AppletStorage* s, u64 inval, u32 cmd_id) {
    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(srv);
    Result rc = serviceDispatchIn(srv, cmd_id, inval,
        .in_num_objects = 1,
        .in_objects = { &s->s },
    );
    appletStorageClose(s);
    return rc;
}

static Result _appletCmdInU32OutStorage(Service* srv, AppletStorage* s, u32 inval, u32 cmd_id) {
    memset(s, 0, sizeof(AppletStorage));
    serviceAssumeDomain(srv);
    Result rc = serviceDispatchIn(srv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
    return rc;
}

static Result _appletCmdNoInOutStorage(Service* srv, AppletStorage* s, u32 cmd_id) {
    memset(s, 0, sizeof(AppletStorage));
    return _appletCmdGetSession(srv, &s->s, cmd_id);
}

static Result _appletCmdSendBufNoOut(Service* srv, const void* buffer, size_t size, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _appletCmdNoInRecvBuf(Service* srv, void* buffer, size_t size, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static Result _appletGetLibraryAppletInfo(Service* srv, LibAppletInfo *info, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *info);
}

static Result _appletGetIdentityInfo(Service* srv, AppletIdentityInfo *info, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *info);
}

static Result _appletGetResolution(Service* srv, s32 *width, s32 *height, u32 cmd_id) {
    struct {
        s32 width;
        s32 height;
    } out;

    serviceAssumeDomain(srv);
    Result rc = serviceDispatchOut(srv, cmd_id, out);
    if (R_SUCCEEDED(rc) && width) *width = out.width;
    if (R_SUCCEEDED(rc) && height) *height = out.height;
    return rc;
}

// ICommonStateGetter

IPC_MAKE_CMD_IMPL(static Result _appletReceiveMessage(u32 *out),     &g_appletICommonStateGetter, 1, _appletCmdNoInOutU32, out)
IPC_MAKE_CMD_IMPL(static Result _appletGetOperationMode(u8 *out),    &g_appletICommonStateGetter, 5, _appletCmdNoInOutU8,  out)
IPC_MAKE_CMD_IMPL(static Result _appletGetPerformanceMode(u32 *out), &g_appletICommonStateGetter, 6, _appletCmdNoInOutU32, out)
IPC_MAKE_CMD_IMPL(       Result appletGetCradleStatus(u8 *status),   &g_appletICommonStateGetter, 7, _appletCmdNoInOutU8,  status)

Result appletGetBootMode(PmBootMode *mode) {
    u8 tmp=0;
    Result rc = _appletCmdNoInOutU8(&g_appletICommonStateGetter, &tmp, 8);
    if (R_SUCCEEDED(rc) && mode) *mode = tmp;
    return rc;
}

IPC_MAKE_CMD_IMPL(static Result _appletGetCurrentFocusState(u8 *out),               &g_appletICommonStateGetter,  9, _appletCmdNoInOutU8, out)
IPC_MAKE_CMD_IMPL(static Result _appletGetAcquiredSleepLockEvent(Event *out_event), &g_appletICommonStateGetter, 13, _appletCmdGetEvent,  out_event, false)

static Result _appletWaitAcquiredSleepLockEvent(void) {
    Result rc=0;
    Event tmpevent={0};

    rc = _appletGetAcquiredSleepLockEvent(&tmpevent);
    if (R_SUCCEEDED(rc)) rc = eventWait(&tmpevent, UINT64_MAX);
    eventClose(&tmpevent);
    return rc;
}

Result appletRequestToAcquireSleepLock(void) {
    Result rc = _appletCmdNoIO(&g_appletICommonStateGetter, 10);
    if (R_SUCCEEDED(rc)) rc = _appletWaitAcquiredSleepLockEvent();
    return rc;
}

IPC_MAKE_CMD_IMPL(Result appletReleaseSleepLock(void), &g_appletICommonStateGetter, 11, _appletCmdNoIO)

Result appletReleaseSleepLockTransiently(void) {
    Result rc = _appletCmdNoIO(&g_appletICommonStateGetter, 12);
    if (R_SUCCEEDED(rc)) rc = _appletWaitAcquiredSleepLockEvent();
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetWakeupCount(u64 *out),        &g_appletICommonStateGetter, 14, _appletCmdNoInOutU64, (11,0,0), out)

IPC_MAKE_CMD_IMPL(Result appletPushToGeneralChannel(AppletStorage *s), &g_appletICommonStateGetter, 20, _appletCmdInStorage, s)

static Result _appletGetHomeButtonRwLockAccessor(Service* srv, AppletLockAccessor *a, u32 cmd_id) {
    Result rc = _appletCmdGetSession(srv, &a->s, cmd_id);
    if (R_FAILED(rc))
        return rc;

    rc = _appletCmdGetEvent(&a->s, &a->event, false, 3);
    if (R_FAILED(rc)) {
        serviceAssumeDomain(&a->s);
        serviceClose(&a->s);
    }
    return rc;
}

IPC_MAKE_CMD_IMPL(Result appletGetHomeButtonReaderLockAccessor(AppletLockAccessor *a), &g_appletICommonStateGetter, 30, _appletGetHomeButtonRwLockAccessor, a)

static Result _appletGetRwLockAccessor(Service* srv, AppletLockAccessor *a, s32 inval, u32 cmd_id) {
    Result rc = _appletCmdGetSessionInU32(srv, &a->s, inval, cmd_id);
    if (R_FAILED(rc))
        return rc;

    rc = _appletCmdGetEvent(&a->s, &a->event, false, 3);
    if (R_FAILED(rc)) {
        serviceAssumeDomain(&a->s);
        serviceClose(&a->s);
    }
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetReaderLockAccessorEx(AppletLockAccessor *a, u32 inval), &g_appletICommonStateGetter, 31, _appletGetRwLockAccessor, (2,0,0), a, inval)

Result appletGetWriterLockAccessorEx(AppletLockAccessor *a, u32 inval) {
    if (hosversionBefore(7,0,0)) {
        if (__nx_applet_type == AppletType_SystemApplet && hosversionAtLeast(2,0,0))
            return _appletGetRwLockAccessor(&g_appletIFunctions, a, inval, 31);

        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    return _appletGetRwLockAccessor(&g_appletICommonStateGetter, a, inval, 32);
}

Result appletGetCradleFwVersion(u32 *out0, u32 *out1, u32 *out2, u32 *out3) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 out0;
        u32 out1;
        u32 out2;
        u32 out3;
    } out;

    serviceAssumeDomain(&g_appletICommonStateGetter);
    Result rc = serviceDispatchOut(&g_appletICommonStateGetter, 40, out);
    if (R_SUCCEEDED(rc)) {
        if (out0) *out0 = out.out0;
        if (out1) *out1 = out.out1;
        if (out2) *out2 = out.out2;
        if (out3) *out3 = out.out3;
    }
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsVrModeEnabled(bool *out), &g_appletICommonStateGetter, 50, _appletCmdNoInOutBool, (3,0,0), out)

Result appletSetVrModeEnabled(bool flag) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (hosversionBefore(7,0,0))
        return _appletCmdInBoolNoOut(&g_appletICommonStateGetter, flag, 51);

    return _appletCmdNoIO(&g_appletICommonStateGetter, flag ? 53 : 54);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetLcdBacklightOffEnabled(bool flag),                             &g_appletICommonStateGetter, 52, _appletCmdInBoolNoOut, (4,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsInControllerFirmwareUpdateSection(bool *out),                   &g_appletICommonStateGetter, 55, _appletCmdNoInOutBool, (3,0,0), out)

Result appletSetVrPositionForDebug(s32 x, s32 y, s32 width, s32 height) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 x;
        s32 y;
        s32 width;
        s32 height;
    } in = { x, y, width, height };

    serviceAssumeDomain(&g_appletICommonStateGetter);
    return serviceDispatchIn(&g_appletICommonStateGetter, 59, in);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetDefaultDisplayResolution(s32 *width, s32 *height),             &g_appletICommonStateGetter, 60, _appletGetResolution,  (3,0,0), width, height)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetDefaultDisplayResolutionChangeEvent(Event *out_event),         &g_appletICommonStateGetter, 61, _appletCmdGetEvent,    (3,0,0), out_event, true)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetHdcpAuthenticationState(s32 *state),                           &g_appletICommonStateGetter, 62, _appletCmdNoInOutU32,  (4,0,0), (u32*)state)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetHdcpAuthenticationStateChangeEvent(Event *out_event),          &g_appletICommonStateGetter, 63, _appletCmdGetEvent,    (4,0,0), out_event, true)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetTvPowerStateMatchingMode(AppletTvPowerStateMatchingMode mode), &g_appletICommonStateGetter, 64, _appletCmdInU32NoOut,  (5,0,0), mode)

Result appletGetApplicationIdByContentActionName(u64 *application_id, const char *name) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletICommonStateGetter);
    return serviceDispatchOut(&g_appletICommonStateGetter, 65, *application_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { name, strlen(name)+1 } },
    );
}

Result appletSetCpuBoostMode(ApmCpuBoostMode mode) {
    Result rc=0;
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdInU32NoOut(&g_appletICommonStateGetter, mode, 66);
    if (R_SUCCEEDED(rc)) g_appletCpuBoostMode = mode;
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletCancelCpuBoostMode(void),                                          &g_appletICommonStateGetter, 67,  _appletCmdNoIO,        (10,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetBuiltInDisplayType(s32 *out),                                   &g_appletICommonStateGetter, 68,  _appletCmdNoInOutU32,  (11,0,0), (u32*)out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletPerformSystemButtonPressingIfInFocus(AppletSystemButtonType type), &g_appletICommonStateGetter, 80,  _appletCmdInU32NoOut,  (6,0,0), type)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetPerformanceConfigurationChangedNotification(bool flag),         &g_appletICommonStateGetter, 90,  _appletCmdInBoolNoOut, (7,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetCurrentPerformanceConfiguration(u32 *PerformanceConfiguration), &g_appletICommonStateGetter, 91,  _appletCmdNoInOutU32,  (7,0,0), PerformanceConfiguration)

Result appletOpenMyGpuErrorHandler(AppletGpuErrorHandler *g) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdGetSession(&g_appletICommonStateGetter, &g->s, 110);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetOperationModeSystemInfo(u32 *info),                             &g_appletICommonStateGetter, 200, _appletCmdNoInOutU32,  (7,0,0), info)

Result appletGetSettingsPlatformRegion(SetSysPlatformRegion *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp=0;
    Result rc = _appletCmdNoInOutU8(&g_appletICommonStateGetter, &tmp, 300);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletActivateMigrationService(void),                                    &g_appletICommonStateGetter, 400,  _appletCmdNoIO,        (10,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletDeactivateMigrationService(void),                                  &g_appletICommonStateGetter, 401,  _appletCmdNoIO,        (10,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletDisableSleepTillShutdown(void),                                    &g_appletICommonStateGetter, 500,  _appletCmdNoIO,        (11,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSuppressDisablingSleepTemporarily(u64 val),                        &g_appletICommonStateGetter, 501,  _appletCmdInU64NoOut,  (11,0,0), val)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetRequestExitToLibraryAppletAtExecuteNextProgramEnabled(void),    &g_appletICommonStateGetter, 900,  _appletCmdNoIO,        (11,0,0))

// IGpuErrorHandler
void appletGpuErrorHandlerClose(AppletGpuErrorHandler *g) {
    serviceAssumeDomain(&g->s);
    serviceClose(&g->s);
}

IPC_MAKE_CMD_IMPL(Result appletGpuErrorHandlerGetManualGpuErrorInfoSize(AppletGpuErrorHandler *g, u64 *out),                     &g->s, 100, _appletCmdNoInOutU64, out)

Result appletGpuErrorHandlerGetManualGpuErrorInfo(AppletGpuErrorHandler *g, void* buffer, size_t size, u64 *out) {
    if (!serviceIsActive(&g->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g->s);
    return serviceDispatchOut(&g->s, 101, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

IPC_MAKE_CMD_IMPL(Result appletGpuErrorHandlerGetManualGpuErrorDetectionSystemEvent(AppletGpuErrorHandler *g, Event *out_event), &g->s, 102,  _appletCmdGetEvent,   out_event, false)
IPC_MAKE_CMD_IMPL(Result appletGpuErrorHandlerFinishManualGpuErrorHandling(AppletGpuErrorHandler *g),                            &g->s, 103, _appletCmdNoIO)

// ISelfController

IPC_MAKE_CMD_IMPL(static Result _appletSelfExit(void),         &g_appletISelfController, 0, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(       Result appletLockExit(void),          &g_appletISelfController, 1, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(       Result appletUnlockExit(void),        &g_appletISelfController, 2, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(       Result appletEnterFatalSection(void), &g_appletISelfController, 3, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(       Result appletLeaveFatalSection(void), &g_appletISelfController, 4, _appletCmdNoIO)

static Result _appletWaitLibraryAppletLaunchableEvent(void) {
    Result rc=0;

    if (!eventActive(&g_appletLibraryAppletLaunchableEvent))
        rc = _appletCmdGetEvent(&g_appletISelfController, &g_appletLibraryAppletLaunchableEvent, false, 9);

    if (R_SUCCEEDED(rc)) rc = eventWait(&g_appletLibraryAppletLaunchableEvent, UINT64_MAX);

    return rc;
}

IPC_MAKE_CMD_IMPL(       Result appletSetScreenShotPermission(AppletScreenShotPermission permission), &g_appletISelfController, 10, _appletCmdInU32NoOut,  permission)
IPC_MAKE_CMD_IMPL(static Result _appletSetOperationModeChangedNotification(bool flag),                &g_appletISelfController, 11, _appletCmdInBoolNoOut, flag)
IPC_MAKE_CMD_IMPL(static Result _appletSetPerformanceModeChangedNotification(bool flag),              &g_appletISelfController, 12, _appletCmdInBoolNoOut, flag)

static Result _appletSetFocusHandlingMode(bool inval0, bool inval1, bool inval2) {
    const struct {
        u8 inval0;
        u8 inval1;
        u8 inval2;
    } in = { inval0!=0, inval1!=0, inval2!=0 };

    serviceAssumeDomain(&g_appletISelfController);
    return serviceDispatchIn(&g_appletISelfController, 13, in);
}

IPC_MAKE_CMD_IMPL(Result appletSetRestartMessageEnabled(bool flag), &g_appletISelfController, 14, _appletCmdInBoolNoOut, flag)

Result appletSetScreenShotAppletIdentityInfo(AppletIdentityInfo *info) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletISelfController);
    return serviceDispatchIn(&g_appletISelfController, 15, *info);
}

IPC_MAKE_CMD_IMPL(static Result _appletSetOutOfFocusSuspendingEnabled(bool flag),                  &g_appletISelfController, 16, _appletCmdInBoolNoOut,          flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetControllerFirmwareUpdateSection(bool flag),               &g_appletISelfController, 17, _appletCmdInBoolNoOut, (3,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetRequiresCaptureButtonShortPressedMessage(bool flag),      &g_appletISelfController, 18, _appletCmdInBoolNoOut, (3,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetAlbumImageOrientation(AlbumImageOrientation orientation), &g_appletISelfController, 19, _appletCmdInU32NoOut,  (3,0,0), orientation)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetDesirableKeyboardLayout(SetKeyboardLayout layout),        &g_appletISelfController, 20, _appletCmdInU32NoOut,  (4,0,0), layout)
IPC_MAKE_CMD_IMPL(       Result appletCreateManagedDisplayLayer(u64 *out),                         &g_appletISelfController, 40, _appletCmdNoInOutU64,           out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsSystemBufferSharingEnabled(void),                          &g_appletISelfController, 41, _appletCmdNoIO,        (4,0,0))

Result appletGetSystemSharedLayerHandle(u64 *SharedBufferHandle, u64 *SharedLayerHandle) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 SharedBufferHandle;
        u64 SharedLayerHandle;
    } out;

    serviceAssumeDomain(&g_appletISelfController);
    Result rc = serviceDispatchOut(&g_appletISelfController, 42, out);
    if (R_SUCCEEDED(rc) && SharedBufferHandle) *SharedBufferHandle = out.SharedBufferHandle;
    if (R_SUCCEEDED(rc) && SharedLayerHandle) *SharedLayerHandle = out.SharedLayerHandle;
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetSystemSharedBufferHandle(u64 *SharedBufferHandle) , &g_appletISelfController, 43, _appletCmdNoInOutU64,  (5,0,0), SharedBufferHandle)

Result appletCreateManagedDisplaySeparableLayer(u64 *display_layer, u64 *recording_layer) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 display_layer;
        u64 recording_layer;
    } out;

    serviceAssumeDomain(&g_appletISelfController);
    Result rc = serviceDispatchOut(&g_appletISelfController, 44, out);
    if (R_SUCCEEDED(rc) && display_layer) *display_layer = out.display_layer;
    if (R_SUCCEEDED(rc) && recording_layer) *recording_layer = out.recording_layer;
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetManagedDisplayLayerSeparationMode(u32 mode),        &g_appletISelfController, 45, _appletCmdInU32NoOut, (10,0,0), mode)

IPC_MAKE_CMD_IMPL(       Result appletSetHandlesRequestToDisplay(bool flag),                 &g_appletISelfController, 50, _appletCmdInBoolNoOut,          flag)
IPC_MAKE_CMD_IMPL(       Result appletApproveToDisplay(void),                                &g_appletISelfController, 51, _appletCmdNoIO)

Result appletOverrideAutoSleepTimeAndDimmingTime(s32 inval0, s32 inval1, s32 inval2, s32 inval3) {
    const struct {
        s32 inval0;
        s32 inval1;
        s32 inval2;
        s32 inval3;
    } in = { inval0, inval1, inval2, inval3 };

    serviceAssumeDomain(&g_appletISelfController);
    return serviceDispatchIn(&g_appletISelfController, 60, in);
}

IPC_MAKE_CMD_IMPL(       Result appletSetIdleTimeDetectionExtension(AppletIdleTimeDetectionExtension ext),  &g_appletISelfController, 62, _appletCmdInU32NoOut,         ext)
IPC_MAKE_CMD_IMPL(       Result appletGetIdleTimeDetectionExtension(AppletIdleTimeDetectionExtension *ext), &g_appletISelfController, 63, _appletCmdNoInOutU32,         ext)
IPC_MAKE_CMD_IMPL(       Result appletSetInputDetectionSourceSet(u32 val),                                  &g_appletISelfController, 64, _appletCmdInU32NoOut,         val)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletReportUserIsActive(void),                                             &g_appletISelfController, 65, _appletCmdNoIO,       (2,0,0))

Result appletGetCurrentIlluminance(float *fLux) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletISelfController);
    return serviceDispatchOut(&g_appletISelfController, 66, *fLux);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsIlluminanceAvailable(bool *out), &g_appletISelfController, 67, _appletCmdNoInOutBool, (3,0,0), out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetAutoSleepDisabled(bool flag),   &g_appletISelfController, 68, _appletCmdInBoolNoOut, (5,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsAutoSleepDisabled(bool *out),    &g_appletISelfController, 69, _appletCmdNoInOutBool, (5,0,0), out)

Result appletGetCurrentIlluminanceEx(bool *bOverLimit, float *fLux) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u8 bOverLimit;
        float fLux;
    } out;

    serviceAssumeDomain(&g_appletISelfController);
    Result rc = serviceDispatchOut(&g_appletISelfController, 71, out);
    if (R_SUCCEEDED(rc) && bOverLimit) *bOverLimit = out.bOverLimit & 1;
    if (R_SUCCEEDED(rc) && fLux) *fLux = out.fLux;
    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(       Result appletSetInputDetectionPolicy(AppletInputDetectionPolicy policy), &g_appletISelfController, 72,  _appletCmdInU32NoOut, (9,0,0), policy)
IPC_MAKE_CMD_IMPL_HOSVER(       Result appletSetWirelessPriorityMode(AppletWirelessPriorityMode mode),   &g_appletISelfController, 80,  _appletCmdInU32NoOut, (4,0,0), mode)
IPC_MAKE_CMD_IMPL_HOSVER(static Result _appletGetAccumulatedSuspendedTickValue(u64 *tick),               &g_appletISelfController, 90,  _appletCmdNoInOutU64, (6,0,0), tick)
IPC_MAKE_CMD_IMPL_HOSVER(static Result _appletGetAccumulatedSuspendedTickChangedEvent(Event *out_event), &g_appletISelfController, 91,  _appletCmdGetEvent,   (6,0,0), out_event, true)

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

IPC_MAKE_CMD_IMPL_HOSVER(       Result appletSetAlbumImageTakenNotificationEnabled(bool flag),           &g_appletISelfController, 100, _appletCmdInBoolNoOut, (7,0,0), flag)

Result appletSetApplicationAlbumUserData(const void* buffer, size_t size) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletISelfController);
    return serviceDispatch(&g_appletISelfController, 110,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

IPC_MAKE_CMD_IMPL_HOSVER(       Result appletSaveCurrentScreenshot(AlbumReportOption option),            &g_appletISelfController, 120,  _appletCmdInU32NoOut, (11,0,0), option)

// IWindowController

IPC_MAKE_CMD_IMPL(static Result _appletGetAppletResourceUserId(u64 *out), &g_appletIWindowController, 1, _appletCmdNoInOutU64, out)

u64 appletGetAppletResourceUserId(void) {
    return g_appletResourceUserId;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetAppletResourceUserIdOfCallerApplet(u64 *out), &g_appletIWindowController, 2,  _appletCmdNoInOutU64,  (6,0,0), out)
IPC_MAKE_CMD_IMPL(static Result _appletAcquireForegroundRights(void),                  &g_appletIWindowController, 10, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetAppletWindowVisibility(bool flag),            &g_appletIWindowController, 20, _appletCmdInBoolNoOut, (7,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetAppletGpuTimeSlice(s64 val),                  &g_appletIWindowController, 21, _appletCmdInU64NoOut,  (7,0,0), val)

// IAudioController

Result appletSetExpectedMasterVolume(float mainAppletVolume, float libraryAppletVolume) {
    const struct {
        float mainAppletVolume;
        float libraryAppletVolume;
    } in = { mainAppletVolume, libraryAppletVolume };

    serviceAssumeDomain(&g_appletIAudioController);
    return serviceDispatchIn(&g_appletIAudioController, 0, in);
}

Result appletGetExpectedMasterVolume(float *mainAppletVolume, float *libraryAppletVolume) {
    Result rc=0;

    if (mainAppletVolume) rc = _appletCmdNoInOutU32(&g_appletIAudioController, (u32*)mainAppletVolume, 1); // GetMainAppletExpectedMasterVolume
    if (R_SUCCEEDED(rc) && libraryAppletVolume) rc = _appletCmdNoInOutU32(&g_appletIAudioController, (u32*)libraryAppletVolume, 2); // GetLibraryAppletExpectedMasterVolume

    return rc;
}

Result appletChangeMainAppletMasterVolume(float volume, u64 unk) {
    const struct {
        float volume;
        u64 unk;
    } in = { volume, unk };

    serviceAssumeDomain(&g_appletIAudioController);
    return serviceDispatchIn(&g_appletIAudioController, 3, in);
}

Result appletSetTransparentVolumeRate(float val) {
    serviceAssumeDomain(&g_appletIAudioController);
    return serviceDispatchIn(&g_appletIAudioController, 4, val);
}

// IDisplayController

IPC_MAKE_CMD_IMPL(Result appletUpdateLastForegroundCaptureImage(void), &g_appletIDisplayController, 1, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(Result appletUpdateCallerAppletCaptureImage(void),   &g_appletIDisplayController, 4, _appletCmdNoIO)

static Result _appletGetCaptureImageEx(void* buffer, size_t size, bool *flag, u32 cmd_id) {
    u8 tmp=0;
    serviceAssumeDomain(&g_appletIDisplayController);
    Result rc = serviceDispatchOut(&g_appletIDisplayController, cmd_id, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
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

    const struct {
        u8 flag;
        s32 captureBuf;
    } in = { flag!=0, captureBuf };

    serviceAssumeDomain(&g_appletIDisplayController);
    return serviceDispatchIn(&g_appletIDisplayController, 8, in);
}

Result appletCopyBetweenCaptureBuffers(AppletCaptureSharedBuffer dstCaptureBuf, AppletCaptureSharedBuffer srcCaptureBuf) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 dstCaptureBuf;
        s32 srcCaptureBuf;
    } in = { dstCaptureBuf, srcCaptureBuf };

    serviceAssumeDomain(&g_appletIDisplayController);
    return serviceDispatchIn(&g_appletIDisplayController, 9, in);
}

Result appletClearCaptureBuffer(bool flag, AppletCaptureSharedBuffer captureBuf, u32 color) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 flag;
        u8 pad[3];
        s32 captureBuf;
        u32 color;
    } in = { flag!=0, {0}, captureBuf, color };

    serviceAssumeDomain(&g_appletIDisplayController);
    return serviceDispatchIn(&g_appletIDisplayController, 20, in);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletClearAppletTransitionBuffer(u32 color), &g_appletIDisplayController, 21, _appletCmdInU32NoOut, (3,0,0), color)

static Result _appletAcquireCaptureSharedBuffer(bool *flag, s32 *id, u64 cmd_id) {
    struct {
        u8 flag;
        s32 id;
    } out;

    serviceAssumeDomain(&g_appletIDisplayController);
    Result rc = serviceDispatchOut(&g_appletIDisplayController, cmd_id, out);
    if (R_SUCCEEDED(rc) && flag) *flag = out.flag & 1;
    if (R_SUCCEEDED(rc) && id) *id = out.id;
    return rc;
}

Result appletAcquireLastApplicationCaptureSharedBuffer(bool *flag, s32 *id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletAcquireCaptureSharedBuffer(flag, id, 22);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletReleaseLastApplicationCaptureSharedBuffer(void), &g_appletIDisplayController, 23, _appletCmdNoIO, (4,0,0))

Result appletAcquireLastForegroundCaptureSharedBuffer(bool *flag, s32 *id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletAcquireCaptureSharedBuffer(flag, id, 24);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletReleaseLastForegroundCaptureSharedBuffer(void), &g_appletIDisplayController, 25, _appletCmdNoIO, (4,0,0))

Result appletAcquireCallerAppletCaptureSharedBuffer(bool *flag, s32 *id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletAcquireCaptureSharedBuffer(flag, id, 26);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletReleaseCallerAppletCaptureSharedBuffer(void), &g_appletIDisplayController, 27, _appletCmdNoIO, (4,0,0))

Result appletTakeScreenShotOfOwnLayerEx(bool flag0, bool immediately, AppletCaptureSharedBuffer captureBuf) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 flag0;
        u8 immediately;
        s32 captureBuf;
    } in = { flag0!=0, immediately!=0, captureBuf };

    serviceAssumeDomain(&g_appletIDisplayController);
    return serviceDispatchIn(&g_appletIDisplayController, 28, in);
}

// IProcessWindingController

static Result _appletGetLaunchReason(AppletProcessLaunchReason *reason) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_appletIProcessWindingController);
    return serviceDispatchOut(&g_appletIProcessWindingController, 0, *reason);
}

static Result _appletOpenCallingLibraryApplet(AppletHolder *h) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletOpenExistingLibraryApplet(h, &g_appletIProcessWindingController, 11);
}

IPC_MAKE_CMD_IMPL_INITEXPR(              Result appletPushContext(AppletStorage *s),                        &g_appletIProcessWindingController, 21, _appletCmdInStorage,      __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(              Result appletPopContext(AppletStorage *s),                         &g_appletIProcessWindingController, 22, _appletCmdNoInOutStorage, __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       static Result _appletWindAndDoReserved(void),                             &g_appletIProcessWindingController, 30, _appletCmdNoIO,           __nx_applet_type != AppletType_LibraryApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(       static Result _appletReserveToStartAndWaitAndUnwindThis(AppletHolder *h), &g_appletIProcessWindingController, 40, _appletCmdInSession,      __nx_applet_type != AppletType_LibraryApplet,          &h->s)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(static Result _appletReserveToStartAndWait(AppletHolder *h),              &g_appletIProcessWindingController, 41, _appletCmdInSession,      __nx_applet_type != AppletType_LibraryApplet, (4,0,0), &h->s)

// LockAccessor
void appletLockAccessorClose(AppletLockAccessor *a) {
    eventClose(&a->event);
    serviceAssumeDomain(&a->s);
    serviceClose(&a->s);
}

static Result _appletLockAccessorTryLock(AppletLockAccessor *a, bool get_handle, Handle* handle_out, bool *outflag) {
    Handle tmphandle = INVALID_HANDLE;
    u8 tmp = get_handle!=0;
    u8 tmpout=0;
    serviceAssumeDomain(&a->s);
    Result rc = serviceDispatchInOut(&a->s, 1, tmp, tmpout,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmphandle,
    );
    if (R_SUCCEEDED(rc) && outflag) *outflag = tmpout & 1;
    if (R_SUCCEEDED(rc) && handle_out) *handle_out = tmphandle;
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
        rc = eventWait(&a->event, UINT64_MAX);
        if (R_SUCCEEDED(rc)) rc = _appletLockAccessorTryLock(a, false, NULL, &flag);
        if (R_FAILED(rc)) break;
    } while(!flag);

    return rc;
}

IPC_MAKE_CMD_IMPL(Result appletLockAccessorUnlock(AppletLockAccessor *a), &a->s, 2, _appletCmdNoIO)

// ILibraryAppletCreator

static Result _appletCreateLibraryApplet(Service* srv_out, AppletId id, LibAppletMode mode) {
    const struct {
        u32 id;
        u32 mode;
    } in = { id, mode };

    serviceAssumeDomain(&g_appletILibraryAppletCreator);
    return serviceDispatchIn(&g_appletILibraryAppletCreator, 0, in,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletGetIndirectLayerConsumerHandle(Service* srv, u64 *out) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    serviceAssumeDomain(srv);
    return serviceDispatchInOut(srv, 160, AppletResourceUserId, *out,
        .in_send_pid = true,
    );
}

static Result _appletHolderCreateState(AppletHolder *h, LibAppletMode mode, bool creating_self) {
    Result rc=0;

    h->mode = mode;
    h->creating_self = creating_self;

    if (R_SUCCEEDED(rc)) rc = _appletCmdGetEvent(&h->s, &h->StateChangedEvent, false, 0);//GetAppletStateChangedEvent

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

static Result _appletOpenExistingLibraryApplet(AppletHolder *h, Service* srv, u32 cmd_id) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc=0;
    LibAppletInfo info={0};

    memset(h, 0, sizeof(AppletHolder));

    rc = _appletCmdGetSession(srv, &h->s, cmd_id);

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

IPC_MAKE_CMD_IMPL(Result appletTerminateAllLibraryApplets(void),    &g_appletILibraryAppletCreator, 1, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(Result appletAreAnyLibraryAppletsLeft(bool *out), &g_appletILibraryAppletCreator, 2, _appletCmdNoInOutBool, out)

// ILibraryAppletAccessor

void appletHolderClose(AppletHolder *h) {
    eventClose(&h->PopInteractiveOutDataEvent);

    eventClose(&h->StateChangedEvent);
    serviceAssumeDomain(&h->s);
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

IPC_MAKE_CMD_IMPL(Result appletHolderTerminate(AppletHolder *h), &h->s, 25, _appletCmdNoIO)

static Result _appletAccessorRequestExitOrTerminate(Service* srv, u64 timeout) {
    Result rc=0;
    Event StateChangedEvent={0};

    if (!serviceIsActive(srv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdGetEvent(srv, &StateChangedEvent, false, 0);//GetAppletStateChangedEvent

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

    eventWait(&h->StateChangedEvent, UINT64_MAX);
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

LibAppletExitReason appletHolderGetExitReason(AppletHolder *h) {
    return h->exitreason;
}

IPC_MAKE_CMD_IMPL_INITEXPR(Result appletHolderSetOutOfFocusApplicationSuspendingEnabled(AppletHolder *h, bool flag), &h->s, 50, _appletCmdInBoolNoOut, !_appletIsApplication(), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletHolderPresetLibraryAppletGpuTimeSliceZero(AppletHolder *h), &h->s, 60, _appletCmdNoIO, (10,0,0))

Result appletHolderGetPopInteractiveOutDataEvent(AppletHolder *h, Event **out_event) {
    Result rc=0;

    if (!eventActive(&h->PopInteractiveOutDataEvent)) rc = _appletCmdGetEvent(&h->s, &h->PopInteractiveOutDataEvent, false, 106);
    if (R_SUCCEEDED(rc) && out_event) *out_event = &h->PopInteractiveOutDataEvent;
    return rc;
}

bool appletHolderWaitInteractiveOut(AppletHolder *h) {
    Result rc=0;
    s32 idx = 0;
    Event *event = NULL;

    rc = appletHolderGetPopInteractiveOutDataEvent(h, &event);
    if (R_FAILED(rc)) return false;

    rc = waitMulti(&idx, UINT64_MAX, waiterForEvent(event), waiterForEvent(&h->StateChangedEvent));
    if (R_FAILED(rc)) return false;

    return idx==0;
}

IPC_MAKE_CMD_IMPL(Result appletHolderPushInData(AppletHolder *h, AppletStorage *s),              &h->s, 100, _appletCmdInStorage,         s)
IPC_MAKE_CMD_IMPL(Result appletHolderPopOutData(AppletHolder *h, AppletStorage *s),              &h->s, 101, _appletCmdNoInOutStorage,    s)
IPC_MAKE_CMD_IMPL(Result appletHolderPushExtraStorage(AppletHolder *h, AppletStorage *s),        &h->s, 102, _appletCmdInStorage,         s)
IPC_MAKE_CMD_IMPL(Result appletHolderPushInteractiveInData(AppletHolder *h, AppletStorage *s),   &h->s, 103, _appletCmdInStorage,         s)
IPC_MAKE_CMD_IMPL(Result appletHolderPopInteractiveOutData(AppletHolder *h, AppletStorage *s),   &h->s, 104, _appletCmdNoInOutStorage,    s)
IPC_MAKE_CMD_IMPL(Result appletHolderGetLibraryAppletInfo(AppletHolder *h, LibAppletInfo *info), &h->s, 120, _appletGetLibraryAppletInfo, info)

// (ILibraryAppletCreator ->) IStorage

Result appletCreateStorage(AppletStorage *s, s64 size) {
    memset(s, 0, sizeof(AppletStorage));

    return _appletCmdGetSessionInU64(&g_appletILibraryAppletCreator, &s->s, size, 10);
}

static Result _appletCreateTransferMemoryStorage(Service* srv_out, TransferMemory *tmem, bool writable) {
    const struct {
        u8 writable;
        u64 size;
    } in = { writable!=0, tmem->size };

    serviceAssumeDomain(&g_appletILibraryAppletCreator);
    return serviceDispatchIn(&g_appletILibraryAppletCreator, 11, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
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

    return _appletCmdInHandleU64OutSession(&g_appletILibraryAppletCreator, &s->s, handle, inval, 12);
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
    serviceAssumeDomain(&s->s);
    serviceClose(&s->s);
}

void appletStorageCloseTmem(AppletStorage *s) {
    tmemClose(&s->tmem);
}

static Result _appletStorageAccessorRW(Service* srv, size_t ipcbufsize, s64 offset, void* buffer, size_t size, bool rw) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, rw ? 10 : 11, offset,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | (rw ? SfBufferAttr_In : SfBufferAttr_Out) },
        .buffers = { { buffer, size } },
    );
}

Result appletStorageGetSize(AppletStorage *s, s64 *size) {
    Result rc=0;
    Service tmp_srv;//IStorageAccessor

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdGetSession(&s->s, &tmp_srv, 0);//Open
    if (R_FAILED(rc)) return rc;

    rc = _appletCmdNoInOutU64(&tmp_srv, (u64*)size, 0);
    serviceAssumeDomain(&tmp_srv);
    serviceClose(&tmp_srv);

    return rc;
}

static Result _appletStorageRW(AppletStorage *s, s64 offset, void* buffer, size_t size, bool rw) {
    Result rc=0;
    Service tmp_srv;//IStorageAccessor

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdGetSession(&s->s, &tmp_srv, 0);//Open
    if (R_FAILED(rc)) return rc;

    if (R_SUCCEEDED(rc)) rc = _appletStorageAccessorRW(&tmp_srv, tmp_srv.pointer_buffer_size, offset, buffer, size, rw);
    serviceAssumeDomain(&tmp_srv);
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
    Handle tmphandle = INVALID_HANDLE;
    serviceAssumeDomain(srv);
    Result rc = serviceDispatchOut(srv, 1, *out,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmphandle,
    );
    if (R_SUCCEEDED(rc) && handle) *handle = tmphandle;
    return rc;
}

Result appletStorageGetHandle(AppletStorage *s, s64 *out, Handle *handle) {
    Result rc=0;
    Service tmp_srv;//ITransferStorageAccessor

    if (!serviceIsActive(&s->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdGetSession(&s->s, &tmp_srv, 1);//OpenTransferStorage
    if (R_FAILED(rc)) return rc;

    rc = _appletStorageGetHandle(&tmp_srv, out, handle);
    serviceAssumeDomain(&tmp_srv);
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
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=kind;
    memset(s, 0, sizeof(AppletStorage));
    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 1, tmp,
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
}

static Result _appletCreateApplicationAndPushAndRequestToStart(Service* srv, u64 application_id, AppletStorage* s, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, application_id,
        .in_num_objects = 1,
        .in_objects = { &s->s },
    );
}

static Result _appletCreateApplicationAndPushAndRequestToStartForQuest(u64 application_id, AppletStorage* s, const AppletApplicationAttributeForQuest *attr) { // [2.0.0+]
    const struct {
        u32 val0, val1;
        u64 application_id;
    } in = { attr->unk_x0, attr->unk_x4, application_id };

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 11, in,
        .in_num_objects = 1,
        .in_objects = { &s->s },
    );
}

static Result _appletCreateApplicationAndRequestToStart(u64 application_id) { // [4.0.0+]
    return _appletCmdInU64NoOut(&g_appletIFunctions, application_id, 12);
}

static Result _appletCreateApplicationAndRequestToStartForQuest(u64 application_id, const AppletApplicationAttributeForQuest *attr) { // [4.0.0+]
    const struct {
        u32 val0, val1;
        u64 application_id;
    } in = { attr->unk_x0, attr->unk_x4, application_id };

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 13, in);
}

static Result _appletCreateApplicationWithAttributeAndPushAndRequestToStartForQuest(u64 application_id, AppletStorage* s, const AppletApplicationAttribute *attr) { // [7.0.0+]
    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 14, application_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { attr, sizeof(*attr) } },
        .in_num_objects = 1,
        .in_objects = { &s->s },
    );
}

static Result _appletCreateApplicationWithAttributeAndRequestToStartForQuest(u64 application_id, const AppletApplicationAttribute *attr) { // [7.0.0+]
    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 15, application_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { attr, sizeof(*attr) } },
    );
}

Result appletRequestLaunchApplication(u64 application_id, AppletStorage* s) {
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
        rc = _appletCreateApplicationAndPushAndRequestToStart(&g_appletILibraryAppletSelfAccessor, application_id, s, 90);
    }
    else {
        if (hosversionAtLeast(4,0,0) && s==NULL) {
            rc = _appletCreateApplicationAndRequestToStart(application_id);
        }
        else {
            rc = _appletCreateApplicationAndPushAndRequestToStart(&g_appletIFunctions, application_id, s, 10);
        }
    }

    if (s) appletStorageClose(s);

    return rc;
}

Result appletRequestLaunchApplicationForQuest(u64 application_id, AppletStorage* s, const AppletApplicationAttributeForQuest *attr) {
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
            rc = _appletCreateApplicationWithAttributeAndRequestToStartForQuest(application_id, &appattr);
        else
            rc = _appletCreateApplicationAndRequestToStartForQuest(application_id, attr);
    }
    else {
        if (hosversionBefore(3,0,0)) rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
        if (R_SUCCEEDED(rc)) {
            if (hosversionAtLeast(7,0,0))
                rc = _appletCreateApplicationWithAttributeAndPushAndRequestToStartForQuest(application_id, s, &appattr);
            else
                rc = _appletCreateApplicationAndPushAndRequestToStartForQuest(application_id, s, attr);
        }
    }

    if (s) appletStorageClose(s);

    return rc;
}

IPC_MAKE_CMD_IMPL_INITEXPR(Result appletGetDesiredLanguage(u64 *LanguageCode), &g_appletIFunctions, 21, _appletCmdNoInOutU64, !_appletIsApplication(), LanguageCode)

Result appletGetDisplayVersion(char *displayVersion) {
    char out[0x10]={0};

    if (displayVersion) memset(displayVersion, 0, 0x10);

    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_appletIFunctions);
    Result rc = serviceDispatchOut(&g_appletIFunctions, 23, out);
    if (R_SUCCEEDED(rc) && displayVersion) {
        strncpy(displayVersion, out, 0x10);
        displayVersion[0xf] = 0;
    }
    return rc;
}

IPC_MAKE_CMD_IMPL_INITEXPR(Result appletBeginBlockingHomeButtonShortAndLongPressed(s64 val), &g_appletIFunctions, 30, _appletCmdInU64NoOut, !_appletIsApplication(), val)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletEndBlockingHomeButtonShortAndLongPressed(void),      &g_appletIFunctions, 31, _appletCmdNoIO,       !_appletIsApplication())
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletBeginBlockingHomeButton(s64 val),                    &g_appletIFunctions, 32, _appletCmdInU64NoOut, !_appletIsApplication(), val)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletEndBlockingHomeButton(void),                         &g_appletIFunctions, 33, _appletCmdNoIO,       !_appletIsApplication())

void appletNotifyRunning(bool *out) {
    if (__nx_applet_type!=AppletType_Application || g_appletNotifiedRunning) return;
    g_appletNotifiedRunning = 1;

    Result rc = _appletCmdNoInOutBool(&g_appletIFunctions, out, 40);

    if (R_FAILED(rc)) diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadAppletNotifyRunning));
}

Result appletGetPseudoDeviceId(Uuid *out) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchOut(&g_appletIFunctions, 50, *out);
}

Result appletSetMediaPlaybackState(bool state) {
    if (!serviceIsActive(&g_appletSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (!_appletIsApplication())
        return _appletCmdInBoolNoOut(&g_appletISelfController, state, 61);//SetMediaPlaybackState

    return _appletCmdInBoolNoOut(&g_appletIFunctions, state, 60);//SetMediaPlaybackStateForApplication
}

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(       Result appletIsGamePlayRecordingSupported(bool *flag),           &g_appletIFunctions, 65, _appletCmdNoInOutBool, !_appletIsRegularApplication(),                                    (3,0,0), flag)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(static Result _appletInitializeGamePlayRecording(TransferMemory *tmem), &g_appletIFunctions, 66, _appletCmdInTmemNoOut, !_appletIsRegularApplication(),                                    (3,0,0), tmem)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(       Result appletSetGamePlayRecordingState(bool state),              &g_appletIFunctions, 67, _appletCmdInU32NoOut,  !_appletIsRegularApplication() || g_appletRecordingInitialized==0, (3,0,0), state!=0)

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

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletRequestFlushGamePlayingMovieForDebug(void), &g_appletIFunctions, 68, _appletCmdNoIO, !_appletIsApplication(), (4,0,0))

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

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletRequestToSleep(void), &g_appletIFunctions, 72, _appletCmdNoIO, !_appletIsApplication(), (10,0,0))

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
    const struct {
        s32 width;
        s32 height;
        u64 size;
    } in = { width, height, tmem->size };

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 100, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
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

Result appletSetApplicationCopyrightImage(const void* buffer, size_t size, s32 x, s32 y, s32 width, s32 height, AppletWindowOriginMode mode) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 x;
        s32 y;
        s32 width;
        s32 height;
        s32 mode;
    } in = { x, y, width, height, mode };

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 101, in,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result appletSetApplicationCopyrightVisibility(bool visible) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletCmdInBoolNoOut(&g_appletIFunctions, visible, 102);
}

//Official sw has these under 'pdm'.
Result appletQueryApplicationPlayStatistics(PdmApplicationPlayStatistics *stats, const u64 *application_ids, s32 count, s32 *total_out) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchOut(&g_appletIFunctions, 110, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { stats, count*sizeof(PdmApplicationPlayStatistics) },
            { application_ids, count*sizeof(u64) },
        },
    );
}

Result appletQueryApplicationPlayStatisticsByUid(AccountUid uid, PdmApplicationPlayStatistics *stats, const u64 *application_ids, s32 count, s32 *total_out) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchInOut(&g_appletIFunctions, 111, uid, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { stats, count*sizeof(PdmApplicationPlayStatistics) },
            { application_ids, count*sizeof(u64) },
        },
    );
}

static Result _appletExecuteProgramCmd(AppletProgramSpecifyKind kind, u64 inval) {
    if (!serviceIsActive(&g_appletSrv) || !_appletIsApplication())
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 kind;
        u64 inval;
    } in = { kind, inval };

    serviceAssumeDomain(&g_appletIFunctions);
    return serviceDispatchIn(&g_appletIFunctions, 120, in);
}

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(static Result _appletClearUserChannel(void),               &g_appletIFunctions, 121, _appletCmdNoIO,      !_appletIsApplication(), (5,0,0))
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(static Result _appletUnpopToUserChannel(AppletStorage *s), &g_appletIFunctions, 122, _appletCmdInStorage, !_appletIsApplication(), (5,0,0), s)

static Result _appletExecuteProgram(AppletProgramSpecifyKind kind, u64 inval, const void* buffer, size_t size) {
    Result rc=0;
    AppletStorage storage={0};

    if (size > 0x1000)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (buffer!=NULL && size!=0) {
        rc = appletCreateStorage(&storage, size);
        if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&storage, 0, buffer, size);
    }

    if (R_SUCCEEDED(rc)) rc = _appletClearUserChannel();
    if (R_SUCCEEDED(rc) && buffer!=0 && size!=0) rc = _appletUnpopToUserChannel(&storage);
    if (R_SUCCEEDED(rc)) rc = _appletExecuteProgramCmd(kind, inval);

    appletStorageClose(&storage);

    return rc;
}

Result appletExecuteProgram(s32 programIndex, const void* buffer, size_t size) {
    Result rc=0;

    if (programIndex<0 || programIndex>0xff) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (R_SUCCEEDED(rc)) rc = _appletExecuteProgram(AppletProgramSpecifyKind_ExecuteProgram, (u64)programIndex, buffer, size);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

Result appletJumpToSubApplicationProgramForDevelopment(u64 application_id, const void* buffer, size_t size) {
    return _appletExecuteProgram(AppletProgramSpecifyKind_JumpToSubApplicationProgramForDevelopment, application_id, buffer, size);
}

Result appletRestartProgram(const void* buffer, size_t size) {
    return _appletExecuteProgram(AppletProgramSpecifyKind_RestartProgram, 0, buffer, size);
}

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetPreviousProgramIndex(s32 *programIndex),                 &g_appletIFunctions, 123,  _appletCmdNoInOutU32,       !_appletIsApplication(), (5,0,0), (u32*)programIndex)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetDelayTimeToAbortOnGpuError(u64 val),                     &g_appletIFunctions, 131,  _appletCmdInU64NoOut,       !_appletIsApplication(), (11,0,0), val)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetFriendInvitationStorageChannelEvent(Event *out_event),   &g_appletIFunctions, 140,  _appletCmdGetEvent,         !_appletIsApplication(), (9,0,0), out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletTryPopFromFriendInvitationStorageChannel(AppletStorage *s), &g_appletIFunctions, 141,  _appletCmdNoInOutStorage,   !_appletIsApplication(), (9,0,0), s)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetNotificationStorageChannelEvent(Event *out_event),       &g_appletIFunctions, 150,  _appletCmdGetEvent,         !_appletIsApplication(), (9,0,0), out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletTryPopFromNotificationStorageChannel(AppletStorage *s),     &g_appletIFunctions, 151,  _appletCmdNoInOutStorage,   !_appletIsApplication(), (9,0,0), s)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetHealthWarningDisappearedSystemEvent(Event *out_event),   &g_appletIFunctions, 160,  _appletCmdGetEvent,         !_appletIsApplication(), (9,0,0), out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetHdcpAuthenticationActivated(bool flag),                  &g_appletIFunctions, 170,  _appletCmdInBoolNoOut,      !_appletIsApplication(), (9,0,0), flag)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetLastApplicationExitReason(s32 *out),                     &g_appletIFunctions, 200,  _appletCmdNoInOutU32,       !_appletIsApplication(), (11,0,0), (u32*)out)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletCreateMovieMaker(Service* srv_out, TransferMemory *tmem),   &g_appletIFunctions, 1000, _appletCmdInTmemOutSession, !_appletIsApplication(), (5,0,0), srv_out, tmem)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletPrepareForJit(void),                                        &g_appletIFunctions, 1001, _appletCmdNoIO,             !_appletIsApplication(), (5,0,0))

// IHomeMenuFunctions

IPC_MAKE_CMD_IMPL_INITEXPR(Result appletRequestToGetForeground(void),                           &g_appletIFunctions, 10, _appletCmdNoIO,                     __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletLockForeground(void),                                   &g_appletIFunctions, 11, _appletCmdNoIO,                     __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletUnlockForeground(void),                                 &g_appletIFunctions, 12, _appletCmdNoIO,                     __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletPopFromGeneralChannel(AppletStorage *s),                &g_appletIFunctions, 20, _appletCmdNoInOutStorage,           __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet, s)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletGetPopFromGeneralChannelEvent(Event *out_event),        &g_appletIFunctions, 21, _appletCmdGetEvent,                 __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet, out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletGetHomeButtonWriterLockAccessor(AppletLockAccessor *a), &g_appletIFunctions, 30, _appletGetHomeButtonRwLockAccessor, __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet, a)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletIsSleepEnabled(bool *out),                       &g_appletIFunctions, 40,  _appletCmdNoInOutBool, __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet, (11,0,0), out)

Result appletPopRequestLaunchApplicationForDebug(AccountUid *uids, s32 count, u64 *application_id, s32 *total_out) {
    if (__nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 application_id;
        s32 total_out;
    } out;

    serviceAssumeDomain(&g_appletIFunctions);
    Result rc = serviceDispatchOut(&g_appletIFunctions, 100, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { uids, count*sizeof(AccountUid) } },
    );
    if (R_SUCCEEDED(rc) && application_id) *application_id = out.application_id;
    if (R_SUCCEEDED(rc) && total_out) *total_out = out.total_out;
    return rc;
}

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletIsForceTerminateApplicationDisabledForDebug(bool *out), &g_appletIFunctions, 110,  _appletCmdNoInOutBool, __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet, (9,0,0), out)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetLastApplicationExitReason(s32 reason),               &g_appletIFunctions, 1000, _appletCmdInU32NoOut,  __nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet, (11,0,0), reason)

Result appletLaunchDevMenu(void) {
    Result rc=0;

    if (__nx_applet_type != AppletType_SystemApplet && __nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _appletCmdNoIO(&g_appletIFunctions, 200);
    if (R_SUCCEEDED(rc)) _appletInfiniteSleepLoop();
    return rc;
}

// IGlobalStateController

IPC_MAKE_CMD_IMPL(       Result appletStartSleepSequence(bool flag),                      &g_appletIGlobalStateController, 2,  _appletCmdInBoolNoOut, flag)
IPC_MAKE_CMD_IMPL(       Result appletStartShutdownSequence(void),                        &g_appletIGlobalStateController, 3,  _appletCmdNoIO         )
IPC_MAKE_CMD_IMPL(       Result appletStartRebootSequence(void),                          &g_appletIGlobalStateController, 4,  _appletCmdNoIO         )
IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsAutoPowerDownRequested(bool *out),                &g_appletIGlobalStateController, 9,  _appletCmdNoInOutBool, (7,0,0), out)
IPC_MAKE_CMD_IMPL(       Result appletLoadAndApplyIdlePolicySettings(void),               &g_appletIGlobalStateController, 10, _appletCmdNoIO         )
IPC_MAKE_CMD_IMPL_HOSVER(Result appletNotifyCecSettingsChanged(void),                     &g_appletIGlobalStateController, 11, _appletCmdNoIO,        (2,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetDefaultHomeButtonLongPressTime(s64 val),         &g_appletIGlobalStateController, 12, _appletCmdInU64NoOut,  (3,0,0), val)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletUpdateDefaultDisplayResolution(void),               &g_appletIGlobalStateController, 13, _appletCmdNoIO,        (3,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletShouldSleepOnBoot(bool *out),                       &g_appletIGlobalStateController, 14, _appletCmdNoInOutBool, (3,0,0), out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetHdcpAuthenticationFailedEvent(Event *out_event), &g_appletIGlobalStateController, 15, _appletCmdGetEvent,    (4,0,0), out_event, false)

// IApplicationCreator

static Result _appletApplicationCreateState(AppletApplication *a) {
    return _appletCmdGetEvent(&a->s, &a->StateChangedEvent, false, 0);//GetAppletStateChangedEvent
}

static Result _appletApplicationCreate(Service* srv, AppletApplication *a, u32 cmd_id) {
    Result rc=0;

    memset(a, 0, sizeof(AppletApplication));

    rc = _appletCmdGetSession(srv, &a->s, cmd_id);

    if (R_SUCCEEDED(rc)) rc = _appletApplicationCreateState(a);

    return rc;
}

static Result _appletApplicationCreateIn64(Service* srv, AppletApplication *a, u64 val, u32 cmd_id) {
    Result rc=0;

    memset(a, 0, sizeof(AppletApplication));

    rc = _appletCmdGetSessionInU64(srv, &a->s, val, cmd_id);

    if (R_SUCCEEDED(rc)) rc = _appletApplicationCreateState(a);

    return rc;
}

IPC_MAKE_CMD_IMPL_INITEXPR(Result appletCreateApplication(AppletApplication *a, u64 application_id), &g_appletIApplicationCreator, 0,   _appletApplicationCreateIn64, __nx_applet_type != AppletType_SystemApplet, a, application_id)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletPopLaunchRequestedApplication(AppletApplication *a),         &g_appletIApplicationCreator, 1,   _appletApplicationCreate,     __nx_applet_type != AppletType_SystemApplet, a)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletCreateSystemApplication(AppletApplication *a, u64 system_application_id), &g_appletIApplicationCreator, 10,  _appletApplicationCreateIn64, __nx_applet_type != AppletType_SystemApplet, a, system_application_id)
IPC_MAKE_CMD_IMPL_INITEXPR(Result appletPopFloatingApplicationForDevelopment(AppletApplication *a),  &g_appletIApplicationCreator, 100, _appletApplicationCreate,     __nx_applet_type != AppletType_SystemApplet, a)

// IApplicationAccessor

void appletApplicationClose(AppletApplication *a) {
    eventClose(&a->StateChangedEvent);
    serviceAssumeDomain(&a->s);
    serviceClose(&a->s);
    memset(a, 0, sizeof(AppletApplication));
}

bool appletApplicationActive(AppletApplication *a) {
    return serviceIsActive(&a->s);
}

IPC_MAKE_CMD_IMPL(Result appletApplicationStart(AppletApplication *a),       &a->s, 10, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(Result appletApplicationRequestExit(AppletApplication *a), &a->s, 20, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(Result appletApplicationTerminate(AppletApplication *a),   &a->s, 25, _appletCmdNoIO)

void appletApplicationJoin(AppletApplication *a) {
    Result rc=0;
    AppletApplicationExitReason res = AppletApplicationExitReason_Normal;
    u32 desc=0;

    eventWait(&a->StateChangedEvent, UINT64_MAX);
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

IPC_MAKE_CMD_IMPL(Result appletApplicationRequestForApplicationToGetForeground(AppletApplication *a), &a->s, 101, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(Result appletApplicationTerminateAllLibraryApplets(AppletApplication *a),           &a->s, 110, _appletCmdNoIO)
IPC_MAKE_CMD_IMPL(Result appletApplicationAreAnyLibraryAppletsLeft(AppletApplication *a, bool *out),  &a->s, 111, _appletCmdNoInOutBool, out)

Result appletApplicationRequestExitLibraryAppletOrTerminate(AppletApplication *a, u64 timeout) {
    Result rc=0;
    Service srv={0};

    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _appletCmdGetSession(&a->s, &srv, 112);//GetCurrentLibraryApplet
    if (R_SUCCEEDED(rc)) rc = _appletAccessorRequestExitOrTerminate(&srv, timeout);
    serviceAssumeDomain(&srv);
    serviceClose(&srv);

    return rc;
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationGetApplicationId(AppletApplication *a, u64 *application_id), &a->s, 120, _appletCmdNoInOutU64, (6,0,0), application_id)

Result appletApplicationPushLaunchParameter(AppletApplication *a, AppletLaunchParameterKind kind, AppletStorage* s) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletCmdInStorageU32(&a->s, s, kind, 121);
}

IPC_MAKE_CMD_IMPL(       Result appletApplicationGetApplicationControlProperty(AppletApplication *a, NacpStruct *nacp),                    &a->s, 122, _appletCmdNoInRecvBuf,          nacp, sizeof(*nacp))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationGetApplicationLaunchProperty(AppletApplication *a, AppletApplicationLaunchProperty *out), &a->s, 123, _appletCmdNoInRecvBuf, (2,0,0), out, sizeof(*out))

Result appletApplicationGetApplicationLaunchRequestInfo(AppletApplication *a, AppletApplicationLaunchRequestInfo *out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&a->s);
    return serviceDispatchOut(&a->s, 124, *out);
}

Result appletApplicationSetUsers(AppletApplication *a, const AccountUid *uids, s32 count, bool flag) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp = flag!=0;
    serviceAssumeDomain(&a->s);
    return serviceDispatchIn(&a->s, 130, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { uids, count*sizeof(AccountUid) } },
    );
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationCheckRightsEnvironmentAvailable(AppletApplication *a, bool *out), &a->s, 131, _appletCmdNoInOutBool, (6,0,0), out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationGetNsRightsEnvironmentHandle(AppletApplication *a, u64 *handle),  &a->s, 132, _appletCmdNoInOutU64,  (6,0,0), handle)

Result appletApplicationGetDesirableUids(AppletApplication *a, AccountUid *uids, s32 count, s32 *total_out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&a->s);
    return serviceDispatchOut(&a->s, 140, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { uids, count*sizeof(AccountUid) } },
    );
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationReportApplicationExitTimeout(AppletApplication *a),                                    &a->s, 150, _appletCmdNoIO,         (6,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationSetApplicationAttribute(AppletApplication *a, const AppletApplicationAttribute *attr), &a->s, 160, _appletCmdSendBufNoOut, (8,0,0), attr, sizeof(*attr))

Result appletApplicationHasSaveDataAccessPermission(AppletApplication *a, u64 application_id, bool *out) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmpout=0;
    serviceAssumeDomain(&a->s);
    Result rc = serviceDispatchInOut(&a->s, 170, application_id, tmpout);
    if (R_SUCCEEDED(rc) && out) *out = tmpout & 1;
    return rc;
}

static Result _appletPushToFriendInvitationStorageChannel(Service* srv, AccountUid uid, const void* buffer, u64 size, u32 cmd_id) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    AppletStorage storage;

    rc = appletCreateStorage(&storage, size+sizeof(uid));
    if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&storage, 0, &uid, sizeof(uid));
    if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&storage, sizeof(uid), buffer, size);
    if (R_SUCCEEDED(rc)) rc = _appletCmdInStorage(srv, &storage, cmd_id);
    appletStorageClose(&storage);

    return rc;
}

static Result _appletPushToNotificationStorageChannel(Service* srv, const void* buffer, u64 size, u32 cmd_id) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    AppletStorage storage;

    rc = appletCreateStorage(&storage, size);
    if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&storage, 0, buffer, size);
    if (R_SUCCEEDED(rc)) rc = _appletCmdInStorage(srv, &storage, cmd_id);
    appletStorageClose(&storage);

    return rc;
}

Result appletApplicationPushToFriendInvitationStorageChannel(AppletApplication *a, AccountUid uid, const void* buffer, u64 size) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletPushToFriendInvitationStorageChannel(&a->s, uid, buffer, size, 180);
}

Result appletApplicationPushToNotificationStorageChannel(AppletApplication *a, const void* buffer, u64 size) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _appletPushToNotificationStorageChannel(&a->s, buffer, size, 190);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationRequestApplicationSoftReset(AppletApplication *a),       &a->s, 200, _appletCmdNoIO, (10,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletApplicationRestartApplicationTimer(AppletApplication *a),           &a->s, 201, _appletCmdNoIO, (10,0,0))

// ILibraryAppletSelfAccessor

IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletPopInData(AppletStorage *s),                               &g_appletILibraryAppletSelfAccessor, 0,  _appletCmdNoInOutStorage,    __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletPushOutData(AppletStorage *s),                             &g_appletILibraryAppletSelfAccessor, 1,  _appletCmdInStorage,         __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletPopInteractiveInData(AppletStorage *s),                    &g_appletILibraryAppletSelfAccessor, 2,  _appletCmdNoInOutStorage,    __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletPushInteractiveOutData(AppletStorage *s),                  &g_appletILibraryAppletSelfAccessor, 3,  _appletCmdInStorage,         __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetPopInDataEvent(Event *out_event),                       &g_appletILibraryAppletSelfAccessor, 5,  _appletCmdGetEvent,             __nx_applet_type != AppletType_LibraryApplet,          out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetPopInteractiveInDataEvent(Event *out_event),            &g_appletILibraryAppletSelfAccessor, 6,  _appletCmdGetEvent,             __nx_applet_type != AppletType_LibraryApplet,          out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR(static Result _appletExitProcessAndReturn(void),                               &g_appletILibraryAppletSelfAccessor, 10, _appletCmdNoIO,              __nx_applet_type != AppletType_LibraryApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetLibraryAppletInfo(LibAppletInfo *info),                 &g_appletILibraryAppletSelfAccessor, 11, _appletGetLibraryAppletInfo, __nx_applet_type != AppletType_LibraryApplet,          info)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetMainAppletIdentityInfo(AppletIdentityInfo *info),       &g_appletILibraryAppletSelfAccessor, 12, _appletGetIdentityInfo,      __nx_applet_type != AppletType_LibraryApplet,          info)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletCanUseApplicationCore(bool *out),                          &g_appletILibraryAppletSelfAccessor, 13, _appletCmdNoInOutBool,       __nx_applet_type != AppletType_LibraryApplet,          out)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetCallerAppletIdentityInfo(AppletIdentityInfo *info),     &g_appletILibraryAppletSelfAccessor, 14, _appletGetIdentityInfo,      __nx_applet_type != AppletType_LibraryApplet,          info)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetMainAppletApplicationControlProperty(NacpStruct *nacp), &g_appletILibraryAppletSelfAccessor, 15, _appletCmdNoInRecvBuf,       __nx_applet_type != AppletType_LibraryApplet, (2,0,0), nacp, sizeof(*nacp))

Result appletGetMainAppletStorageId(NcmStorageId *storageId) {
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

    serviceAssumeDomain(&g_appletILibraryAppletSelfAccessor);
    return serviceDispatchOut(&g_appletILibraryAppletSelfAccessor, 17, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { stack, count*sizeof(AppletIdentityInfo) } },
    );
}

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetNextReturnDestinationAppletIdentityInfo(AppletIdentityInfo *info), &g_appletILibraryAppletSelfAccessor, 18,  _appletGetIdentityInfo,     __nx_applet_type != AppletType_LibraryApplet, (4,0,0), info)

Result appletGetDesirableKeyboardLayout(SetKeyboardLayout *layout) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _appletCmdNoInOutU32(&g_appletILibraryAppletSelfAccessor, &tmp, 19);
    if (R_SUCCEEDED(rc) && layout) *layout = tmp;
    return rc;
}

IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletPopExtraStorage(AppletStorage *s),                                    &g_appletILibraryAppletSelfAccessor, 20,  _appletCmdNoInOutStorage,   __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetPopExtraStorageEvent(Event *out_event),                            &g_appletILibraryAppletSelfAccessor, 25,  _appletCmdGetEvent,         __nx_applet_type != AppletType_LibraryApplet,          out_event, false)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletUnpopInData(AppletStorage *s),                                        &g_appletILibraryAppletSelfAccessor, 30,  _appletCmdInStorage,        __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletUnpopExtraStorage(AppletStorage *s),                                  &g_appletILibraryAppletSelfAccessor, 31,  _appletCmdInStorage,        __nx_applet_type != AppletType_LibraryApplet,          s)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetIndirectLayerProducerHandle(u64 *out),                             &g_appletILibraryAppletSelfAccessor, 40,  _appletCmdNoInOutU64,       __nx_applet_type != AppletType_LibraryApplet, (2,0,0), out)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetMainAppletApplicationDesiredLanguage(u64 *LanguageCode),           &g_appletILibraryAppletSelfAccessor, 60,  _appletCmdNoInOutU64,       __nx_applet_type != AppletType_LibraryApplet, (4,0,0), LanguageCode)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletGetCurrentApplicationId(u64 *application_id),                         &g_appletILibraryAppletSelfAccessor, 70,  _appletCmdNoInOutU64,       __nx_applet_type != AppletType_LibraryApplet, (8,0,0), application_id)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletRequestExitToSelf(void),                                              &g_appletILibraryAppletSelfAccessor, 80,  _appletCmdNoIO,             __nx_applet_type != AppletType_LibraryApplet, (6,0,0))
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletCreateGameMovieTrimmer(Service* srv_out, TransferMemory *tmem),       &g_appletILibraryAppletSelfAccessor, 100, _appletCmdInTmemOutSession, __nx_applet_type != AppletType_LibraryApplet, (4,0,0), srv_out, tmem)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletReserveResourceForMovieOperation(void),                               &g_appletILibraryAppletSelfAccessor, 101, _appletCmdNoIO,             __nx_applet_type != AppletType_LibraryApplet, (5,0,0))
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletUnreserveResourceForMovieOperation(void),                             &g_appletILibraryAppletSelfAccessor, 102, _appletCmdNoIO,             __nx_applet_type != AppletType_LibraryApplet, (5,0,0))

Result appletGetMainAppletAvailableUsers(AccountUid *uids, s32 count, bool *flag, s32 *total_out) {
    if (__nx_applet_type != AppletType_LibraryApplet)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u8 flag;
        s32 total_out;
    } out;

    serviceAssumeDomain(&g_appletILibraryAppletSelfAccessor);
    Result rc = serviceDispatchOut(&g_appletILibraryAppletSelfAccessor, 110, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { uids, count*sizeof(AccountUid) } },
    );
    if (R_SUCCEEDED(rc) && flag) *flag = out.flag & 1;
    if (R_SUCCEEDED(rc) && total_out) *total_out = out.total_out;
    return rc;
}

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetApplicationMemoryReservation(u64 val),                             &g_appletILibraryAppletSelfAccessor, 140, _appletCmdInU64NoOut,        __nx_applet_type != AppletType_LibraryApplet, (10,0,0), val)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletShouldSetGpuTimeSliceManually(bool *out),                             &g_appletILibraryAppletSelfAccessor, 150, _appletCmdNoInOutBool,       __nx_applet_type != AppletType_LibraryApplet, (10,0,0), out)

// IOverlayFunctions

IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletBeginToWatchShortHomeButtonMessage(void),         &g_appletIFunctions, 0, _appletCmdNoIO,        __nx_applet_type != AppletType_OverlayApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletEndToWatchShortHomeButtonMessage(void),           &g_appletIFunctions, 1, _appletCmdNoIO,        __nx_applet_type != AppletType_OverlayApplet)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletGetApplicationIdForLogo(u64 *application_id),     &g_appletIFunctions, 2, _appletCmdNoInOutU64,  __nx_applet_type != AppletType_OverlayApplet,          application_id)
IPC_MAKE_CMD_IMPL_INITEXPR(       Result appletSetGpuTimeSliceBoost(u64 val),                    &g_appletIFunctions, 3, _appletCmdInU64NoOut,  __nx_applet_type != AppletType_OverlayApplet,          val)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetAutoSleepTimeAndDimmingTimeEnabled(bool flag), &g_appletIFunctions, 4, _appletCmdInBoolNoOut, __nx_applet_type != AppletType_OverlayApplet, (2,0,0), flag)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletTerminateApplicationAndSetReason(Result reason),  &g_appletIFunctions, 5, _appletCmdInU32NoOut,  __nx_applet_type != AppletType_OverlayApplet, (2,0,0), reason)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetScreenShotPermissionGlobally(bool flag),       &g_appletIFunctions, 6, _appletCmdInBoolNoOut, __nx_applet_type != AppletType_OverlayApplet, (3,0,0), flag)

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

IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletSetHealthWarningShowingState(bool flag),             &g_appletIFunctions, 30,  _appletCmdInBoolNoOut, __nx_applet_type != AppletType_OverlayApplet, (9,0,0), flag)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletIsHealthWarningRequired(bool *out),                  &g_appletIFunctions, 31, _appletCmdNoInOutBool,  __nx_applet_type != AppletType_OverlayApplet, (10,0,0), out)
IPC_MAKE_CMD_IMPL_INITEXPR_HOSVER(Result appletBeginToObserveHidInputForDevelop(void),              &g_appletIFunctions, 101, _appletCmdNoIO,        __nx_applet_type != AppletType_OverlayApplet, (5,0,0))

// IAppletCommonFunctions

Result appletReadThemeStorage(void* buffer, size_t size, u64 offset, u64 *transfer_size) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_appletIAppletCommonFunctions);
    return serviceDispatchInOut(&g_appletIAppletCommonFunctions, 10, offset, *transfer_size,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result appletWriteThemeStorage(const void* buffer, size_t size, u64 offset) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_appletIAppletCommonFunctions);
    return serviceDispatchIn(&g_appletIAppletCommonFunctions, 11, offset,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletPushToAppletBoundChannel(AppletStorage *s),     &g_appletIAppletCommonFunctions, 20, _appletCmdInStorage,      (9,0,0), s)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletTryPopFromAppletBoundChannel(AppletStorage *s), &g_appletIAppletCommonFunctions, 21, _appletCmdNoInOutStorage, (9,0,0), s)

IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetDisplayLogicalResolution(s32 *width, s32 *height), &g_appletIAppletCommonFunctions, 40, _appletGetResolution, (8,0,0), width, height)

Result appletSetDisplayMagnification(float x, float y, float width, float height) {
    if (!serviceIsActive(&g_appletIAppletCommonFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        float x;
        float y;
        float width;
        float height;
    } in = { x, y, width, height };

    serviceAssumeDomain(&g_appletIAppletCommonFunctions);
    return serviceDispatchIn(&g_appletIAppletCommonFunctions, 42, in);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetHomeButtonDoubleClickEnabled(bool flag), &g_appletIAppletCommonFunctions, 50, _appletCmdInBoolNoOut, (8,0,0), flag)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletGetHomeButtonDoubleClickEnabled(bool *out), &g_appletIAppletCommonFunctions, 51, _appletCmdNoInOutBool, (8,0,0), out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsHomeButtonShortPressedBlocked(bool *out), &g_appletIAppletCommonFunctions, 52, _appletCmdNoInOutBool, (10,0,0), out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletIsVrModeCurtainRequired(bool *out),         &g_appletIAppletCommonFunctions, 60, _appletCmdNoInOutBool, (11,0,0), out)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletSetCpuBoostRequestPriority(s32 priority),   &g_appletIAppletCommonFunctions, 70, _appletCmdInU32NoOut,  (11,0,0), priority)

// IDebugFunctions

Result appletOpenMainApplication(AppletApplication *a) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _appletApplicationCreate(&g_appletIDebugFunctions, a, 1);
}

IPC_MAKE_CMD_IMPL(Result appletPerformSystemButtonPressing(AppletSystemButtonType type), &g_appletIDebugFunctions, 10, _appletCmdInU32NoOut,     type)
IPC_MAKE_CMD_IMPL(Result appletInvalidateTransitionLayer(void),                          &g_appletIDebugFunctions, 20, _appletCmdNoIO)

Result appletRequestLaunchApplicationWithUserAndArgumentForDebug(u64 application_id, const AccountUid *uids, s32 total_uids, bool flag, const void* buffer, size_t size) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 flag;
        u64 application_id;
    } in = { flag!=0, application_id };

    serviceAssumeDomain(&g_appletIDebugFunctions);
    return serviceDispatchIn(&g_appletIDebugFunctions, 30, in,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { uids, total_uids*sizeof(AccountUid) },
            { buffer, size },
        },
    );
}

Result appletGetAppletResourceUsageInfo(AppletResourceUsageInfo *info) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_appletIDebugFunctions);
    return serviceDispatchOut(&g_appletIDebugFunctions, 40, *info);
}

IPC_MAKE_CMD_IMPL_HOSVER(Result appletPushToAppletBoundChannelForDebug(AppletStorage *s, s32 channel),                        &g_appletIDebugFunctions, 110, _appletCmdInStorageU32,                      (9,0,0), s, channel)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletTryPopFromAppletBoundChannelForDebug(AppletStorage *s, s32 channel),                    &g_appletIDebugFunctions, 111, _appletCmdInU32OutStorage,                   (9,0,0), s, channel)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletAlarmSettingNotificationEnableAppEventReserve(AppletStorage *s, u64 application_id),    &g_appletIDebugFunctions, 120, _appletCmdInStorageU64,                      (9,0,0), s, application_id)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletAlarmSettingNotificationDisableAppEventReserve(void),                                   &g_appletIDebugFunctions, 121, _appletCmdNoIO,                              (9,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletAlarmSettingNotificationPushAppEventNotify(const void* buffer, u64 size),               &g_appletIDebugFunctions, 122, _appletPushToNotificationStorageChannel,     (9,0,0), buffer, size)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletFriendInvitationSetApplicationParameter(AppletStorage *s, u64 application_id),          &g_appletIDebugFunctions, 130, _appletCmdInStorageU64,                      (9,0,0), s, application_id)
IPC_MAKE_CMD_IMPL_HOSVER(Result appletFriendInvitationClearApplicationParameter(void),                                        &g_appletIDebugFunctions, 131, _appletCmdNoIO,                              (9,0,0))
IPC_MAKE_CMD_IMPL_HOSVER(Result appletFriendInvitationPushApplicationParameter(AccountUid uid, const void* buffer, u64 size), &g_appletIDebugFunctions, 132, _appletPushToFriendInvitationStorageChannel, (9,0,0), uid, buffer, size)

Result appletCreateGeneralStorageForDebug(u64 id, u64 size) {
    if (!serviceIsActive(&g_appletIDebugFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 id;
        u64 size;
    } in = { id, size };

    serviceAssumeDomain(&g_appletIDebugFunctions);
    return serviceDispatchIn(&g_appletIDebugFunctions, 410, in);
}

Result appletReadGeneralStorageForDebug(void* buffer, size_t size, u64 id, u64 offset, u64 *out_size) {
    if (!serviceIsActive(&g_appletIDebugFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 id;
        u64 offset;
    } in = { id, offset };

    serviceAssumeDomain(&g_appletIDebugFunctions);
    return serviceDispatchInOut(&g_appletIDebugFunctions, 411, in, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result appletWriteGeneralStorageForDebug(const void* buffer, size_t size, u64 id, u64 offset) {
    if (!serviceIsActive(&g_appletIDebugFunctions))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 id;
        u64 offset;
    } in = { id, offset };

    serviceAssumeDomain(&g_appletIDebugFunctions);
    return serviceDispatchIn(&g_appletIDebugFunctions, 412, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

// Common cmds
Result appletSetTerminateResult(Result res) {
    if (!serviceIsActive(&g_appletSrv) || (!_appletIsApplication() && !serviceIsActive(&g_appletIAppletCommonFunctions)))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (serviceIsActive(&g_appletIAppletCommonFunctions) && hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service *srv = &g_appletIFunctions;
    u32 cmd_id = 22;
    if (serviceIsActive(&g_appletIAppletCommonFunctions)) {
        srv = &g_appletIAppletCommonFunctions;
        cmd_id = 0;
    }

    return _appletCmdInU32NoOut(srv, res, cmd_id);
}

Result appletGetLaunchStorageInfoForDebug(NcmStorageId *app_storageId, NcmStorageId *update_storageId) {
    if (!serviceIsActive(&g_appletSrv) || (!_appletIsApplication() && __nx_applet_type != AppletType_LibraryApplet))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (_appletIsApplication() && hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (__nx_applet_type == AppletType_LibraryApplet && hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service *srv = &g_appletIFunctions;
    u32 cmd_id = 24;
    if (__nx_applet_type == AppletType_LibraryApplet) {
        srv = &g_appletILibraryAppletSelfAccessor;
        cmd_id = 120;
    }

    struct {
        u8 app_storageId;
        u8 update_storageId;
    } out;

    serviceAssumeDomain(srv);
    Result rc = serviceDispatchOut(srv, cmd_id, out);
    if (R_SUCCEEDED(rc) && app_storageId) *app_storageId = out.app_storageId;
    if (R_SUCCEEDED(rc) && update_storageId) *update_storageId = out.update_storageId;
    return rc;
}

Result appletGetGpuErrorDetectedSystemEvent(Event *out_event) {
    if (!serviceIsActive(&g_appletSrv) || (!_appletIsApplication() && __nx_applet_type != AppletType_LibraryApplet))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (_appletIsApplication() && hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (__nx_applet_type == AppletType_LibraryApplet && hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service *srv = &g_appletIFunctions;
    if (__nx_applet_type == AppletType_LibraryApplet)
        srv = &g_appletILibraryAppletSelfAccessor;

    return _appletCmdGetEvent(srv, out_event, false, 130);
}

Result appletSetHandlingHomeButtonShortPressedEnabled(bool flag) {
    if (__nx_applet_type == AppletType_OverlayApplet && hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (__nx_applet_type != AppletType_OverlayApplet && hosversionBefore(9,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service *srv = &g_appletIFunctions;
    u32 cmd_id = 20;
    if (__nx_applet_type != AppletType_OverlayApplet && hosversionAtLeast(9,1,0)) {
        srv = &g_appletICommonStateGetter;
        cmd_id = 100;
    }

    return _appletCmdInBoolNoOut(srv, flag, cmd_id);
}

// State / other

AppletOperationMode appletGetOperationMode(void) {
    return g_appletOperationMode;
}

ApmPerformanceMode appletGetPerformanceMode(void) {
    return g_appletPerformanceMode;
}

AppletFocusState appletGetFocusState(void) {
    return (AppletFocusState)g_appletFocusState;
}

AppletInfo *appletGetAppletInfo(void) {
    if (!g_appletInfoInitialized) return NULL;
    return &g_appletInfo;
}

Event *appletGetMessageEvent(void) {
    return &g_appletMessageEvent;
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

        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadAppletReceiveMessage));
    }

    return 0;
}

bool appletProcessMessage(u32 msg) {
    Result rc=0;

    switch(msg) {
        case AppletMessage_ExitRequest:
            appletCallHook(AppletHookType_OnExitRequest);
            return false;
        break;

        case AppletMessage_FocusStateChanged:
            rc = _appletGetCurrentFocusState(&g_appletFocusState);
            if (R_FAILED(rc))
                diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetCurrentFocusState));

            appletCallHook(AppletHookType_OnFocusState);
        break;

        case AppletMessage_Resume:
            appletCallHook(AppletHookType_OnResume);
        break;

        case AppletMessage_OperationModeChanged:
            rc = _appletGetOperationMode(&g_appletOperationMode);
            if (R_FAILED(rc))
                diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetOperationMode));

            appletCallHook(AppletHookType_OnOperationMode);
        break;

        case AppletMessage_PerformanceModeChanged:
            rc = _appletGetPerformanceMode(&g_appletPerformanceMode);
            if (R_FAILED(rc))
                diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadAppletGetPerformanceMode));

            appletCallHook(AppletHookType_OnPerformanceMode);
        break;

        case AppletMessage_RequestToDisplay:
            appletCallHook(AppletHookType_RequestToDisplay);
        break;

        case AppletMessage_CaptureButtonShortPressed:
            appletCallHook(AppletHookType_OnCaptureButtonShortPressed);
        break;

        case AppletMessage_AlbumScreenShotTaken:
            appletCallHook(AppletHookType_OnAlbumScreenShotTaken);
        break;
    }

    return true;
}

bool appletMainLoop(void) {
    u32 msg = 0;

    if (R_FAILED(appletGetMessage(&msg))) return true;

    return appletProcessMessage(msg);
}
