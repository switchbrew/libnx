#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/lm.h"

static Service g_lmSrv;
static Service g_lmLoggerSrv;

static Result _lmOpenLogger(Service* srv_out);

NX_GENERATE_SERVICE_GUARD(lm);

Result _lmInitialize(void) {
    Result rc = smGetService(&g_lmSrv, "lm");

    // Open logger interface.
    if (R_SUCCEEDED(rc)) {
        rc = _lmOpenLogger(&g_lmLoggerSrv);
    }

    return rc;
}

void _lmCleanup(void) {
    serviceClose(&g_lmLoggerSrv);
    serviceClose(&g_lmSrv);
}

Service* lmGetServiceSession(void) {
    return &g_lmSrv;    
}

Service* lmGetServiceSession_Logger(void) {
    return &g_lmLoggerSrv;
}

static Result _lmOpenLogger(Service* srv_out) {
    u64 pid_placeholder = 0;
    return serviceDispatchIn(&g_lmSrv, 0, pid_placeholder,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result lmLog(const void *buf, size_t buf_size) {
    return serviceDispatch(&g_lmLoggerSrv, 0,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result lmSetDestination(LmLogDestination destination) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 in = (u32)destination;
    return serviceDispatchIn(&g_lmLoggerSrv, 1, in);
}
