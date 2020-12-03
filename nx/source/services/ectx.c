#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ectx.h"

static Service g_ectxrSrv;

NX_GENERATE_SERVICE_GUARD(ectxr);

Result _ectxrInitialize(void) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return smGetService(&g_ectxrSrv, "ectx:r");
}

void _ectxrCleanup(void) {
    serviceClose(&g_ectxrSrv);
}

Service* ectxrGetServiceSession(void) {
    return &g_ectxrSrv;
}

Result ectxrPullContext(s32 *out0, u32 *out_total_size, u32 *out_size, void *dst, size_t dst_size, u32 descriptor, Result result) {
    const struct {
        u32 desc;
        u32 res;
    } in = { descriptor, result };

    struct {
        s32 unk0;
        u32 total_size;
        u32 size;
    } out;

    Result rc = serviceDispatchInOut(&g_ectxrSrv, 1, in, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { dst, dst_size } },
    );

    if (R_SUCCEEDED(rc)) {
        if (out0) *out0 = out.unk0;
        if (out_total_size) *out_total_size = out.total_size;
        if (out_size) *out_size = out.size;
    }

    return rc;
}
