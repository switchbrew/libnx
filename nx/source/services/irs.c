#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/shmem.h"
#include "kernel/tmem.h"
#include "services/applet.h"
#include "services/irs.h"
#include "services/hid.h"
#include "services/sm.h"

typedef struct {
    bool initialized;
    u32 IrCameraHandle;
    TransferMemory transfermem;
} IrsCameraEntry;

static Service g_irsSrv;
static u64 g_refCnt;
static SharedMemory g_irsSharedmem;
static bool g_irsSensorActivated;

static IrsCameraEntry g_irsCameras[8];

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out, u64 AppletResourceUserId);

Result irsInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_irsSrv))
        return 0;

    Result rc;
    Handle sharedmem_handle;

    g_irsSensorActivated = 0;
    memset(g_irsCameras, 0, sizeof(g_irsCameras));

    // If this failed (for example because we're a sysmodule) AppletResourceUserId stays zero
    u64 AppletResourceUserId=0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

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
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        size_t entrycount = sizeof(g_irsCameras)/sizeof(IrsCameraEntry);
        IrsCameraEntry *entry;

        int i;
        for(i=0; i<entrycount; i++) {
            entry = &g_irsCameras[i];
            if (!entry->initialized) continue;
            irsStopImageProcessor(entry->IrCameraHandle);
        }

        irsActivateIrsensor(0);

        serviceClose(&g_irsSrv);
        shmemClose(&g_irsSharedmem);
    }
}

static Result _IrsCameraEntryAlloc(u32 IrCameraHandle, IrsCameraEntry **out_entry) {
    int i;
    size_t entrycount = sizeof(g_irsCameras)/sizeof(IrsCameraEntry);
    int empty_entry = -1;
    IrsCameraEntry *entry;

    if (out_entry) *out_entry = NULL;

    for(i=0; i<entrycount; i++) {
        entry = &g_irsCameras[i];
        if (entry->initialized) {
            if (entry->IrCameraHandle == IrCameraHandle)
                return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);
        }
        else if (empty_entry == -1)
            empty_entry = i;
    }

    if (empty_entry == -1)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    entry = &g_irsCameras[empty_entry];

    entry->initialized = 1;
    entry->IrCameraHandle = IrCameraHandle;

    if (out_entry) *out_entry = entry;

    return 0;
}

static Result _IrsCameraEntryGet(u32 IrCameraHandle, IrsCameraEntry **out_entry) {
    int i;
    size_t entrycount = sizeof(g_irsCameras)/sizeof(IrsCameraEntry);
    IrsCameraEntry *entry;
    *out_entry = NULL;

    for(i=0; i<entrycount; i++) {
        entry = &g_irsCameras[i];
        if (entry->initialized && entry->IrCameraHandle == IrCameraHandle) {
            *out_entry = entry;
            return 0;
        }
    }

    return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
}

static void _IrsCameraEntryFree(IrsCameraEntry *entry) {
    tmemClose(&entry->transfermem);
    memset(entry, 0, sizeof(IrsCameraEntry));
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

static Result _irsStopImageProcessor(u32 IrCameraHandle, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 305;
    raw->IrCameraHandle = IrCameraHandle;
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
    }

    return rc;
}

Result irsStopImageProcessor(u32 IrCameraHandle) {
    Result rc=0;
    u64 AppletResourceUserId=0;
    IrsCameraEntry *entry = NULL;

    if (!serviceIsActive(&g_irsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _IrsCameraEntryGet(IrCameraHandle, &entry);
    if (R_FAILED(rc))
        return rc;

    if (entry->transfermem.handle == INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _irsStopImageProcessor(IrCameraHandle, AppletResourceUserId);
    _IrsCameraEntryFree(entry);
    return rc;
}

static Result _irsRunImageTransferProcessor(u32 IrCameraHandle, u64 AppletResourceUserId, IrsPackedImageTransferProcessorConfig *config, Handle transfermem, u64 transfermem_size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
        IrsPackedImageTransferProcessorConfig config;
        u64 TransferMemory_size;
    } *raw;

    ipcSendPid(&c);
    ipcSendHandleCopy(&c, transfermem);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 308;
    raw->IrCameraHandle = IrCameraHandle;
    raw->AppletResourceUserId = AppletResourceUserId;
    raw->TransferMemory_size = transfermem_size;

    memcpy(&raw->config, config, sizeof(raw->config));

    Result rc = serviceIpcDispatch(&g_irsSrv);

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

Result irsRunImageTransferProcessor(u32 IrCameraHandle, IrsImageTransferProcessorConfig *config, size_t size) {
    Result rc=0;
    u64 AppletResourceUserId=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedImageTransferProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.exposure = config->exposure;
    packed_config.ir_leds = config->ir_leds;
    packed_config.digital_gain = config->digital_gain;
    packed_config.color_invert = config->color_invert;
    packed_config.unk_constant = 0xa0003;
    packed_config.sensor_res = config->sensor_res;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _IrsCameraEntryAlloc(IrCameraHandle, &entry);
    if (R_FAILED(rc))
        return rc;

    rc = tmemCreate(&entry->transfermem, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = _irsRunImageTransferProcessor(IrCameraHandle, AppletResourceUserId, &packed_config, entry->transfermem.handle, size);

    if (R_FAILED(rc)) _IrsCameraEntryFree(entry);

    return rc;
}

static Result _irsGetImageTransferProcessorState(u32 IrCameraHandle, u64 AppletResourceUserId, void* buffer, size_t size, IrsImageTransferProcessorState *state) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, buffer, size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 309;
    raw->IrCameraHandle = IrCameraHandle;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_irsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            IrsImageTransferProcessorState state;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && state)
            memcpy(state, &resp->state, sizeof(IrsImageTransferProcessorState)); 
    }

    return rc;
}

Result irsGetImageTransferProcessorState(u32 IrCameraHandle, void* buffer, size_t size, IrsImageTransferProcessorState *state) {
    Result rc=0;
    u64 AppletResourceUserId=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _irsGetImageTransferProcessorState(IrCameraHandle, AppletResourceUserId, buffer, size, state);
    return rc;
}

void irsGetDefaultImageTransferProcessorConfig(IrsImageTransferProcessorConfig *config) {
    memset(config, 0, sizeof(IrsImageTransferProcessorConfig));

    config->exposure = 300000;
    config->ir_leds = 0;
    config->digital_gain = 8;
    config->color_invert = 0;
    config->sensor_res = 0;
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

static Result _irsSuspendImageProcessor(u32 IrCameraHandle, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 313;
    raw->IrCameraHandle = IrCameraHandle;
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
    }

    return rc;
}

Result irsSuspendImageProcessor(u32 IrCameraHandle) {
    Result rc=0;
    u64 AppletResourceUserId=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _irsSuspendImageProcessor(IrCameraHandle, AppletResourceUserId);
    return rc;
}

