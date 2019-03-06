#include <string.h>
#include "types.h"
#include "result.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/vi.h"
#include "display/parcel.h"

__attribute__((weak)) u64 __nx_vi_layer_id = 0;
__attribute__((weak)) ViLayerFlags __nx_vi_stray_layer_flags = ViLayerFlags_Default;

static ViServiceType g_viServiceType = ViServiceType_Default;

static Service g_viIApplicationDisplayService;
static Service g_viIHOSBinderDriverRelay;
static Service g_viISystemDisplayService;
static Service g_viIManagerDisplayService;
static Service g_viIHOSBinderDriverIndirect;

static Result _viGetSession(Service* srv, Service* srv_out, void* inraw, size_t rawsize);
static Result _viGetSessionNoParams(Service* srv, Service* srv_out, u64 cmd_id);

Result viInitialize(ViServiceType service_type)
{
    if (serviceIsActive(&g_viIApplicationDisplayService))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    Service root_srv;
    Result rc = 0;

    if (service_type == ViServiceType_Default || service_type == ViServiceType_Manager) {
        rc = smGetService(&root_srv, "vi:m");
        g_viServiceType = ViServiceType_Manager;
    }

    if ((service_type == ViServiceType_Default && R_FAILED(rc)) || service_type == ViServiceType_System) {
        rc = smGetService(&root_srv, "vi:s");
        g_viServiceType = ViServiceType_System;
    }

    if ((service_type == ViServiceType_Default && R_FAILED(rc)) || service_type == ViServiceType_Application) {
        rc = smGetService(&root_srv, "vi:u");
        g_viServiceType = ViServiceType_Application;
    }

    if (R_SUCCEEDED(rc)) {
        struct {
            u64 magic;
            u64 cmd_id;
            u64 inval0;
        } raw;

        raw.magic = SFCI_MAGIC;
        raw.cmd_id = g_viServiceType; // ViServiceType matches the cmdid needed to open an IApplicationDisplayService session.
        raw.inval0 = 0;

        rc = _viGetSession(&root_srv, &g_viIApplicationDisplayService, &raw, sizeof(raw));
        serviceClose(&root_srv);
    }

    if (R_SUCCEEDED(rc))
        rc = _viGetSessionNoParams(&g_viIApplicationDisplayService, &g_viIHOSBinderDriverRelay, 100);

    if (g_viServiceType >= ViServiceType_System && R_SUCCEEDED(rc))
        rc = _viGetSessionNoParams(&g_viIApplicationDisplayService, &g_viISystemDisplayService, 101);

    if (g_viServiceType >= ViServiceType_Manager && R_SUCCEEDED(rc))
        rc = _viGetSessionNoParams(&g_viIApplicationDisplayService, &g_viIManagerDisplayService, 102);

    if (g_viServiceType >= ViServiceType_System && R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0))
        rc = _viGetSessionNoParams(&g_viIApplicationDisplayService, &g_viIHOSBinderDriverIndirect, 103);

    if (R_FAILED(rc))
        viExit();

    return rc;
}

void viExit(void)
{
    serviceClose(&g_viIHOSBinderDriverIndirect);
    serviceClose(&g_viIManagerDisplayService);
    serviceClose(&g_viISystemDisplayService);
    serviceClose(&g_viIHOSBinderDriverRelay);
    serviceClose(&g_viIApplicationDisplayService);
    g_viServiceType = ViServiceType_Default;
}

Service* viGetSession_IApplicationDisplayService(void)
{
    return &g_viIApplicationDisplayService;
}

Service* viGetSession_IHOSBinderDriverRelay(void)
{
    return &g_viIHOSBinderDriverRelay;
}

Service* viGetSession_ISystemDisplayService(void)
{
    return &g_viISystemDisplayService;
}

Service* viGetSession_IManagerDisplayService(void)
{
    return &g_viIManagerDisplayService;
}

Service* viGetSession_IHOSBinderDriverIndirect(void)
{
    return &g_viIHOSBinderDriverIndirect;
}

static Result _viGetSession(Service* srv, Service* srv_out, void* inraw, size_t rawsize)
{
    IpcCommand c;
    ipcInitialize(&c);

    memcpy(ipcPrepareHeader(&c, rawsize), inraw, rawsize);

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
            serviceCreate(srv_out, r.Handles[0]);
        }
    }

    return rc;
}

static Result _viGetSessionNoParams(Service* srv, Service* srv_out, u64 cmd_id)
{
    struct {
        u64 magic;
        u64 cmd_id;
    } raw;

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = cmd_id;

    return _viGetSession(srv, srv_out, &raw, sizeof(raw));
}

Result viOpenDisplay(const char *display_name, ViDisplay *display)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        char display_name[0x40];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1010;

    memset(display, 0, sizeof(ViDisplay));
    strncpy(display->display_name, display_name, sizeof(display->display_name)-1);
    memcpy(raw->display_name, display->display_name, sizeof(display->display_name));

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

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
            display->initialized = true;
        }
    }

    return rc;
}

Result viCloseDisplay(ViDisplay *display)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1020;
    raw->display_id = display->display_id;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
        memset(display, 0, sizeof(ViDisplay));
    }

    return rc;
}

Result viGetDisplayResolution(ViDisplay *display, u64 *width, u64 *height)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1102;
    raw->display_id = display->display_id;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

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

Result viGetDisplayLogicalResolution(ViDisplay *display, u32 *width, u32 *height)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1203;
    raw->display_id = display->display_id;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 width;
            u32 height;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (width) *width = resp->width;
            if (height) *height = resp->height;
        }
    }

    return rc;
}

Result viSetDisplayMagnification(ViDisplay *display, u32 x, u32 y, u32 width, u32 height)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 x;
        u32 y;
        u32 width;
        u32 height;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1204;
    raw->display_id = display->display_id;
    raw->x = x;
    raw->y = y;
    raw->width = width;
    raw->height = height;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

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

Result viGetDisplayVsyncEvent(ViDisplay *display, Event *event_out)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5202;
    raw->display_id = display->display_id;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event_out, r.Handles[0], true);
        }
    }

    return rc;
}

Result viSetDisplayPowerState(ViDisplay *display, ViPowerState state)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 power_state;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4205;
    raw->display_id = display->display_id;
    raw->power_state = state;

    Result rc = serviceIpcDispatch(&g_viIManagerDisplayService);

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

Result viSetDisplayAlpha(ViDisplay *display, float alpha)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        float alpha;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4201;
    raw->display_id = display->display_id;
    raw->alpha = alpha;

    Result rc = serviceIpcDispatch(&g_viIManagerDisplayService);

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

Result viGetDisplayMinimumZ(ViDisplay *display, u64 *z) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1200;
    raw->display_id = display->display_id;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 z;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (z) *z = resp->z;
        }
    }

    return rc;
}

Result viGetDisplayMaximumZ(ViDisplay *display, u64 *z) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 display_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1202;
    raw->display_id = display->display_id;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 z;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (z) *z = resp->z;
        }
    }

    return rc;
}

Result viCreateManagedLayer(const ViDisplay *display, ViLayerFlags layer_flags, u64 aruid, u64 *layer_id)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 layer_flags;
        u32 pad;
        u64 display_id;
        u64 aruid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2010;
    raw->layer_flags = layer_flags;
    raw->pad = 0;
    raw->display_id = display->display_id;
    raw->aruid = aruid;

    Result rc = serviceIpcDispatch(&g_viIManagerDisplayService);

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

Result viSetContentVisibility(bool v) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 visibility;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7000;
    raw->visibility = v;

    Result rc = serviceIpcDispatch(&g_viIManagerDisplayService);

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

static Result _viOpenLayer(const ViDisplay *display, u64 layer_id, u64 aruid, u8 native_window[0x100], u64 *native_window_size)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        char display_name[0x40];
        u64 layer_id;
        u64 aruid;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, native_window, 0x100, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2020;

    memcpy(raw->display_name, display->display_name, sizeof(display->display_name));

    raw->layer_id = layer_id;
    raw->aruid = aruid;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 native_window_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *native_window_size = resp->native_window_size;
        }
    }

    return rc;
}

static Result _viCreateStrayLayer(const ViDisplay *display, u32 layer_flags, u64 *layer_id, u8 native_window[0x100], u64 *native_window_size)
{
    Service* target_srv = &g_viIApplicationDisplayService;
    bool sysver_flag = hosversionBefore(7,0,0);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 layer_flags;
        u32 pad;
        u64 display_id;
    } *raw;

    ipcAddRecvBuffer(&c, native_window, 0x100, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;

    if (g_viServiceType < ViServiceType_System) {
        raw->cmd_id = 2030;
        target_srv = &g_viIApplicationDisplayService;
    }
    else {
        if (sysver_flag) {
            raw->cmd_id = 2312;
            target_srv = &g_viISystemDisplayService;
        }
        else {
            raw->cmd_id = 2012;
            target_srv = &g_viIManagerDisplayService;
        }
    }

    raw->layer_flags = layer_flags;
    raw->pad = 0;
    raw->display_id = display->display_id;
    
    Result rc=0;
    if (!serviceIsActive(target_srv)) rc = MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (R_SUCCEEDED(rc)) rc = serviceIpcDispatch(target_srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 layer_id;
            u64 native_window_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *layer_id = resp->layer_id;
            *native_window_size = resp->native_window_size;
        }
    }

    return rc;
}

Result viCreateLayer(const ViDisplay *display, ViLayer *layer)
{
    alignas(8) u8 native_window_raw[0x100];
    u64 native_window_size = 0;
    Result rc = 0;

    if (!display->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u64 aruid = 0;
    appletGetAppletResourceUserId(&aruid); // failure is not fatal

    memset(layer, 0, sizeof(ViLayer));
    layer->layer_id = __nx_vi_layer_id;

    if (!layer->layer_id && aruid) {
        rc = appletCreateManagedDisplayLayer(&layer->layer_id);
        if (R_FAILED(rc)) return rc;
    }

    if (layer->layer_id) {
        rc = _viOpenLayer(display, layer->layer_id, aruid, native_window_raw, &native_window_size);
    } else {
        layer->stray_layer = true;
        rc = _viCreateStrayLayer(display, __nx_vi_stray_layer_flags, &layer->layer_id, native_window_raw, &native_window_size);
    }

    if (R_SUCCEEDED(rc)) {
        layer->initialized = true;

        // Parse the parcel and get the binder id
        ParcelHeader* hdr = (ParcelHeader*)native_window_raw;
        if (hdr->payload_off > native_window_size)
            goto _bad_parcel;
        if ((hdr->payload_off+hdr->payload_size) > native_window_size)
            goto _bad_parcel;
        if (hdr->payload_size < 3*4)
            goto _bad_parcel;

        // Get the IGraphicBufferProducer binder object id
        u32* payload = (u32*)&native_window_raw[hdr->payload_off];
        layer->igbp_binder_obj_id = payload[2];
    }

    return rc;

_bad_parcel:
    viCloseLayer(layer);
    return MAKERESULT(Module_Libnx, LibnxError_BadInput);
}

Result viSetLayerSize(ViLayer *layer, u64 width, u64 height) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!layer->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 layer_id;
        u64 width;
        u64 height;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2203;
    raw->layer_id = layer->layer_id;
    raw->width = width;
    raw->height = height;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

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

Result viSetLayerZ(ViLayer *layer, u64 z) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!layer->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 layer_id;
        u64 z;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2205;
    raw->layer_id = layer->layer_id;
    raw->z = z;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

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

Result viSetLayerPosition(ViLayer *layer, float x, float y) {
    IpcCommand c;
    ipcInitialize(&c);

    if (!layer->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        float x;
        float y;
        u64 layer_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2201;
    raw->layer_id = layer->layer_id;
    raw->x = x;
    raw->y = y;

    Result rc = serviceIpcDispatch(&g_viISystemDisplayService);

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

Result viCloseLayer(ViLayer *layer)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!layer->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 layer_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = layer->stray_layer ? 2021 : 2031;
    raw->layer_id = layer->layer_id;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

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

Result viSetLayerScalingMode(ViLayer *layer, ViScalingMode scaling_mode)
{
    IpcCommand c;
    ipcInitialize(&c);

    if (!layer->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 scaling_mode;
        u32 pad;
        u64 layer_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2101;
    raw->scaling_mode = scaling_mode;
    raw->pad = 0;
    raw->layer_id = layer->layer_id;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

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

Result viGetIndirectLayerImageMap(void* buffer, size_t size, s32 width, s32 height, u64 IndirectLayerConsumerHandle, u64 *out0, u64 *out1)
{
    IpcCommand c;
    ipcInitialize(&c);

    u64 aruid = 0;
    Result rc = appletGetAppletResourceUserId(&aruid);
    if (R_FAILED(rc)) return rc;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 width;
        u64 height;
        u64 IndirectLayerConsumerHandle;
        u64 aruid;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, buffer, size, BufferType_Type1);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2450;
    raw->width = width;
    raw->height = height;
    raw->IndirectLayerConsumerHandle = IndirectLayerConsumerHandle;
    raw->aruid = aruid;

    rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 out0;
            u64 out1;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out0) *out0 = resp->out0;
            if (out1) *out1 = resp->out1;
        }
    }

    return rc;
}

Result viGetIndirectLayerImageRequiredMemoryInfo(s32 width, s32 height, u64 *out_size, u64 *out_alignment)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 width;
        u64 height;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2460;
    raw->width = width;
    raw->height = height;

    Result rc = serviceIpcDispatch(&g_viIApplicationDisplayService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 out_size;
            u64 out_alignment;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_size) *out_size = resp->out_size;
            if (out_alignment) *out_alignment = resp->out_alignment;
        }
    }

    return rc;
}

