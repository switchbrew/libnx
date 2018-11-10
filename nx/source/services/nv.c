#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/tmem.h"
#include "services/applet.h"
#include "nvidia/ioctl.h"
#include "services/nv.h"
#include "services/sm.h"

__attribute__((weak)) u32 __nx_nv_transfermem_size = 0x800000;

static Service g_nvSrv;
static Service g_nvSrvClone;
static u64 g_refCnt;

static size_t g_nvIpcBufferSize = 0;
static TransferMemory g_nvTransfermem;

static Result _nvInitialize(Handle proc, Handle sharedmem, u32 transfermem_size);
static Result _nvSetClientPID(u64 AppletResourceUserId);

Result nvInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_nvSrv))
        return 0;

    Result rc = 0;
    u64 AppletResourceUserId = 0;

    switch (appletGetAppletType()) {
    case AppletType_None:
        rc = smGetService(&g_nvSrv, "nvdrv:s");
        break;

    case AppletType_Default:
    case AppletType_Application:
    case AppletType_SystemApplication:
    default:
        rc = smGetService(&g_nvSrv, "nvdrv");
        break;

    case AppletType_SystemApplet:
    case AppletType_LibraryApplet:
    case AppletType_OverlayApplet:
        rc = smGetService(&g_nvSrv, "nvdrv:a");
        break;
    }

    if (R_SUCCEEDED(rc)) {
        g_nvIpcBufferSize = 0;
        rc = ipcQueryPointerBufferSize(g_nvSrv.handle, &g_nvIpcBufferSize);

        if (R_SUCCEEDED(rc))
            rc = tmemCreate(&g_nvTransfermem, __nx_nv_transfermem_size, Perm_None);

        if (R_SUCCEEDED(rc))
            rc = _nvInitialize(CUR_PROCESS_HANDLE, g_nvTransfermem.handle, __nx_nv_transfermem_size);

        // Clone the session handle - the cloned session is used to execute certain commands in parallel
        Handle nv_clone = INVALID_HANDLE;
        if (R_SUCCEEDED(rc))
            rc = ipcCloneSession(g_nvSrv.handle, 1, &nv_clone);

        if (R_SUCCEEDED(rc))
            serviceCreate(&g_nvSrvClone, nv_clone);

        if (R_SUCCEEDED(rc))
        {
            // Send aruid if available (non-fatal condition if get-aruid fails)
            Result aruid_rc = appletGetAppletResourceUserId(&AppletResourceUserId);
            if (R_SUCCEEDED(aruid_rc))
                rc = _nvSetClientPID(AppletResourceUserId);
        }
    }

    if (R_FAILED(rc)) {
        nvExit();
    }

    return rc;
}

void nvExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_nvSrvClone);
        serviceClose(&g_nvSrv);
        tmemClose(&g_nvTransfermem);
    }
}

static Result _nvInitialize(Handle proc, Handle sharedmem, u32 transfermem_size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 transfermem_size;
    } *raw;

    ipcSendHandleCopy(&c, proc);
    ipcSendHandleCopy(&c, sharedmem);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->transfermem_size = transfermem_size;

    Result rc = serviceIpcDispatch(&g_nvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _nvSetClientPID(u64 AppletResourceUserId) {
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
    raw->cmd_id = 8;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_nvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nvOpen(u32 *fd, const char *devicepath) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddSendBuffer(&c, devicepath, strlen(devicepath), 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_nvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 fd;
            u32 error;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            rc = nvConvertError(resp->error);

        if (R_SUCCEEDED(rc))
            *fd = resp->fd;
    }

    return rc;
}

// Get the appropriate session for the specified request (same logic as official sw)
static inline Service* _nvGetSessionForRequest(u32 request)
{
    if (
        (request & 0xC000FFFF) == 0xC0004808 || // NVGPU_IOCTL_CHANNEL_SUBMIT_GPFIFO
        request == 0xC018481B ||                // NVGPU_IOCTL_CHANNEL_KICKOFF_PB
        request == 0xC004001C ||                // NVHOST_IOCTL_CTRL_EVENT_SIGNAL
        request == 0xC010001E                   // NVHOST_IOCTL_CTRL_EVENT_WAIT_ASYNC
        )
        return &g_nvSrvClone;
    return &g_nvSrv;
}

Result nvIoctl(u32 fd, u32 request, void* argp) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 fd;
        u32 request;
    } *raw;

    size_t bufsize = _NV_IOC_SIZE(request);
    u32 dir = _NV_IOC_DIR(request);

    void* buf_send = NULL, *buf_recv = NULL;
    size_t buf_send_size = 0, buf_recv_size = 0;

    if(dir & _NV_IOC_WRITE) {
        buf_send = argp;
        buf_send_size = bufsize;
    }

    if(dir & _NV_IOC_READ) {
        buf_recv = argp;
        buf_recv_size = bufsize;
    }

    ipcAddSendSmart(&c, g_nvIpcBufferSize, buf_send, buf_send_size, 0);
    ipcAddRecvSmart(&c, g_nvIpcBufferSize, buf_recv, buf_recv_size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->fd = fd;
    raw->request = request;

    Result rc = serviceIpcDispatch(_nvGetSessionForRequest(request));

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 error;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            rc = nvConvertError(resp->error);
    }

    return rc;
}

Result nvIoctl2(u32 fd, u32 request, void* argp, const void* inbuf, size_t inbuf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 fd;
        u32 request;
    } *raw;

    size_t bufsize = _NV_IOC_SIZE(request);
    u32 dir = _NV_IOC_DIR(request);

    void* buf_send = NULL, *buf_recv = NULL;
    size_t buf_send_size = 0, buf_recv_size = 0;

    if(dir & _NV_IOC_WRITE) {
        buf_send = argp;
        buf_send_size = bufsize;
    }

    if(dir & _NV_IOC_READ) {
        buf_recv = argp;
        buf_recv_size = bufsize;
    }

    ipcAddSendSmart(&c, g_nvIpcBufferSize, buf_send, buf_send_size, 0);
    ipcAddSendSmart(&c, g_nvIpcBufferSize, inbuf, inbuf_size, 1);
    ipcAddRecvSmart(&c, g_nvIpcBufferSize, buf_recv, buf_recv_size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->fd = fd;
    raw->request = request;

    Result rc = serviceIpcDispatch(_nvGetSessionForRequest(request));

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 error;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            rc = nvConvertError(resp->error);
    }

    return rc;
}

Result nvClose(u32 fd) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 fd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->fd = fd;

    Result rc = serviceIpcDispatch(&g_nvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 error;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            rc = nvConvertError(resp->error);
    }

    return rc;
}

Result nvQueryEvent(u32 fd, u32 event_id, Event *event_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 fd;
        u32 event_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->fd = fd;
    raw->event_id = event_id;

    Result rc = serviceIpcDispatch(&g_nvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 error;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            rc = nvConvertError(resp->error);

        if (R_SUCCEEDED(rc))
            eventLoadRemote(event_out, r.Handles[0], true);
    }

    return rc;
}

Result nvConvertError(int rc)
{
    if (rc == 0) // Fast path.
        return 0;

    int desc;
    switch (rc) {
    case 1:  desc = LibnxNvidiaError_NotImplemented; break;
    case 2:  desc = LibnxNvidiaError_NotSupported; break;
    case 3:  desc = LibnxNvidiaError_NotInitialized; break;
    case 4:  desc = LibnxNvidiaError_BadParameter; break;
    case 5:  desc = LibnxNvidiaError_Timeout; break;
    case 6:  desc = LibnxNvidiaError_InsufficientMemory; break;
    case 7:  desc = LibnxNvidiaError_ReadOnlyAttribute; break;
    case 8:  desc = LibnxNvidiaError_InvalidState; break;
    case 9:  desc = LibnxNvidiaError_InvalidAddress; break;
    case 10: desc = LibnxNvidiaError_InvalidSize; break;
    case 11: desc = LibnxNvidiaError_BadValue; break;
    case 13: desc = LibnxNvidiaError_AlreadyAllocated; break;
    case 14: desc = LibnxNvidiaError_Busy; break;
    case 15: desc = LibnxNvidiaError_ResourceError; break;
    case 16: desc = LibnxNvidiaError_CountMismatch; break;
    case 0x1000: desc = LibnxNvidiaError_SharedMemoryTooSmall; break;
    case 0x30003: desc = LibnxNvidiaError_FileOperationFailed; break;
    case 0x3000F: desc = LibnxNvidiaError_IoctlFailed; break;
    default: desc = LibnxNvidiaError_Unknown; break;
    }

    return MAKERESULT(Module_LibnxNvidia, desc);
}
