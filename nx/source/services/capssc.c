#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/capssc.h"

static Service g_capsscSrv;

NX_GENERATE_SERVICE_GUARD(capssc);

Result _capsscInitialize(void) {
    Result rc=0;

    if (hosversionBefore(2,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capsscSrv, "caps:sc");

    return rc;
}

void _capsscCleanup(void) {
    serviceClose(&g_capsscSrv);
}

Service* capsscGetServiceSession(void) {
    return &g_capsscSrv;
}

Result capsscCaptureRawImageWithTimeout(void* buf, size_t size, ViLayerStack layer_stack, u64 width, u64 height, s64 buffer_count, s64 buffer_index, s64 timeout) {
    const struct {
        s32 layer_stack;
        u32 pad;
        u64 width;
        u64 height;
        s64 buffer_count;
        s64 buffer_index;
        s64 timeout;
    } in = { layer_stack, 0, width, height, buffer_count, buffer_index, timeout };

    return serviceDispatchIn(&g_capsscSrv, 2, in,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

Result capsscOpenRawScreenShotReadStream(u64 *out_size, u64 *out_width, u64 *out_height, ViLayerStack layer_stack, s64 timeout) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 layer_stack;
        u32 pad;
        u64 timeout;
    } in = {layer_stack, 0, timeout};

    struct {
        u64 size;
        u64 width;
        u64 height;
    } out;

    Result rc = serviceDispatchInOut(&g_capsscSrv, 1201, in, out);

    if (R_SUCCEEDED(rc)) {
        if (out_size) *out_size = out.size;
        if (out_width) *out_width = out.width;
        if (out_height) *out_height = out.height;
    }
    return rc;
}

Result capsscCloseRawScreenShotReadStream(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_capsscSrv, 1202);
}

Result capsscReadRawScreenShotReadStream(u64 *bytes_read, void* buf, size_t size, u64 offset) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_capsscSrv, 1203, offset, *bytes_read,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { buf, size } },
    );
}

Result capsscCaptureJpegScreenShot(u64* out_jpeg_size, void* jpeg_buf, size_t jpeg_buf_size, ViLayerStack layer_stack, s64 timeout) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 layer_stack;
        u32 pad;
        s64 timeout;
    } in = { layer_stack, 0, timeout };

    return serviceDispatchInOut(&g_capsscSrv, 1204, in, *out_jpeg_size,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { jpeg_buf, jpeg_buf_size } },
    );
}