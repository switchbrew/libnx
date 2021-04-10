#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "sf/tipc.h"
#include "runtime/hosversion.h"
#include "runtime/diag.h"

static union {
    Service cmif;
    TipcService tipc;
} g_smSrv;

#define MAX_OVERRIDES 32

static struct {
    SmServiceName name;
    Handle handle;
} g_smOverrides[MAX_OVERRIDES];

static size_t g_smOverridesNum = 0;

static bool _smShouldUseTipc(void) {
    return hosversionIsAtmosphere() || hosversionAtLeast(12,0,0);
}

void smAddOverrideHandle(SmServiceName name, Handle handle) {
    if (g_smOverridesNum == MAX_OVERRIDES)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_TooManyOverrides));

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

static Result _smCmifCmdInPid(u32 cmd_id) {
    u64 pid_placeholder = 0;
    return serviceDispatchIn(&g_smSrv.cmif, cmd_id, pid_placeholder, .in_send_pid = true);
}

static Result _smTipcCmdInPid(u32 cmd_id) {
    return tipcDispatch(&g_smSrv.tipc, cmd_id, .in_send_pid = true);
}

Result _smInitialize(void) {
    Handle sm_handle;
    Result rc = svcConnectToNamedPort(&sm_handle, "sm:");
    while (R_VALUE(rc) == KERNELRESULT(NotFound)) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&sm_handle, "sm:");
    }

    // Call RegisterClient. This is unconditionally done through cmif,
    // see comment in smGetServiceOriginal for more details.
    if (R_SUCCEEDED(rc)) {
        serviceCreate(&g_smSrv.cmif, sm_handle);
        rc = _smCmifCmdInPid(0); // RegisterClient
    }

    return rc;
}

void _smCleanup(void) {
    serviceClose(&g_smSrv.cmif);
}

Service *smGetServiceSession(void) {
    return &g_smSrv.cmif;
}

TipcService *smGetServiceSessionTipc(void) {
    return &g_smSrv.tipc;
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
    // Even though GetServiceHandle is also available through tipc, we choose to only
    // call this command through cmif, since that is available on all system versions.
    return serviceDispatchIn(&g_smSrv.cmif, 1, name,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smRegisterService(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions) {
    if (_smShouldUseTipc())
        return smRegisterServiceTipc(handle_out, name, is_light, max_sessions);
    else
        return smRegisterServiceCmif(handle_out, name, is_light, max_sessions);
}

Result smRegisterServiceCmif(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions) {
    const struct {
        SmServiceName service_name;
        u8 is_light;
        s32 max_sessions;
    } in = { name, is_light!=0, max_sessions };

    return serviceDispatchIn(&g_smSrv.cmif, 2, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smRegisterServiceTipc(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions) {
    const struct {
        SmServiceName service_name;
        s32 max_sessions;
        u8 is_light;
    } in = { name, max_sessions, is_light!=0 };

    return tipcDispatchIn(&g_smSrv.tipc, 2, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smUnregisterService(SmServiceName name) {
    if (_smShouldUseTipc())
        return smUnregisterServiceTipc(name);
    else
        return smUnregisterServiceCmif(name);
}

Result smUnregisterServiceCmif(SmServiceName name) {
    return serviceDispatchIn(&g_smSrv.cmif, 3, name);
}

Result smUnregisterServiceTipc(SmServiceName name) {
    return tipcDispatchIn(&g_smSrv.tipc, 3, name);
}

Result smDetachClient(void) {
    if (hosversionIsAtmosphere())
        return smDetachClientTipc();
    else if (hosversionBetween(11, 12))
        return smDetachClientCmif();
    else
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
}

Result smDetachClientCmif(void) {
    return _smCmifCmdInPid(4);
}

Result smDetachClientTipc(void) {
    return _smTipcCmdInPid(4);
}
