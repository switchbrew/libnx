#include <string.h>
#include "types.h"
#include "result.h"
#include "ipc.h"
#include "services/applet.h"
#include "services/irs.h"
#include "services/hid.h"
#include "services/sm.h"
#include "kernel/shmem.h"
#include "kernel/tmem.h"

typedef struct {
    bool initialized;
    u32 IrCameraHandle;
    TransferMemory transfermem;
} irsCameraEntry;

static Service g_irsSrv;
static SharedMemory g_irsSharedmem;
static bool g_irsSensorActivated;

static irsCameraEntry g_irsCameras[8];

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out, u64 AppletResourceUserId);

Result irsInitialize(void)
{
    if (serviceIsActive(&g_irsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    Result rc;
    Handle sharedmem_handle;
    u64 AppletResourceUserId=0;

    g_irsSensorActivated = 0;
    memset(g_irsCameras, 0, sizeof(g_irsCameras));

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
    int i;
    size_t entrycount = sizeof(g_irsCameras)/sizeof(irsCameraEntry);
    irsCameraEntry *entry;

    for(i=0; i<entrycount; i++) {
        entry = &g_irsCameras[i];
        if (!entry->initialized) continue;
        irsStopImageProcessor(entry->IrCameraHandle);
    }

    irsActivateIrsensor(0);

    serviceClose(&g_irsSrv);
    shmemClose(&g_irsSharedmem);
}

static Result _irsCameraEntryAlloc(u32 IrCameraHandle, irsCameraEntry **out_entry) {
    int i;
    size_t entrycount = sizeof(g_irsCameras)/sizeof(irsCameraEntry);
    int empty_entry = -1;
    irsCameraEntry *entry;

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

static Result _irsCameraEntryGet(u32 IrCameraHandle, irsCameraEntry **out_entry) {
    int i;
    size_t entrycount = sizeof(g_irsCameras)/sizeof(irsCameraEntry);
    irsCameraEntry *entry;
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

static void _irsCameraEntryFree(irsCameraEntry *entry) {
    tmemClose(&entry->transfermem);
    memset(entry, 0, sizeof(irsCameraEntry));
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
    irsCameraEntry *entry = NULL;

    if (!serviceIsActive(&g_irsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _irsCameraEntryGet(IrCameraHandle, &entry);
    if (R_FAILED(rc))
        return rc;

    if (entry->transfermem.handle == INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _irsStopImageProcessor(IrCameraHandle, AppletResourceUserId);
    _irsCameraEntryFree(entry);
    return rc;
}

static Result _irsRunImageTransferProcessor(u32 IrCameraHandle, u64 AppletResourceUserId, irsPackedImageTransferProcessorConfig *config, Handle transfermem, u64 transfermem_size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
        irsPackedImageTransferProcessorConfig config;
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

Result irsRunImageTransferProcessor(u32 IrCameraHandle, irsImageTransferProcessorConfig *config, size_t size) {
    Result rc=0;
    u64 AppletResourceUserId=0;
    irsCameraEntry *entry = NULL;
    irsPackedImageTransferProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.unk_x0 = config->unk_x0;
    packed_config.unk_x8 = config->unk_x8;
    packed_config.unk_x9 = config->unk_xc;
    packed_config.unk_xa = config->unk_x10;
    packed_config.unk_constant = 0xa0003;
    packed_config.unk_x14 = config->unk_x18;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _irsCameraEntryAlloc(IrCameraHandle, &entry);
    if (R_FAILED(rc))
        return rc;

    rc = tmemCreate(&entry->transfermem, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = _irsRunImageTransferProcessor(IrCameraHandle, AppletResourceUserId, &packed_config, entry->transfermem.handle, size);

    if (R_FAILED(rc)) _irsCameraEntryFree(entry);

    return rc;
}

static Result _irsGetImageTransferProcessorState(u32 IrCameraHandle, u64 AppletResourceUserId, void* buffer, size_t size, irsImageTransferProcessorState *state) {
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
            irsImageTransferProcessorState state;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && state)
            memcpy(state, &resp->state, sizeof(irsImageTransferProcessorState)); 
    }

    return rc;
}

Result irsGetImageTransferProcessorState(u32 IrCameraHandle, void* buffer, size_t size, irsImageTransferProcessorState *state) {
    Result rc=0;
    u64 AppletResourceUserId=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = _irsGetImageTransferProcessorState(IrCameraHandle, AppletResourceUserId, buffer, size, state);
    return rc;
}

void irsGetDefaultImageTransferProcessorConfig(irsImageTransferProcessorConfig *config) {
    memset(config, 0, sizeof(irsImageTransferProcessorConfig));

    config->unk_x0 = 0x493E0;
    config->unk_xc = 0x8;
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

