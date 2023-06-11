#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/clkrst.h"
#include "runtime/hosversion.h"

static Service g_clkrstSrv;

NX_GENERATE_SERVICE_GUARD(clkrst);

Result _clkrstInitialize(void) {
    if(hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_clkrstSrv, "clkrst");
}

void _clkrstCleanup(void) {
    serviceClose(&g_clkrstSrv);
}

Service* clkrstGetServiceSession(void) {
    return &g_clkrstSrv;
}

Result clkrstOpenSession(ClkrstSession* session_out, PcvModuleId module_id, u32 unk) {
    const struct {
        u32 module_id;
        u32 unk;
    } in = { module_id, unk };
    return serviceDispatchIn(&g_clkrstSrv, 0, in,
        .out_num_objects = 1,
        .out_objects = &session_out->s,
    );
}

void clkrstCloseSession(ClkrstSession* session) {
    serviceClose(&session->s);
}

Result clkrstSetClockRate(ClkrstSession* session, u32 hz) {
    return serviceDispatchIn(&session->s, 7, hz);
}

Result clkrstGetClockRate(ClkrstSession* session, u32 *out_hz) {
    return serviceDispatchOut(&session->s, 8, *out_hz);
}

Result clkrstGetPossibleClockRates(ClkrstSession *session, u32 *rates, s32 max_count, PcvClockRatesListType *out_type, s32 *out_count) {
    struct {
        s32 type;
        s32 count;
    } out;

    Result rc = serviceDispatchInOut(&session->s, 10, max_count, out,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect, },
        .buffers = { { rates, max_count * sizeof(u32) }, }
    );

    if (R_SUCCEEDED(rc) && out_type) *out_type = out.type;
    if (R_SUCCEEDED(rc) && out_count) *out_count = out.count;

    return rc;
}
