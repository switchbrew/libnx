#include <string.h>
#include <switch.h>

static Handle g_viServiceSession = 0;
static u32 g_viServiceType = -1;
static Handle g_viIApplicationDisplayService = 0;
static Handle g_viIHOSBinderDriverRelay = 0;
static Handle g_viISystemDisplayService = 0;
static Handle g_viIManagerDisplayService = 0;
static Handle g_viIHOSBinderDriverIndirect = 0;

static Result _viGetSession(Handle sessionhandle, Handle* handle_out, void* inraw, size_t rawsize);
static Result _viGetSessionNoParams(Handle sessionhandle, Handle* handle_out, u64 cmd_id);

Result viInitialize(viServiceType servicetype) {
    if(g_viServiceType!=-1)return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;

    if (servicetype==VISERVTYPE_Default || servicetype==VISERVTYPE_Manager) {
        rc = smGetService(&g_viServiceSession, "vi:m");
        g_viServiceType = 2;
    }

    if ((servicetype==VISERVTYPE_Default && R_FAILED(rc)) || servicetype==VISERVTYPE_System) {
        rc = smGetService(&g_viServiceSession, "vi:s");
        g_viServiceType = 1;
    }

    if ((servicetype==VISERVTYPE_Default && R_FAILED(rc)) || servicetype==VISERVTYPE_Application)
    {
        rc = smGetService(&g_viServiceSession, "vi:u");
        g_viServiceType = 0;
    }

    if (R_SUCCEEDED(rc)) {
        struct {
            u64 magic;
            u64 cmd_id;
            u64 inval0;
        } raw;

        raw.magic = SFCI_MAGIC;
        raw.cmd_id = g_viServiceType;
        raw.inval0 = 0;

        rc = _viGetSession(g_viServiceSession, &g_viIApplicationDisplayService, &raw, sizeof(raw));
    }

    if (R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIHOSBinderDriverRelay, 100);
    if (g_viServiceType>=VISERVTYPE_System && R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viISystemDisplayService, 101);
    if (g_viServiceType>=VISERVTYPE_Manager && R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIManagerDisplayService, 102);
    if (g_viServiceType>=VISERVTYPE_System && R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIHOSBinderDriverIndirect, 103);

    if (R_FAILED(rc))g_viServiceType = -1;

    return rc;
}

void viExit(void)
{
    if(g_viServiceType==-1)return;

    g_viServiceType = -1;

    if(g_viServiceSession)
    {
        svcCloseHandle(g_viServiceSession);
        g_viServiceSession = 0;
    }

    if(g_viIApplicationDisplayService)
    {
        svcCloseHandle(g_viIApplicationDisplayService);
        g_viIApplicationDisplayService = 0;
    }

    if(g_viISystemDisplayService)
    {
        svcCloseHandle(g_viISystemDisplayService);
        g_viISystemDisplayService = 0;
    }

    if(g_viIManagerDisplayService)
    {
        svcCloseHandle(g_viIManagerDisplayService);
        g_viIManagerDisplayService = 0;
    }

    if(g_viIHOSBinderDriverIndirect)
    {
        svcCloseHandle(g_viIHOSBinderDriverIndirect);
        g_viIHOSBinderDriverIndirect = 0;
    }
}

Handle viGetSessionService(void)
{
    return g_viServiceSession;
}

Handle viGetSession_IApplicationDisplayService(void)
{
    return g_viIApplicationDisplayService;
}

Handle viGetSession_IHOSBinderDriverRelay(void)
{
    return g_viIHOSBinderDriverRelay;
}

Handle viGetSession_ISystemDisplayService(void)
{
    return g_viISystemDisplayService;
}

Handle viGetSession_IManagerDisplayService(void)
{
    return g_viIManagerDisplayService;
}

Handle viGetSession_IHOSBinderDriverIndirect(void)
{
    return g_viIHOSBinderDriverIndirect;
}

static Result _viGetSession(Handle sessionhandle, Handle* handle_out, void* inraw, size_t rawsize) {
    IpcCommand c;
    ipcInitialize(&c);

    memcpy(ipcPrepareHeader(&c, rawsize), inraw, rawsize);

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

static Result _viGetSessionNoParams(Handle sessionhandle, Handle* handle_out, u64 cmd_id) {
    struct {
        u64 magic;
        u64 cmd_id;
    } raw;

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = cmd_id;

    return _viGetSession(sessionhandle, handle_out, &raw, sizeof(raw));
}

Result viOpenDisplay(const char *DisplayName, viDisplay *display) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        char DisplayName[0x40];
    } *raw;

    memset(display, 0, sizeof(viDisplay));

    raw = ipcPrepareHeader(&c, sizeof(raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1010;

    strncpy(display->DisplayName, DisplayName, sizeof(display->DisplayName)-1);
    memcpy(raw->DisplayName, display->DisplayName, sizeof(display->DisplayName));

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 DisplayId;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            display->DisplayId = resp->DisplayId;
            display->initialized = 1;
        }
    }

    return rc;
}

Result viCloseDisplay(viDisplay *display) {
    IpcCommand c;
    ipcInitialize(&c);

    if(!display->initialized)return 0;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 DisplayId;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1020;
    raw->DisplayId = display->DisplayId;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        display->initialized = 0;
    }

    return rc;
}

static Result _viOpenLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const viDisplay *display, u64 LayerId, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        char DisplayName[0x40];
        u64 LayerId;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, NativeWindow, 0x100, 0);

    raw = ipcPrepareHeader(&c, sizeof(raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2020;

    memcpy(raw->DisplayName, display->DisplayName, sizeof(display->DisplayName));

    raw->LayerId = LayerId;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 NativeWindow_Size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *NativeWindow_Size = resp->NativeWindow_Size;
        }
    }

    return rc;
}

static Result _viCreateStrayLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const viDisplay *display, u32 LayerFlags, u64 *LayerId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 LayerFlags;
        u32 pad;
        u64 DisplayId;
    } *raw;

    ipcAddRecvBuffer(&c, NativeWindow, 0x100, 0);

    raw = ipcPrepareHeader(&c, sizeof(raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2030;
    raw->LayerFlags = LayerFlags;
    raw->DisplayId = display->DisplayId;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 LayerId;
            u64 NativeWindow_Size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *LayerId = resp->LayerId;
            *NativeWindow_Size = resp->NativeWindow_Size;
        }
    }

    return rc;
}

Result viOpenLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const viDisplay *display, viLayer *layer, u32 LayerFlags, u64 LayerId) {
    Result rc = 0;
    u64 AppletResourceUserId = 0;

    memset(layer, 0, sizeof(viLayer));

    if (LayerId==0) rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (LayerId==0 && (R_FAILED(rc) || AppletResourceUserId == 0)) {
        rc = _viCreateStrayLayer(NativeWindow, NativeWindow_Size, display, LayerFlags, &layer->LayerId);
    }
    else {
        if (LayerId==0) {
            rc = appletCreateManagedDisplayLayer(&LayerId);
            if (R_FAILED(rc)) return rc;
        }

        rc = _viOpenLayer(NativeWindow, NativeWindow_Size, display, LayerId, AppletResourceUserId);

        if (R_SUCCEEDED(rc)) layer->LayerId = LayerId;
    }

    if (R_SUCCEEDED(rc)) layer->initialized = 1;

    return rc;
}

Result viCloseLayer(viLayer *layer) {
    IpcCommand c;
    ipcInitialize(&c);

    if(!layer->initialized)return 0;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 LayerId;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = layer->StrayLayer==0 ? 2021 : 2031;
    raw->LayerId = layer->LayerId;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        memset(layer, 0, sizeof(viLayer));
    }

    return rc;
}

