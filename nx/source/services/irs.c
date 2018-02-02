#include <string.h>
#include "types.h"
#include "result.h"
#include "ipc.h"
#include "services/applet.h"
#include "services/irs.h"
#include "services/hid.h"
#include "services/sm.h"
#include "kernel/shmem.h"

static Service g_irsSrv;
static SharedMemory g_irsSharedmem;
bool g_irsSensorActivated;

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out, u64 AppletResourceUserId);

Result irsInitialize(void)
{
    if (serviceIsActive(&g_irsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    Result rc;
    Handle sharedmem_handle;
    u64 AppletResourceUserId=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = smGetService(&g_irsSrv, "irs");
    if (R_FAILED(rc))
        return rc;

    rc = _irsGetIrsensorSharedMemoryHandle(&sharedmem_handle, AppletResourceUserId);

    if (R_SUCCEEDED(rc))
    {
        shmemLoadRemote(&g_irsSharedmem, sharedmem_handle, 0x8000, Perm_R);

        rc = shmemMap(&g_irsSharedmem);
    }

    if (R_FAILED(rc))
        irsExit();

    return rc;
}

void irsExit(void)
{
    irsActivateIrsensor(0);

    serviceClose(&g_irsSrv);
    shmemClose(&g_irsSharedmem);
}

Service* irsGetSessionService(void) {
    return &g_irsSrv;
}

void* irsGetSharedmemAddr(void) {
    return shmemGetAddr(&g_irsSharedmem);
}

Result irsActivateIrsensor(bool activate) {
    if (g_irsSensorActivated==activate) return 0;

    Result rc=0;
    u64 AppletResourceUserId=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = activate ? 302 : 303;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_irsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) g_irsSensorActivated = activate;
    }

    return rc;
}

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 304;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_irsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

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

Result irsGetIrCameraHandle(u32 *IrCameraHandle, HidControllerID id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 311;
    raw->id = id;

    Result rc = serviceIpcDispatch(&g_irsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 IrCameraHandle;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && IrCameraHandle) {
            *IrCameraHandle = resp->IrCameraHandle;
        }
    }

    return rc;
}

