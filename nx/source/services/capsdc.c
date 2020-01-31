#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/capsdc.h"

static Service g_capsdcSrv;

NX_GENERATE_SERVICE_GUARD(capsdc);

Result _capsdcInitialize(void) {
    return smGetService(&g_capsdcSrv, "caps:dc");
}

void _capsdcCleanup(void) {
    serviceClose(&g_capsdcSrv);
}

Service* capsdcGetServiceSession(void) {
    return &g_capsdcSrv;
}

Result capsdcDecodeJpeg(const u32 width, const u32 height, const CapsScreenShotDecodeOption *opts, const void* jpeg, const u64 jpeg_size, void* image, const u64 image_size) {
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
            { image, image_size },
        }
    );
}
