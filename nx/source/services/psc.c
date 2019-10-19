#include "service_guard.h"
#include "services/psc.h"
#include "runtime/hosversion.h"

static Service g_pscmSrv;

NX_GENERATE_SERVICE_GUARD(pscm);

Result _pscmInitialize(void) {
    Result rc = smGetService(&g_pscmSrv, "psc:m");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_pscmSrv);
    }

    return rc;
}

void _pscmCleanup(void) {
    serviceClose(&g_pscmSrv);
}

Service* pscmGetServiceSession(void) {
    return &g_pscmSrv;
}


NX_INLINE Result _pscPmModuleInitialize(PscPmModule *module, PscPmModuleId module_id, const u16 *dependencies, size_t dependency_count, bool autoclear);

Result pscmGetPmModule(PscPmModule *out, PscPmModuleId module_id, const u16 *dependencies, size_t dependency_count, bool autoclear) {
    serviceAssumeDomain(&g_pscmSrv);
    Result rc = serviceDispatch(&g_pscmSrv, 0,
        .out_num_objects = 1,
        .out_objects = &out->srv,
    );

    if (R_SUCCEEDED(rc)) {
        if (R_FAILED((rc = _pscPmModuleInitialize(out, module_id, dependencies, dependency_count, autoclear)))) {
            pscPmModuleClose(out);
        }
    }

    return rc;
}

Result _pscPmModuleInitialize(PscPmModule *module, PscPmModuleId module_id, const u16 *dependencies, size_t dependency_count, bool autoclear) {
    _Static_assert(sizeof(module_id) == sizeof(u32), "PscPmModuleId size");

    Handle evt_handle = INVALID_HANDLE;
    serviceAssumeDomain(&module->srv);
    Result rc = serviceDispatchIn(&module->srv, 0, module_id,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { dependencies,  dependency_count * sizeof(*dependencies) } },
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &evt_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(&module->event, evt_handle, autoclear);
        module->module_id = module_id;
    }

    return rc;
}

Result pscPmModuleGetRequest(PscPmModule *module, PscPmState *out_state, u32 *out_flags) {
    struct {
        u32 state;
        u32 flags;
    } out;

    serviceAssumeDomain(&module->srv);
    Result rc = serviceDispatchOut(&module->srv, 1, out);

    if (R_SUCCEEDED(rc)) {
        if (out_state) *out_state = (PscPmState)out.state;
        if (out_flags) *out_flags = out.flags;
    }

    return rc;
}

Result pscPmModuleAcknowledge(PscPmModule *module, PscPmState state) {
    serviceAssumeDomain(&module->srv);

    if (hosversionAtLeast(6,0,0)) {
        _Static_assert(sizeof(state) == sizeof(u32), "PscPmState size");
        return serviceDispatchIn(&module->srv, 4, state);
    } else {
        return serviceDispatch(&module->srv, 2);
    }
}

Result pscPmModuleFinalize(PscPmModule *module) {
    serviceAssumeDomain(&module->srv);
    return serviceDispatch(&module->srv, 3);
}

void pscPmModuleClose(PscPmModule *module) {
    serviceAssumeDomain(&module->srv);
    serviceClose(&module->srv);
}
