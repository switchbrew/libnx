#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/miiimg.h"
#include "runtime/hosversion.h"

static Service g_miiimgSrv;

NX_GENERATE_SERVICE_GUARD(miiimg);

static Result _miiimgInitializeCmd(u8 in, u8 *out) {
    return serviceDispatchInOut(&g_miiimgSrv, 0, in, *out);
}

Result _miiimgInitialize(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    Result rc = smGetService(&g_miiimgSrv, "miiimg");
    if (R_SUCCEEDED(rc)) {
        u8 tmp;
        rc = _miiimgInitializeCmd(1, &tmp);
    }
    return rc;
}

void _miiimgCleanup(void) {
    serviceClose(&g_miiimgSrv);
}

Service* miiimgGetServiceSession(void) {
    return &g_miiimgSrv;
}

Result miiimgReload() {
    return serviceDispatch(&g_miiimgSrv, 10);
}

Result miiimgGetCount(u32 *out_count) {
    return serviceDispatchOut(&g_miiimgSrv, 11, *out_count);
}

Result miiimgIsEmpty(u8 *out_empty) {
    return serviceDispatchOut(&g_miiimgSrv, 12, *out_empty);
}

Result miiimgIsFull(u8 *out_full) {
    return serviceDispatchOut(&g_miiimgSrv, 13, *out_full);
}

Result miiimgGetAttribute(u32 index, MiiimgImageAttribute *out_attr) {
    return serviceDispatchInOut(&g_miiimgSrv, 14, index, *out_attr);
}

Result miiimgLoadImage(MiiimgImageId id, void *out_image, size_t out_image_size) {
    return serviceDispatchIn(&g_miiimgSrv, 15, id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_image, out_image_size } },
    );
}
