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

Result capsscCaptureRawImageWithTimeout(void* buf, size_t size, u32 layer_stack, u64 width, u64 height, s64 buffer_count, s64 buffer_index, u64 timeout) {
    const struct {
        u32 layer_stack;
        u32 pad;
        u64 width;
        u64 height;
        s64 buffer_count;
        s64 buffer_index;
        u64 timeout;
    } in = { layer_stack, 0, width, height, buffer_count, buffer_index, timeout };

    return serviceDispatchIn(&g_capsscSrv, 2, in,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

