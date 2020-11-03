#include "service_guard.h"
#include "services/ins.h"

static Service g_insrSrv;
static Service g_inssSrv;

NX_GENERATE_SERVICE_GUARD(insr);

Result _insrInitialize(void) {
    return smGetService(&g_insrSrv, "ins:r");
}

void _insrCleanup(void) {
    serviceClose(&g_insrSrv);
}

Service* insrGetServiceSession(void) {
    return &g_insrSrv;
}

Result insrGetLastTick(u32 id, u64 *tick) {
    return serviceDispatchInOut(&g_insrSrv, 0, id, *tick);
}

Result insrGetReadableEvent(u32 id, Event *out) {
    Handle tmp_handle;
    struct {
        u32 id;
        u64 unk;
    } in = { id, 0 };
    Result rc = serviceDispatchIn(&g_insrSrv, 1, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out, tmp_handle, false);

    return rc;
}

NX_GENERATE_SERVICE_GUARD(inss);

Result _inssInitialize(void) {
    return smGetService(&g_inssSrv, "ins:s");
}

void _inssCleanup(void) {
    serviceClose(&g_inssSrv);
}

Service* inssGetServiceSession(void) {
    return &g_inssSrv;
}

Result inssGetWritableEvent(u32 id, Event *out) {
    Handle tmp_handle;
    struct {
        u32 id;
        u64 unk;
    } in = { id, 0 };
    Result rc = serviceDispatchIn(&g_inssSrv, 0, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) {
        *out = (Event){
            .revent    = INVALID_HANDLE,
            .wevent    = tmp_handle,
            .autoclear = false,
        };
    }

    return rc;
}
