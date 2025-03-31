#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/capsdc.h"

static Service g_capsdcSrv;

NX_GENERATE_SERVICE_GUARD(capsdc);

Result _capsdcInitialize(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_capsdcSrv, "caps:dc");
}

void _capsdcCleanup(void) {
    serviceClose(&g_capsdcSrv);
}

Service* capsdcGetServiceSession(void) {
    return &g_capsdcSrv;
}

Result capsdcDecodeJpeg(u32 width, u32 height, const CapsScreenShotDecodeOption *opts, const void* jpeg, size_t jpeg_size, void* out_image, size_t out_image_size) {
    const struct {
        u32 width;
        u32 height;
        CapsScreenShotDecodeOption opts;
    } in = { width, height, *opts };
    return serviceDispatchIn(&g_capsdcSrv, 3001, in,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_Out | SfBufferAttr_HipcMapAlias | SfBufferAttr_HipcMapTransferAllowsNonSecure,
        },
        .buffers = {
            { jpeg, jpeg_size },
            { out_image, out_image_size },
        }
    );
}

Result capsdcShrinkJpeg(u32 width, u32 height, const CapsScreenShotDecodeOption *opts, const void* jpeg, size_t jpeg_size, void* out_jpeg, size_t out_jpeg_size, u64 *out_result_size) {
    if (hosversionBefore(17,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 width;
        u32 height;
        CapsScreenShotDecodeOption opts;
    } in = { width, height, *opts };
    return serviceDispatchInOut(&g_capsdcSrv, 4001, in, *out_result_size,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_Out | SfBufferAttr_HipcMapAlias | SfBufferAttr_HipcMapTransferAllowsNonSecure,
        },
        .buffers = {
            { jpeg, jpeg_size },
            { out_jpeg, out_jpeg_size },
        }
    );
}

Result capsdcShrinkJpegEx(u32 scaled_width, u32 scaled_height, u32 jpeg_quality, const CapsScreenShotDecodeOption *opts, const void* jpeg, size_t jpeg_size, void* out_jpeg, size_t out_jpeg_size, u64 *out_result_size) {
    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 scaled_width;
        u32 scaled_height;
        u32 jpeg_quality;
        u8 pad[4];
        CapsScreenShotDecodeOption opts;
    } in = { scaled_width, scaled_height, jpeg_quality, {}, *opts };
    return serviceDispatchInOut(&g_capsdcSrv, 4002, in, *out_result_size,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_Out | SfBufferAttr_HipcMapAlias | SfBufferAttr_HipcMapTransferAllowsNonSecure,
        },
        .buffers = {
            { jpeg, jpeg_size },
            { out_jpeg, out_jpeg_size },
        }
    );
}
