#include <string.h>
#include <switch.h>

static Handle g_viServiceSession = INVALID_HANDLE;
static u32 g_viServiceType = -1;
static Handle g_viIApplicationDisplayService = INVALID_HANDLE;
static Handle g_viIHOSBinderDriverRelay = INVALID_HANDLE;
static Handle g_viISystemDisplayService = INVALID_HANDLE;
static Handle g_viIManagerDisplayService = INVALID_HANDLE;
static Handle g_viIHOSBinderDriverIndirect = INVALID_HANDLE;

static Result _viGetSession(Handle sessionhandle, Handle* handle_out, void* inraw, size_t rawsize);
static Result _viGetSessionNoParams(Handle sessionhandle, Handle* handle_out, u64 cmd_id);

Result viInitialize(ViServiceType servicetype)
{
    if(g_viServiceType != -1)
        return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;

    if (servicetype==ViServiceType_Default || servicetype==ViServiceType_Manager) {
        rc = smGetService(&g_viServiceSession, "vi:m");
        g_viServiceType = 2;
    }

    if ((servicetype==ViServiceType_Default && R_FAILED(rc)) || servicetype==ViServiceType_System) {
        rc = smGetService(&g_viServiceSession, "vi:s");
        g_viServiceType = 1;
    }

    if ((servicetype==ViServiceType_Default && R_FAILED(rc)) || servicetype==ViServiceType_Application)
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

    if (R_SUCCEEDED(rc))
        rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIHOSBinderDriverRelay, 100);

    if (g_viServiceType>=ViServiceType_System && R_SUCCEEDED(rc))
        rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viISystemDisplayService, 101);

    if (g_viServiceType>=ViServiceType_Manager && R_SUCCEEDED(rc))
        rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIManagerDisplayService, 102);

    if (g_viServiceType>=ViServiceType_System && R_SUCCEEDED(rc) && kernelAbove200())
        rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIHOSBinderDriverIndirect, 103);

    if (R_FAILED(rc)) {
        viExit();
        g_viServiceType = -1;
    }

    return rc;
}

void viExit(void)
{
    if(g_viServiceType == -1)
        return;

    g_viServiceType = -1;

    if(g_viServiceSession != INVALID_HANDLE) {
        svcCloseHandle(g_viServiceSession);
        g_viServiceSession = 0;
    }

    if(g_viIApplicationDisplayService != INVALID_HANDLE) {
        svcCloseHandle(g_viIApplicationDisplayService);
        g_viIApplicationDisplayService = 0;
    }

    if(g_viISystemDisplayService != INVALID_HANDLE) {
        svcCloseHandle(g_viISystemDisplayService);
        g_viISystemDisplayService = 0;
    }

    if(g_viIManagerDisplayService != INVALID_HANDLE) {
        svcCloseHandle(g_viIManagerDisplayService);
        g_viIManagerDisplayService = 0;
    }

    if(g_viIHOSBinderDriverIndirect != INVALID_HANDLE) {
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

static Result _viGetSessionNoParams(Handle sessionhandle, Handle* handle_out, u64 cmd_id) {
    struct {
        u64 magic;
        u64 cmd_id;
    } raw;

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = cmd_id;

    return _viGetSession(sessionhandle, handle_out, &raw, sizeof(raw));
}

Result viOpenDisplay(const char *display_name, ViDisplay *display) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        char display_name[0x40];
    } *raw;

    memset(display, 0, sizeof(ViDisplay));

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1010;

    strncpy(display->display_name, display_name, sizeof(display->display_name)-1);
    memcpy(raw->display_name, display->display_name, sizeof(display->display_name));

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 display_id;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            display->display_id = resp->display_id;
            display->initialized = 1;
        }
    }

    return rc;
}

Result viCloseDisplay(ViDisplay *display) {
    IpcCommand c;
    ipcInitialize(&c);

    if(!display->initialized)return 0;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1020;
    raw->display_id = display->display_id;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        display->initialized = 0;
    }

    return rc;
}

Result viCreateManagedLayer(const ViDisplay *display, u32 LayerFlags, u64 AppletResourceUserId, u64 *layer_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 LayerFlags;
        u32 pad;
        u64 display_id;
        u64 AppletResourceUserId;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2010;
    raw->LayerFlags = LayerFlags;
    raw->pad = 0;
    raw->display_id = display->display_id;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = ipcDispatch(g_viIManagerDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 layer_id;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *layer_id = resp->layer_id;
        }
    }

    return rc;
}

static Result _viOpenLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const ViDisplay *display, u64 layer_id, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        char display_name[0x40];
        u64 layer_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, NativeWindow, 0x100, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2020;

    memcpy(raw->display_name, display->display_name, sizeof(display->display_name));

    raw->layer_id = layer_id;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

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

static Result _viCreatestray_layer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const ViDisplay *display, u32 LayerFlags, u64 *layer_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 LayerFlags;
        u32 pad;
        u64 display_id;
    } *raw;

    ipcAddRecvBuffer(&c, NativeWindow, 0x100, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2030;
    raw->LayerFlags = LayerFlags;
    raw->pad = 0;
    raw->display_id = display->display_id;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 layer_id;
            u64 NativeWindow_Size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *layer_id = resp->layer_id;
            *NativeWindow_Size = resp->NativeWindow_Size;
        }
    }

    return rc;
}

Result viOpenLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const ViDisplay *display, ViLayer *layer, u32 LayerFlags, u64 layer_id) {
    Result rc = 0;
    u64 AppletResourceUserId = 0;

    memset(layer, 0, sizeof(ViLayer));

    if (layer_id==0) rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (layer_id==0 && (R_FAILED(rc) || AppletResourceUserId == 0)) {
        rc = _viCreatestray_layer(NativeWindow, NativeWindow_Size, display, LayerFlags, &layer->layer_id);
        if (R_SUCCEEDED(rc)) layer->stray_layer = 1;
    }
    else {
        if (layer_id==0) {
            rc = appletCreateManagedDisplayLayer(&layer_id);

            if (R_FAILED(rc)) return rc;
        }

        rc = _viOpenLayer(NativeWindow, NativeWindow_Size, display, layer_id, AppletResourceUserId);

        if (R_SUCCEEDED(rc)) layer->layer_id = layer_id;
    }

    if (R_SUCCEEDED(rc)) layer->initialized = 1;

    return rc;
}

Result viCloseLayer(ViLayer *layer) {
    IpcCommand c;
    ipcInitialize(&c);

    if(!layer->initialized)return 0;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 layer_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = layer->stray_layer == 0 ? 2021 : 2031;
    raw->layer_id = layer->layer_id;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        memset(layer, 0, sizeof(ViLayer));
    }

    return rc;
}

Result viSetLayerScalingMode(ViLayer *layer, u32 ScalingMode) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!layer->initialized) return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 ScalingMode;
        u32 pad;
        u64 layer_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2101;
    raw->ScalingMode = ScalingMode;
    raw->pad = 0;
    raw->layer_id = layer->layer_id;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

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

Result viGetDisplayResolution(ViDisplay *display, u64 *width, u64 *height) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1102;
    raw->display_id = display->display_id;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 width;
            u64 height;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (width) *width = resp->width;
            if (height) *height = resp->height;
        }
    }

    return rc;
}

Result viGetDisplayVsyncEvent(ViDisplay *display, Handle *handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5202;
    raw->display_id = display->display_id;

    Result rc = ipcDispatch(g_viIApplicationDisplayService);

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

