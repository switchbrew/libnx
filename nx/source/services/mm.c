#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/mm.h"
#include "runtime/hosversion.h"

static Service g_mmuSrv;

NX_GENERATE_SERVICE_GUARD(mmu);

Result _mmuInitialize(void) {
    return smGetService(&g_mmuSrv, "mm:u");
}

void _mmuCleanup(void) {
    serviceClose(&g_mmuSrv);
}

Service* mmuGetServiceSession(void) {
    return &g_mmuSrv;
}

Result mmuRequestInitialize(MmuRequest *request, MmuModuleId module, u32 unk, bool autoclear) {
    struct {
        u32 module_id;
        u32 unk;
        u32 autoclear;
    } data = { module, unk, autoclear };
    u32 out = -1;

    Result rc = serviceDispatchInOut(&g_mmuSrv, hosversionBefore(2,0,0) ? 0 : 4, data, out);

    if (R_SUCCEEDED(rc)) {
        request->module = module;
        request->id     = out;
    }

    return rc;
}

Result mmuRequestFinalize(const MmuRequest *request) {
    bool need_old = hosversionBefore(2,0,0);
    return serviceDispatchIn(&g_mmuSrv, need_old ? 1 : 5, *(need_old ? &request->module : &request->id));
}

Result mmuRequestSetAndWait(const MmuRequest *request, u32 freq_hz, s32 timeout) {
    bool need_old = hosversionBefore(2,0,0);

    struct {
        u32 id;
        u32 freq;
        s32 timeout;
    } data = { need_old ? request->module : request->id, freq_hz, timeout };
    return serviceDispatchIn(&g_mmuSrv, need_old ? 2 : 6, data);
}

Result mmuRequestGet(const MmuRequest *request, u32 *out_freq_hz) {
    bool need_old = hosversionBefore(2,0,0);

    u32 freq = 0;
    Result rc = serviceDispatchInOut(&g_mmuSrv, need_old ? 3 : 7, *(need_old ? &request->module : &request->id), freq);

    if (R_SUCCEEDED(rc) && out_freq_hz)
        *out_freq_hz = freq;

    return rc;
}
