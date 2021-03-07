#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/mm.h"

static Service g_mmuSrv;

NX_GENERATE_SERVICE_GUARD(mmu);

Result _mmuInitialize(void) {
    return smGetService(&g_mmuSrv, "mm:u");
}

void _mmuCleanup(void) {
    serviceClose(&g_mmuSrv);
}

Result mmuRequestInitialize(MmuRequest *request, MmuModuleId module, u32 unk, bool autoclear) {
    struct {
        u32 module_id;
        u32 unk;
        u32 autoclear;
    } data = { module, unk, autoclear };
    u32 out = -1;

    Result rc = serviceDispatchInOut(&g_mmuSrv, 4, data, out);

    if (R_SUCCEEDED(rc)) {
        request->module = module;
        request->id     = out;
    }

    return rc;
}

Result mmuRequestFinalize(const MmuRequest *request) {
    return serviceDispatchIn(&g_mmuSrv, 5, request->id);
}

Result mmuRequestSetAndWait(const MmuRequest *request, u32 freq_hz, s32 timeout) {
    struct {
        u32 id;
        u32 freq;
        s32 timeout;
    } data = { request->id, freq_hz, timeout };
    return serviceDispatchIn(&g_mmuSrv, 6, data);
}

Result mmuRequestGet(const MmuRequest *request, u32 *out_freq_hz) {
    u32 freq = 0;
    Result rc = serviceDispatchInOut(&g_mmuSrv, 7, request->id, freq);

    if (R_SUCCEEDED(rc) && out_freq_hz)
        *out_freq_hz = freq;

    return rc;
}
