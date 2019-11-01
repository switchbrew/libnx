#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "kernel/tmem.h"
#include "services/applet.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"

__attribute__((weak)) u32 __nx_nv_transfermem_size = 0x800000;

static Service g_nvSrv;
static Service g_nvSrvClone;

static size_t g_nvIpcBufferSize = 0;
static TransferMemory g_nvTransfermem;

static Result _nvCmdInitialize(Handle proc, Handle sharedmem, u32 transfermem_size);
static Result _nvSetClientPID(u64 AppletResourceUserId);

NX_GENERATE_SERVICE_GUARD(nv);

Result _nvInitialize(void) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
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
        g_nvIpcBufferSize = g_nvSrv.pointer_buffer_size;

        if (R_SUCCEEDED(rc))
            rc = tmemCreate(&g_nvTransfermem, __nx_nv_transfermem_size, Perm_None);

        if (R_SUCCEEDED(rc))
            rc = _nvCmdInitialize(CUR_PROCESS_HANDLE, g_nvTransfermem.handle, __nx_nv_transfermem_size);

        // Clone the session handle - the cloned session is used to execute certain commands in parallel
        if (R_SUCCEEDED(rc))
            rc = serviceCloneEx(&g_nvSrv, 1, &g_nvSrvClone);

        if (R_SUCCEEDED(rc)) {
            // Send aruid if available (non-fatal condition if get-aruid fails)
            Result aruid_rc = appletGetAppletResourceUserId(&AppletResourceUserId);
            if (R_SUCCEEDED(aruid_rc))
                rc = _nvSetClientPID(AppletResourceUserId);
        }
    }

    return rc;
}

void _nvCleanup(void) {
    serviceClose(&g_nvSrvClone);
    serviceClose(&g_nvSrv);
    tmemClose(&g_nvTransfermem);
}

Service* nvGetServiceSession(void) {
    return &g_nvSrv;
}

static Result _nvCmdInitialize(Handle proc, Handle sharedmem, u32 transfermem_size) {
    return serviceDispatchIn(&g_nvSrv, 3, transfermem_size,
        .in_num_handles = 2,
        .in_handles = { proc, sharedmem },
    );
}

static Result _nvSetClientPID(u64 AppletResourceUserId) {
    return serviceDispatchIn(&g_nvSrv, 8, AppletResourceUserId, .in_send_pid = true);
}

Result nvOpen(u32 *fd, const char *devicepath) {
    struct {
        u32 fd;
        u32 error;
    } out;

    Result rc = serviceDispatchOut(&g_nvSrv, 0, out,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { devicepath, strlen(devicepath) } },
    );

    if (R_SUCCEEDED(rc))
        rc = nvConvertError(out.error);

    if (R_SUCCEEDED(rc) && fd)
        *fd = out.fd;

    return rc;
}

// Get the appropriate session for the specified request (same logic as official sw)
static inline Service* _nvGetSessionForRequest(u32 request) {
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
    size_t bufsize = _NV_IOC_SIZE(request);
    u32 dir = _NV_IOC_DIR(request);

    void *buf_send = NULL, *buf_recv = NULL;
    size_t buf_send_size = 0, buf_recv_size = 0;

    if (dir & _NV_IOC_WRITE) {
        buf_send = argp;
        buf_send_size = bufsize;
    }

    if (dir & _NV_IOC_READ) {
        buf_recv = argp;
        buf_recv_size = bufsize;
    }

    const struct {
        u32 fd;
        u32 request;
    } in = { fd, request };

    u32 error = 0;
    Result rc = serviceDispatchInOut(_nvGetSessionForRequest(request), 1, in, error,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { buf_send, buf_send_size },
            { buf_recv, buf_recv_size },
        },
    );

    if (R_SUCCEEDED(rc))
        rc = nvConvertError(error);

    return rc;
}

Result nvIoctl2(u32 fd, u32 request, void* argp, const void* inbuf, size_t inbuf_size) {
    size_t bufsize = _NV_IOC_SIZE(request);
    u32 dir = _NV_IOC_DIR(request);

    void *buf_send = NULL, *buf_recv = NULL;
    size_t buf_send_size = 0, buf_recv_size = 0;

    if (dir & _NV_IOC_WRITE) {
        buf_send = argp;
        buf_send_size = bufsize;
    }

    if (dir & _NV_IOC_READ) {
        buf_recv = argp;
        buf_recv_size = bufsize;
    }

    const struct {
        u32 fd;
        u32 request;
    } in = { fd, request };

    u32 error = 0;
    Result rc = serviceDispatchInOut(_nvGetSessionForRequest(request), 11, in, error,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { buf_send, buf_send_size },
            { inbuf,    inbuf_size    },
            { buf_recv, buf_recv_size },
        },
    );

    if (R_SUCCEEDED(rc))
        rc = nvConvertError(error);

    return rc;
}

Result nvClose(u32 fd) {
    u32 error = 0;
    Result rc = serviceDispatchInOut(&g_nvSrv, 2, fd, error);

    if (R_SUCCEEDED(rc))
        rc = nvConvertError(error);

    return rc;
}

Result nvQueryEvent(u32 fd, u32 event_id, Event *event_out) {
    const struct {
        u32 fd;
        u32 event_id;
    } in = { fd, event_id };

    u32 error = 0;
    Handle event;
    Result rc = serviceDispatchInOut(&g_nvSrv, 4, in, error,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        rc = nvConvertError(error);

    if (R_SUCCEEDED(rc))
        eventLoadRemote(event_out, event, true);

    return rc;
}

Result nvConvertError(int rc) {
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
