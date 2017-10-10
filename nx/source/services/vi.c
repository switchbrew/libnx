#include <string.h>
#include <switch.h>

static Handle g_viServiceSession = 0;
static u32 g_viServiceType = -1;
static Handle g_viIApplicationDisplayService = 0;
static Handle g_viIHOSBinderDriverRelay = 0;
static Handle g_viISystemDisplayService = 0;
static Handle g_viIManagerDisplayService = 0;
static Handle g_viIHOSBinderDriverIndirect = 0;

static Result _viGetSession(Handle sessionhandle, Handle* handle_out, void* inraw, size_t rawsize);
static Result _viGetSessionNoParams(Handle sessionhandle, Handle* handle_out, u64 cmd_id);

Result viInitialize(viServiceType servicetype) {
    if(g_viServiceType!=-1)return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;

    if (servicetype==VISERVTYPE_Default || servicetype==VISERVTYPE_Manager) {
        rc = smGetService(&g_viServiceSession, "vi:m");
        g_viServiceType = 2;
    }

    if ((servicetype==VISERVTYPE_Default && R_FAILED(rc)) || servicetype==VISERVTYPE_System) {
        rc = smGetService(&g_viServiceSession, "vi:s");
        g_viServiceType = 1;
    }

    if ((servicetype==VISERVTYPE_Default && R_FAILED(rc)) || servicetype==VISERVTYPE_Application)
    {
        rc = smGetService(&g_viServiceSession, "vi:u");
        g_viServiceType = 0;
    }

    if (R_SUCCEEDED(rc)) {
        struct {
            u64 magic;
            u64 cmd_id;
            u64 inval0;
        } raw;

        raw.magic = SFCI_MAGIC;
        raw.cmd_id = g_viServiceType;
        raw.inval0 = 0;

        rc = _viGetSession(g_viServiceSession, &g_viIApplicationDisplayService, &raw, sizeof(raw));
    }

    if (R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIHOSBinderDriverRelay, 100);
    if (g_viServiceType>=VISERVTYPE_System && R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viISystemDisplayService, 101);
    if (g_viServiceType>=VISERVTYPE_Manager && R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIManagerDisplayService, 102);
    if (g_viServiceType>=VISERVTYPE_System && R_SUCCEEDED(rc))rc = _viGetSessionNoParams(g_viIApplicationDisplayService, &g_viIHOSBinderDriverIndirect, 103);

    if (R_FAILED(rc))g_viServiceType = -1;

    return rc;
}

void viExit(void)
{
    if(g_viServiceType==-1)return;

    g_viServiceType = -1;

    if(g_viServiceSession)
    {
        svcCloseHandle(g_viServiceSession);
        g_viServiceSession = 0;
    }

    if(g_viIApplicationDisplayService)
    {
        svcCloseHandle(g_viIApplicationDisplayService);
        g_viIApplicationDisplayService = 0;
    }

    if(g_viISystemDisplayService)
    {
        svcCloseHandle(g_viISystemDisplayService);
        g_viISystemDisplayService = 0;
    }

    if(g_viIManagerDisplayService)
    {
        svcCloseHandle(g_viIManagerDisplayService);
        g_viIManagerDisplayService = 0;
    }

    if(g_viIHOSBinderDriverIndirect)
    {
        svcCloseHandle(g_viIHOSBinderDriverIndirect);
        g_viIHOSBinderDriverIndirect = 0;
    }
}

Handle viGetSessionService(void)
{
    return g_viServiceSession;
}

Handle viGetSession_IApplicationDisplayService(void)
{
    return g_viIApplicationDisplayService;
}

Handle viGetSession_IHOSBinderDriverRelay(void)
{
    return g_viIHOSBinderDriverRelay;
}

Handle viGetSession_ISystemDisplayService(void)
{
    return g_viISystemDisplayService;
}

Handle viGetSession_IManagerDisplayService(void)
{
    return g_viIManagerDisplayService;
}

Handle viGetSession_IHOSBinderDriverIndirect(void)
{
    return g_viIHOSBinderDriverIndirect;
}

static Result _viGetSession(Handle sessionhandle, Handle* handle_out, void* inraw, size_t rawsize) {
    IpcCommand c;
    ipcInitialize(&c);

    memcpy(ipcPrepareHeader(&c, rawsize), inraw, rawsize);

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

static Result _viGetSessionNoParams(Handle sessionhandle, Handle* handle_out, u64 cmd_id) {
    struct {
        u64 magic;
        u64 cmd_id;
    } raw;

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = cmd_id;

    return _viGetSession(sessionhandle, handle_out, &raw, sizeof(raw));
}

