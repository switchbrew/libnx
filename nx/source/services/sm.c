// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "sf/service.h"
#include "services/fatal.h"
#include "services/sm.h"

static Service g_smSrv;
static u64 g_refCnt;

#define MAX_OVERRIDES 32

static struct {
    u64    name;
    Handle handle;
} g_smOverrides[MAX_OVERRIDES];

static size_t g_smOverridesNum = 0;

void smAddOverrideHandle(u64 name, Handle handle)
{
    if (g_smOverridesNum == MAX_OVERRIDES)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_TooManyOverrides));

    size_t i = g_smOverridesNum;

    g_smOverrides[i].name   = name;
    g_smOverrides[i].handle = handle;

    g_smOverridesNum++;
}

Handle smGetServiceOverride(u64 name)
{
    size_t i;

    for (i=0; i<g_smOverridesNum; i++)
    {
        if (g_smOverrides[i].name == name)
            return g_smOverrides[i].handle;
    }

    return INVALID_HANDLE;
}

bool smHasInitialized(void) {
    return serviceIsActive(&g_smSrv);
}

Result smInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (smHasInitialized())
        return 0;

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
    if (R_SUCCEEDED(rc) && smGetServiceOriginal(&tmp, smEncodeName("")) == 0x415) {
        const struct {
            u64 pid_placeholder;
        } in = { 0 };

        rc = serviceDispatchIn(&g_smSrv, 0, in, .in_send_pid = true);
    }

    if (R_FAILED(rc))
        smExit();

    return rc;
}

void smExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        serviceClose(&g_smSrv);
    }
}

Service *smGetServiceSession(void)
{
    return &g_smSrv;
}

Result smGetService(Service* service_out, const char* name)
{
    u64 name_encoded = smEncodeName(name);
    Handle handle = smGetServiceOverride(name_encoded);
    bool own_handle = false;
    Result rc = 0;

    if (handle == INVALID_HANDLE)
    {
        own_handle = true;
        rc = smGetServiceOriginal(&handle, name_encoded);
    }

    if (R_SUCCEEDED(rc))
    {
        serviceCreate(service_out, handle);
        service_out->own_handle = own_handle;
    }

    return rc;
}

Result smGetServiceOriginal(Handle* handle_out, u64 name)
{
    const struct {
        u64 service_name;
    } in = { name };

    return serviceDispatchIn(&g_smSrv, 1, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smRegisterService(Handle* handle_out, const char* name, bool is_light, int max_sessions)
{
    const struct {
        u64 service_name;
        u32 is_light;
        u32 max_sessions;
    } in = { smEncodeName(name), !!is_light, max_sessions };

    return serviceDispatchIn(&g_smSrv, 2, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = handle_out,
    );
}

Result smUnregisterService(const char* name)
{
    const struct {
        u64 service_name;
    } in = { smEncodeName(name) };

    return serviceDispatchIn(&g_smSrv, 3, in);
}
