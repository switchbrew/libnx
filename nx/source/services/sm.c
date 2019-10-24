#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/fatal.h"

static Service g_smSrv;

#define MAX_OVERRIDES 32

static struct {
    SmServiceName name;
    Handle handle;
} g_smOverrides[MAX_OVERRIDES];

static size_t g_smOverridesNum = 0;

void smAddOverrideHandle(SmServiceName name, Handle handle) {
    if (g_smOverridesNum == MAX_OVERRIDES)
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_TooManyOverrides));

    size_t i = g_smOverridesNum;

    g_smOverrides[i].name   = name;
    g_smOverrides[i].handle = handle;

    g_smOverridesNum++;
}

Handle smGetServiceOverride(SmServiceName name) {
    for (size_t i = 0; i < g_smOverridesNum; i++)
        if (smServiceNamesAreEqual(g_smOverrides[i].name, name))
            return g_smOverrides[i].handle;

    return INVALID_HANDLE;
}

NX_GENERATE_SERVICE_GUARD(sm);

Result _smInitialize(void) {
    Handle sm_handle;
    Result rc = svcConnectToNamedPort(&sm_handle, "sm:");
    while (R_VALUE(rc) == KERNELRESULT(NotFound)) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&sm_handle, "sm:");
    }

    if (R_SUCCEEDED(rc)) {
        serviceCreate(&g_smSrv, sm_handle);
    }

    Handle tmp;
    if (R_SUCCEEDED(rc) && smGetServiceOriginal(&tmp, (SmServiceName){}) == 0x415) {
        u64 pid_placeholder = 0;
        rc = serviceDispatchIn(&g_smSrv, 0, pid_placeholder, .in_send_pid = true);
    }

    return rc;
}

void _smCleanup(void) {
    serviceClose(&g_smSrv);
}

Service *smGetServiceSession(void) {
    return &g_smSrv;
}

Result smGetServiceWrapper(Service* service_out, SmServiceName name) {
    Handle handle = smGetServiceOverride(name);
    bool own_handle = false;
    Result rc = 0;

    if (handle == INVALID_HANDLE) {
        own_handle = true;
        rc = smGetServiceOriginal(&handle, name);
    }

    if (R_SUCCEEDED(rc)) {
        serviceCreate(service_out, handle);
        service_out->own_handle = own_handle;
    }

    return rc;
}

Result smGetServiceOriginal(Handle* handle_out, SmServiceName name) {
    return serviceDispatchIn(&g_smSrv, 1, name,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smRegisterService(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions) {
    const struct {
        SmServiceName service_name;
        u8 is_light;
        s32 max_sessions;
    } in = { name, is_light!=0, max_sessions };

    return serviceDispatchIn(&g_smSrv, 2, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smUnregisterService(SmServiceName name) {
    return serviceDispatchIn(&g_smSrv, 3, name);
}
