// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
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
        IpcCommand c;
        ipcInitialize(&c);
        ipcSendPid(&c);

        struct {
            u64 magic;
            u64 cmd_id;
            u64 zero;
            u64 reserved[2];
        } *raw;

        raw = serviceIpcPrepareHeader(&g_smSrv, &c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;
        raw->zero = 0;

        rc = serviceIpcDispatch(&g_smSrv);

        if (R_SUCCEEDED(rc)) {
            IpcParsedCommand r;

            struct {
                u64 magic;
                u64 result;
            } *resp;
            serviceIpcParse(&g_smSrv, &r, sizeof(*resp));

            resp = r.Raw;
            rc = resp->result;
        }
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

u64 smEncodeName(const char* name)
{
    u64 name_encoded = 0;
    size_t i;

    for (i=0; i<8; i++)
    {
        if (name[i] == '\0')
            break;

        name_encoded |= ((u64) name[i]) << (8*i);
    }

    return name_encoded;
}

Result smGetService(Service* service_out, const char* name)
{
    u64 name_encoded = smEncodeName(name);
    Handle handle = smGetServiceOverride(name_encoded);
    Result rc;

    if (handle != INVALID_HANDLE)
    {
        service_out->type = ServiceType_Override;
        service_out->handle = handle;
        rc = 0;
    }
    else
    {
        rc = smGetServiceOriginal(&handle, name_encoded);

        if (R_SUCCEEDED(rc))
        {
            service_out->type = ServiceType_Normal;
            service_out->handle = handle;
        }
    }

    return rc;
}

Result smGetServiceOriginal(Handle* handle_out, u64 name)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u64 reserved[2];
    } *raw;

    raw = serviceIpcPrepareHeader(&g_smSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->service_name = name;

    Result rc = serviceIpcDispatch(&g_smSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;
        serviceIpcParse(&g_smSrv, &r, sizeof(*resp));

        resp = r.Raw;
        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

Result smRegisterService(Handle* handle_out, const char* name, bool is_light, int max_sessions) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u32 is_light;
        u32 max_sessions;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_smSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->service_name = smEncodeName(name);
    raw->is_light = !!is_light;
    raw->max_sessions = max_sessions;

    Result rc = serviceIpcDispatch(&g_smSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;
        serviceIpcParse(&g_smSrv, &r, sizeof(*resp));

        resp = r.Raw;
        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

Result smUnregisterService(const char* name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u64 reserved;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_smSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(&g_smSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;
        serviceIpcParse(&g_smSrv, &r, sizeof(*resp));

        resp = r.Raw;
        rc = resp->result;
    }

    return rc;
}
