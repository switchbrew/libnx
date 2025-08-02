#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/applet.h"
#include "services/vi.h"
#include "display/parcel.h"
#include "runtime/hosversion.h"

__attribute__((weak)) u64 __nx_vi_layer_id = 0;
__attribute__((weak)) ViLayerFlags __nx_vi_stray_layer_flags = ViLayerFlags_Default;

static ViServiceType g_viServiceType = ViServiceType_Default;

static Service g_viRootService;
static Service g_viIApplicationDisplayService;
static Service g_viIHOSBinderDriverRelay;
static Service g_viISystemDisplayService;
static Service g_viIManagerDisplayService;
static Service g_viIHOSBinderDriverIndirect;

static Result _viCmdGetSession(Service* srv, Service* srv_out, u32 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _viCmdGetSessionNoParams(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

NX_GENERATE_SERVICE_GUARD_PARAMS(vi, (ViServiceType service_type), (service_type));

Result _viInitialize(ViServiceType service_type) {
    Service root_srv;
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

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
        const u32 inval = (g_viServiceType == ViServiceType_Manager || g_viServiceType == ViServiceType_System) ? 1 : 0;
        rc = _viCmdGetSession(&root_srv, &g_viIApplicationDisplayService, inval, g_viServiceType);

        if (!(hosversionAtLeast(16,0,0) && g_viServiceType == ViServiceType_Manager)) {
            serviceClose(&root_srv);
        } else {
            g_viRootService = root_srv;
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = _viCmdGetSessionNoParams(&g_viIApplicationDisplayService, &g_viIHOSBinderDriverRelay, 100);
    }

    if (g_viServiceType >= ViServiceType_System && R_SUCCEEDED(rc)) {
        rc = _viCmdGetSessionNoParams(&g_viIApplicationDisplayService, &g_viISystemDisplayService, 101);
    }

    if (g_viServiceType >= ViServiceType_Manager && R_SUCCEEDED(rc)) {
        rc = _viCmdGetSessionNoParams(&g_viIApplicationDisplayService, &g_viIManagerDisplayService, 102);
    }

    if (g_viServiceType >= ViServiceType_System && R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0)) {
        rc = _viCmdGetSessionNoParams(&g_viIApplicationDisplayService, &g_viIHOSBinderDriverIndirect, 103);
    }

    return rc;
}

void _viCleanup(void) {
    serviceClose(&g_viIHOSBinderDriverIndirect);
    serviceClose(&g_viIManagerDisplayService);
    serviceClose(&g_viISystemDisplayService);
    serviceClose(&g_viIHOSBinderDriverRelay);
    serviceClose(&g_viIApplicationDisplayService);
    if (hosversionAtLeast(16,0,0) && g_viServiceType == ViServiceType_Manager) {
        serviceClose(&g_viRootService);
    }
    g_viServiceType = ViServiceType_Default;
}

Service* viGetSession_IApplicationDisplayService(void) {
    return &g_viIApplicationDisplayService;
}

Service* viGetSession_IHOSBinderDriverRelay(void) {
    return &g_viIHOSBinderDriverRelay;
}

Service* viGetSession_ISystemDisplayService(void) {
    return &g_viISystemDisplayService;
}

Service* viGetSession_IManagerDisplayService(void) {
    return &g_viIManagerDisplayService;
}

Service* viGetSession_IHOSBinderDriverIndirect(void) {
    return &g_viIHOSBinderDriverIndirect;
}

Result viOpenDisplay(const char *display_name, ViDisplay *display) {
    memset(display, 0, sizeof(*display));
    strncpy(display->display_name.data, display_name, sizeof(display->display_name) - 1);

    Result rc = serviceDispatchInOut(&g_viIApplicationDisplayService, 1010, display->display_name, display->display_id);

    if (R_SUCCEEDED(rc)) {
        display->initialized = true;
    }

    return rc;
}

Result viCloseDisplay(ViDisplay *display) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    Result rc = serviceDispatchIn(&g_viIApplicationDisplayService, 1020, display->display_id);

    if (R_SUCCEEDED(rc)) {
        memset(display, 0, sizeof(*display));
    }

    return rc;
}

Result viGetDisplayResolution(ViDisplay *display, s32 *width, s32 *height) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    struct {
        s64 width;
        s64 height;
    } out;

    Result rc = serviceDispatchInOut(&g_viIApplicationDisplayService, 1102, display->display_id, out);

    if (R_SUCCEEDED(rc)) {
        if (width)  *width  = out.width;
        if (height) *height = out.height;
    }

    return rc;
}

Result viGetDisplayLogicalResolution(ViDisplay *display, s32 *width, s32 *height) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    struct {
        s32 width;
        s32 height;
    } out;

    Result rc = serviceDispatchInOut(&g_viISystemDisplayService, 1203, display->display_id, out);

    if (R_SUCCEEDED(rc)) {
        if (width)  *width  = out.width;
        if (height) *height = out.height;
    }

    return rc;

}

Result viSetDisplayMagnification(ViDisplay *display, s32 x, s32 y, s32 width, s32 height) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }
    if (hosversionBefore(3,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    const struct {
        s32 x;
        s32 y;
        s32 width;
        s32 height;
        u64 display_id;
    } in = { x, y, width, height, display->display_id };
    return serviceDispatchIn(&g_viISystemDisplayService, 1204, in);
}

Result viGetDisplayVsyncEvent(ViDisplay *display, Event *event_out) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    Handle evt_handle;
    Result rc = serviceDispatchIn(&g_viIApplicationDisplayService, 5202, display->display_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &evt_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(event_out, evt_handle, true);
    }

    return rc;
}

Result viSetDisplayPowerState(ViDisplay *display, ViPowerState state) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        u32 power_state;
        u64 display_id;
    } in = { state, display->display_id };
    return serviceDispatchIn(&g_viIManagerDisplayService, 4205, in);
}

Result viSetDisplayAlpha(ViDisplay *display, float alpha) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        float alpha;
        u64 display_id;
    } in = { alpha, display->display_id };
    return serviceDispatchIn(&g_viIManagerDisplayService, 4201, in);
}

Result viGetZOrderCountMin(ViDisplay *display, s32 *z) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    s64 tmp=0;
    Result rc = serviceDispatchInOut(&g_viISystemDisplayService, 1200, display->display_id, tmp);
    if (R_SUCCEEDED(rc) && z) *z = tmp;
    return rc;
}

Result viGetZOrderCountMax(ViDisplay *display, s32 *z) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    s64 tmp=0;
    Result rc = serviceDispatchInOut(&g_viISystemDisplayService, 1202, display->display_id, tmp);
    if (R_SUCCEEDED(rc) && z) *z = tmp;
    return rc;
}

Result viCreateManagedLayer(const ViDisplay *display, ViLayerFlags layer_flags, u64 aruid, u64 *layer_id) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        u32 layer_flags;
        u32 pad;
        u64 display_id;
        u64 aruid;
    } in = { layer_flags, 0, display->display_id, aruid };
    return serviceDispatchInOut(&g_viIManagerDisplayService, 2010, in, *layer_id);
}

Result viDestroyManagedLayer(ViLayer *layer) {
    Result rc = serviceDispatchIn(&g_viIManagerDisplayService, 2011, layer->layer_id);

    if (R_SUCCEEDED(rc)) {
        memset(layer, 0, sizeof(*layer));
    }

    return rc;
}

Result viSetContentVisibility(bool v) {
    u8 tmp = v!=0;
    return serviceDispatchIn(&g_viIManagerDisplayService, 7000, tmp);
}

static Result _viOpenLayer(const ViDisplay *display, u64 layer_id, u8 native_window[0x100], u64 *native_window_size) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        ViDisplayName display_name;
        u64 layer_id;
        u64 aruid;
    } in = { display->display_name, layer_id, appletGetAppletResourceUserId() };
    return serviceDispatchInOut(&g_viIApplicationDisplayService, 2020, in, *native_window_size,
        .in_send_pid = true,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { native_window, 0x100 } },
    );
}

static Result _viCreateStrayLayer(const ViDisplay *display, u32 layer_flags, u64 *layer_id, u8 native_window[0x100], u64 *native_window_size) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    Service* target_srv;
    u32 cmd_id;

    if (g_viServiceType < ViServiceType_System) {
        target_srv = &g_viIApplicationDisplayService;
        cmd_id = 2030;
    } else {
        if (hosversionBefore(7,0,0)) {
            target_srv = &g_viISystemDisplayService;
            cmd_id = 2312;
        } else {
            target_srv = &g_viIManagerDisplayService;
            cmd_id = 2012;
        }
    }

    if (!serviceIsActive(target_srv)) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        u32 layer_flags;
        u32 pad;
        u64 display_id;
    } in = { layer_flags, 0, display->display_id };

    struct {
        u64 layer_id;
        u64 native_window_size;
    } out;

    Result rc = serviceDispatchInOut(target_srv, cmd_id, in, out,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { native_window, 0x100 } },
    );

    if (R_SUCCEEDED(rc)) {
        *layer_id = out.layer_id;
        *native_window_size = out.native_window_size;
    }

    return rc;
}

Result viCreateLayer(const ViDisplay *display, ViLayer *layer) {
    if (!display->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    alignas(8) u8 native_window_raw[0x100];
    u64 native_window_size = 0;

    memset(layer, 0, sizeof(ViLayer));
    layer->layer_id = __nx_vi_layer_id;

    Result rc = 0;
    if (!layer->layer_id && appletGetAppletResourceUserId()) {
        rc = appletCreateManagedDisplayLayer(&layer->layer_id);
        if (R_FAILED(rc)) return rc;
    }

    if (layer->layer_id) {
        rc = _viOpenLayer(display, layer->layer_id, native_window_raw, &native_window_size);
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

Result viSetLayerSize(ViLayer *layer, s32 width, s32 height) {
    if (!layer->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        u64 layer_id;
        s64 width;
        s64 height;
    } in = { layer->layer_id, width, height };
    return serviceDispatchIn(&g_viISystemDisplayService, 2203, in);
}

Result viSetLayerZ(ViLayer *layer, s32 z) {
    if (!layer->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        u64 layer_id;
        s64 z;
    } in = { layer->layer_id, z };
    return serviceDispatchIn(&g_viISystemDisplayService, 2205, in);
}

Result viSetLayerPosition(ViLayer *layer, float x, float y) {
    if (!layer->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        float x;
        float y;
        u64 layer_id;
    } in = { x, y, layer->layer_id };
    return serviceDispatchIn(&g_viISystemDisplayService, 2201, in);
}

Result viCloseLayer(ViLayer *layer) {
    if (!layer->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    Result rc = serviceDispatchIn(&g_viIApplicationDisplayService, layer->stray_layer ? 2031 : 2021, layer->layer_id);

    if (R_SUCCEEDED(rc)) {
        memset(layer, 0, sizeof(*layer));
    }

    return rc;
}

Result viSetLayerScalingMode(ViLayer *layer, ViScalingMode scaling_mode) {
    if (!layer->initialized) {
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    }

    const struct {
        u32 scaling_mode;
        u32 pad;
        u64 layer_id;
    } in = { scaling_mode, 0, layer->layer_id };
    return serviceDispatchIn(&g_viIApplicationDisplayService, 2101, in);
}

Result viGetIndirectLayerImageMap(void* buffer, size_t size, s32 width, s32 height, u64 IndirectLayerConsumerHandle, u64 *out_size, u64 *out_stride) {
    Result rc = 0;

    const struct {
        s64 width;
        s64 height;
        u64 IndirectLayerConsumerHandle;
        u64 aruid;
    } in = { width, height, IndirectLayerConsumerHandle, appletGetAppletResourceUserId() };

    struct {
        s64 size;
        s64 stride;
    } out;

    rc = serviceDispatchInOut(&g_viIApplicationDisplayService, 2450, in, out,
        .in_send_pid = true,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buffer, size } },
    );

    if (R_SUCCEEDED(rc)) {
        if (out_size)   *out_size   = out.size;
        if (out_stride) *out_stride = out.stride;
    }

    return rc;
}

Result viGetIndirectLayerImageRequiredMemoryInfo(s32 width, s32 height, u64 *out_size, u64 *out_alignment) {
    const struct {
        s64 width;
        s64 height;
    } in = { width, height };

    struct {
        s64 size;
        s64 alignment;
    } out;

    Result rc = serviceDispatchInOut(&g_viIApplicationDisplayService, 2460, in, out);

    if (R_SUCCEEDED(rc)) {
        if (out_size)      *out_size      = out.size;
        if (out_alignment) *out_alignment = out.alignment;
    }

    return rc;
}

Result viManagerPrepareFatal(void) {
    if (g_viServiceType != ViServiceType_Manager)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_viRootService, 100);
}

Result viManagerShowFatal(void) {
    if (g_viServiceType != ViServiceType_Manager)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_viRootService, 101);
}

Result viManagerDrawFatalRectangle(s32 x, s32 y, s32 end_x, s32 end_y, ViColorRgba4444 color) {
    if (g_viServiceType != ViServiceType_Manager)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u16 color;
        s32 x;
        s32 y;
        s32 end_x;
        s32 end_y;
    } in = { color, x, y, end_x, end_y };
    return serviceDispatchIn(&g_viRootService, 102, in);
}

Result viManagerDrawFatalText32(s32 *out_advance, s32 x, s32 y, const u32 *utf32_codepoints, size_t num_codepoints, float scale_x, float scale_y, PlSharedFontType font_type, ViColorRgba8888 bg_color, ViColorRgba8888 fg_color, s32 initial_advance) {
    if (g_viServiceType != ViServiceType_Manager)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 x;
        s32 y;
        float scale_x;
        float scale_y;
        u32 font_type;
        u32 bg_color;
        u32 fg_color;
        s32 initial_advance;
    } in = { x, y, scale_x, scale_y, font_type, bg_color, fg_color, initial_advance };

    return serviceDispatchInOut(&g_viRootService, 103, in, *out_advance,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { utf32_codepoints, num_codepoints * sizeof(*utf32_codepoints) } },
    );
}
