#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include "kernel/shmem.h"
#include "kernel/tmem.h"
#include "services/applet.h"
#include "services/irs.h"

typedef struct {
    bool initialized;
    u32 IrCameraHandle;
    TransferMemory transfermem;
} IrsCameraEntry;

static Service g_irsSrv;
static SharedMemory g_irsSharedmem;
static bool g_irsSensorActivated;

static IrsCameraEntry g_irsCameras[8];

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out, u64 AppletResourceUserId);

NX_GENERATE_SERVICE_GUARD(irs);

Result _irsInitialize(void) {
    Result rc=0;
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

    if (R_SUCCEEDED(rc)) {
        shmemLoadRemote(&g_irsSharedmem, sharedmem_handle, 0x8000, Perm_R);

        rc = shmemMap(&g_irsSharedmem);
    }

    return rc;
}

void _irsCleanup(void) {
    size_t entrycount = sizeof(g_irsCameras)/sizeof(IrsCameraEntry);
    IrsCameraEntry *entry;

    if (serviceIsActive(&g_irsSrv)) {
        for(size_t i=0; i<entrycount; i++) {
            entry = &g_irsCameras[i];
            if (!entry->initialized) continue;
            irsStopImageProcessor(entry->IrCameraHandle);
        }

        irsActivateIrsensor(0);
    }

    serviceClose(&g_irsSrv);
    shmemClose(&g_irsSharedmem);
}

Service* irsGetServiceSession(void) {
    return &g_irsSrv;
}

void* irsGetSharedmemAddr(void) {
    return shmemGetAddr(&g_irsSharedmem);
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

Result irsActivateIrsensor(bool activate) {
    if (g_irsSensorActivated==activate) return 0;

    Result rc=0;
    u64 AppletResourceUserId=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        return rc;

    rc = serviceDispatchIn(&g_irsSrv, activate ? 302 : 303, AppletResourceUserId,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc)) g_irsSensorActivated = activate;
    return rc;
}

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out, u64 AppletResourceUserId) {
    return serviceDispatchIn(&g_irsSrv, 304, AppletResourceUserId,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _irsStopImageProcessor(u32 IrCameraHandle, u64 AppletResourceUserId) {
    const struct {
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
    } in = { IrCameraHandle, AppletResourceUserId };

    return serviceDispatchIn(&g_irsSrv, 305, in,
        .in_send_pid = true,
    );
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

static Result _irsRunImageTransferProcessor(u32 IrCameraHandle, u64 AppletResourceUserId, IrsPackedImageTransferProcessorConfig *config, TransferMemory *tmem) {
    const struct {
        u32 IrCameraHandle;
        u32 pad;
        u64 AppletResourceUserId;
        IrsPackedImageTransferProcessorConfig config;
        u64 TransferMemory_size;
    } in = { IrCameraHandle, 0, AppletResourceUserId, *config, tmem->size };

    return serviceDispatchIn(&g_irsSrv, 308, in,
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
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

    rc = _irsRunImageTransferProcessor(IrCameraHandle, AppletResourceUserId, &packed_config, &entry->transfermem);

    if (R_FAILED(rc)) _IrsCameraEntryFree(entry);

    return rc;
}

static Result _irsGetImageTransferProcessorState(u32 IrCameraHandle, u64 AppletResourceUserId, void* buffer, size_t size, IrsImageTransferProcessorState *state) {
    const struct {
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
    } in = { IrCameraHandle, AppletResourceUserId };

    return serviceDispatchInOut(&g_irsSrv, 309, in, *state,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
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
    u32 tmp = hidControllerIDToOfficial(id);
    return serviceDispatchInOut(&g_irsSrv, 311, tmp, *IrCameraHandle);
}

static Result _irsSuspendImageProcessor(u32 IrCameraHandle, u64 AppletResourceUserId) {
    const struct {
        u32 IrCameraHandle;
        u64 AppletResourceUserId;
    } in = { IrCameraHandle, AppletResourceUserId };

    return serviceDispatchIn(&g_irsSrv, 313, in,
        .in_send_pid = true,
    );
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

