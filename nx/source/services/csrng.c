#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/csrng.h"

static Service g_csrngSrv;

NX_GENERATE_SERVICE_GUARD(csrng);

Result _csrngInitialize(void) {
    return smGetService(&g_csrngSrv, "csrng");
}

void _csrngCleanup(void) {
    serviceClose(&g_csrngSrv);
}

Service* csrngGetServiceSession(void) {
    return &g_csrngSrv;
}

Result csrngGetRandomBytes(void *out, size_t out_size) {
    return serviceDispatch(&g_csrngSrv, 0,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, out_size } },
    );
}
