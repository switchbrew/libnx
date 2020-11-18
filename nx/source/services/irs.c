#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <stdatomic.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "kernel/shmem.h"
#include "kernel/tmem.h"
#include "services/applet.h"
#include "services/irs.h"
#include "applets/hid_la.h"

typedef struct {
    IrsIrSensorMode mode;
    u32 internal_status;
    IrsIrCameraHandle handle;
    TransferMemory transfermem;
    bool version_check;

    u8 is_negative_image_used;
    u8 format;
    IrsRect window_of_interest;
} IrsCameraEntry;

typedef struct {
    s64 sampling_number;
    u32 prefix_data;
    u32 prefix_bitcount;
} IrsTeraFilterArg;

static Service g_irsSrv;
static SharedMemory g_irsSharedmem;
static IrsPackedFunctionLevel g_irsFunctionLevel; // In sdknso there's various funcs which get the FunctionLevel, but the only ones which actually use the loaded data is the Run*Processor funcs.

static IrsPackedMcuVersion g_irsRequiredMcuVersion;

static IrsCameraEntry g_irsCameras[IRS_MAX_CAMERAS];

static const size_t g_irsImageFormatSizes[] = {
    [IrsImageTransferProcessorFormat_320x240]    = 320*240,
    [IrsImageTransferProcessorFormat_160x120]    = 160*120,
    [IrsImageTransferProcessorFormat_80x60]      = 80*60,
    [IrsImageTransferProcessorFormat_40x30]      = 40*30,
    [IrsImageTransferProcessorFormat_20x15]      = 20*15,
};

static const Result g_irsCameraStatusResults[] = {
    [IrsIrCameraStatus_Available]   = MAKERESULT(205, 160),
    [IrsIrCameraStatus_Unsupported] = MAKERESULT(205, 111),
    [IrsIrCameraStatus_Unconnected] = MAKERESULT(205, 110),
};

static Result _irsActivateIrsensor(bool activate);

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out);

static Result _irsCheckFirmwareVersion(IrsIrCameraHandle handle, IrsPackedMcuVersion version);

static Result _irsActivateIrsensorWithFunctionLevel(IrsPackedFunctionLevel level);

NX_GENERATE_SERVICE_GUARD(irs);

Result _irsInitialize(void) {
    Result rc=0;
    Handle sharedmem_handle;

    memset(g_irsCameras, 0, sizeof(g_irsCameras));
    for (u32 i=0; i<IRS_MAX_CAMERAS; i++) g_irsCameras[i].version_check = 1;

    g_irsRequiredMcuVersion = (IrsPackedMcuVersion){.major_version = 0x3, .minor_version = 0xB};

    if (hosversionAtLeast(4,0,0))
        g_irsRequiredMcuVersion = (IrsPackedMcuVersion){.major_version = 0x4, .minor_version = 0x12};
    if (hosversionAtLeast(5,0,0))
        g_irsRequiredMcuVersion = (IrsPackedMcuVersion){.major_version = 0x5, .minor_version = 0x18};
    if (hosversionAtLeast(6,0,0))
        g_irsRequiredMcuVersion = (IrsPackedMcuVersion){.major_version = 0x6, .minor_version = 0x1A};
    if (hosversionAtLeast(8,0,0))
        g_irsRequiredMcuVersion = (IrsPackedMcuVersion){.major_version = 0x8, .minor_version = 0x1B};

    g_irsFunctionLevel.ir_sensor_function_level = 0x0;

    if (hosversionAtLeast(4,0,0))
        g_irsFunctionLevel.ir_sensor_function_level = 0x1;
    if (hosversionAtLeast(5,0,0))
        g_irsFunctionLevel.ir_sensor_function_level = 0x2;
    if (hosversionAtLeast(6,0,0))
        g_irsFunctionLevel.ir_sensor_function_level = 0x3;

    rc = smGetService(&g_irsSrv, "irs");
    if (R_FAILED(rc))
        return rc;

    if (hosversionBefore(4,0,0))
        rc = _irsActivateIrsensor(1);
    else
        rc = _irsActivateIrsensorWithFunctionLevel(g_irsFunctionLevel);

    if (R_SUCCEEDED(rc)) rc = _irsGetIrsensorSharedMemoryHandle(&sharedmem_handle);

    if (R_SUCCEEDED(rc)) {
        shmemLoadRemote(&g_irsSharedmem, sharedmem_handle, 0x8000, Perm_R);

        rc = shmemMap(&g_irsSharedmem);
    }

    return rc;
}

void _irsCleanup(void) {
    IrsCameraEntry *entry;

    if (serviceIsActive(&g_irsSrv)) {
        for (u32 i=0; i<IRS_MAX_CAMERAS; i++) {
            entry = &g_irsCameras[i];
            if (entry->mode != IrsIrSensorMode_None) irsStopImageProcessor(entry->handle);
        }

        _irsActivateIrsensor(0);
    }

    serviceClose(&g_irsSrv);
    shmemClose(&g_irsSharedmem);

    g_irsFunctionLevel.ir_sensor_function_level = 0x0;
}

Service* irsGetServiceSession(void) {
    return &g_irsSrv;
}

void* irsGetSharedmemAddr(void) {
    return shmemGetAddr(&g_irsSharedmem);
}

static inline IrsStatusManager *_irsGetStatusManager(void) {
    return (IrsStatusManager*)irsGetSharedmemAddr();
}

static Result _irsCameraEntryGet(IrsIrCameraHandle handle, IrsCameraEntry **out_entry) {
    IrsCameraEntry *entry;
    *out_entry = NULL;

    if (handle.player_number >= IRS_MAX_CAMERAS) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    entry = &g_irsCameras[handle.player_number];
    *out_entry = entry;

    return 0;
}

static void _irsCameraEntryFree(IrsCameraEntry *entry) {
    tmemClose(&entry->transfermem);
}

static bool _irsGetIrSensorAruidStatus(u32 *out) {
    u64 aruid = appletGetAppletResourceUserId();
    for (u32 i=0; i<0x5; i++) {
        IrsAruidFormat *aruid_format = &_irsGetStatusManager()->aruid_format[i];
        if (atomic_load_explicit(&aruid_format->ir_sensor_aruid, memory_order_acquire) == aruid) {
            *out = atomic_load_explicit(&aruid_format->ir_sensor_aruid_status, memory_order_acquire);
            return 1;
        }
    }

    return 0;
}

static bool _irsIsAppletForeground(void) {
    u32 status=0;
    bool flag = _irsGetIrSensorAruidStatus(&status);
    return flag==0 || (status & BIT(0));
}

static bool _irsIsLibraryAppletCallEnabled(IrsIrCameraHandle handle, IrsIrCameraInternalStatus status) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return 0;

    bool ret=0;
    if (status) {
        for (u32 i=0; i<IRS_MAX_CAMERAS; i++) {
            if (g_irsCameras[i].internal_status == status) break;
            if (i==IRS_MAX_CAMERAS-1) ret=1;
        }
    }

    g_irsCameras[handle.player_number].internal_status = status;

    return ret;
}

static void _irsSetInternalStatus(IrsIrCameraHandle handle, IrsIrCameraInternalStatus status) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return;

    g_irsCameras[handle.player_number].internal_status = status;
}

static bool _irsGetVersionCheckFlag(IrsIrCameraHandle handle) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return 0;

    return g_irsCameras[handle.player_number].version_check;
}

static void _irsSetVersionCheckFlag(IrsIrCameraHandle handle, bool flag) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return;

    g_irsCameras[handle.player_number].version_check = flag;
}

Result irsGetIrCameraStatus(IrsIrCameraHandle handle, IrsIrCameraStatus *out) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    IrsIrCameraStatus tmp = atomic_load_explicit(&_irsGetStatusManager()->device_format[handle.player_number].ir_camera_status, memory_order_acquire);
    if (tmp > IrsIrCameraStatus_Unconnected) return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    *out = tmp;

    return 0;
}

static Result _irsGetIrCameraInternalStatus(IrsIrCameraHandle handle, IrsIrCameraInternalStatus *out) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    u32 tmp = atomic_load_explicit(&_irsGetStatusManager()->device_format[handle.player_number].ir_camera_internal_status, memory_order_acquire);
    if (tmp > IrsIrCameraInternalStatus_Setting) return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    *out = tmp;

    return 0;
}

static Result _irsCheckInternalStatus(IrsIrCameraHandle handle) {
    Result rc=0;
    IrsIrCameraInternalStatus status;

    rc = _irsGetIrCameraInternalStatus(handle, &status);
    if (R_FAILED(rc)) return rc;

    bool flag = _irsIsLibraryAppletCallEnabled(handle, status);

    switch (status) {
        case IrsIrCameraInternalStatus_Stopped:
        case IrsIrCameraInternalStatus_Ready:
        break; // Leave rc at value 0 for success.

        case IrsIrCameraInternalStatus_Unknown2: // These are seperate with sdknso.
        case IrsIrCameraInternalStatus_Unknown3:
            rc = status == IrsIrCameraInternalStatus_Unknown2 ? MAKERESULT(205, 123) : MAKERESULT(205, 124);
            // sdknso would use errorResultShow() here with rc when flag is set, however we won't do so.
        break;

        case IrsIrCameraInternalStatus_Unknown4:
            rc = MAKERESULT(205, 161);
        break;

        case IrsIrCameraInternalStatus_FirmwareUpdateNeeded:
            if (flag) {
                HidLaControllerFirmwareUpdateArg arg;
                hidLaCreateControllerFirmwareUpdateArg(&arg);
                arg.enable_force_update = 1;
                rc = hidLaShowControllerFirmwareUpdate(&arg);

                if (R_FAILED(rc)) {
                    rc = R_VALUE(rc) == MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit) ? MAKERESULT(205, 125) : rc;
                    break;
                }
            }
        // fallthrough

        case IrsIrCameraInternalStatus_FirmwareVersionRequested:
        case IrsIrCameraInternalStatus_FirmwareVersionIsInvalid:
        case IrsIrCameraInternalStatus_Setting:
            rc = MAKERESULT(205, 160);
        break;

        default:
            rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
        break;
    }

    return rc;
}

Result irsGetIrCameraHandle(IrsIrCameraHandle *handle, HidNpadIdType id) {
    u32 tmp = id;
    return serviceDispatchInOut(&g_irsSrv, 311, tmp, *handle);
}

Result irsCheckFirmwareUpdateNecessity(IrsIrCameraHandle handle, bool *out) {
    if (hosversionBefore(4,0,0)) // sdknso didn't implement this until 4.x (the RequiredMcuVersion was also updated with that version).
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    IrsIrCameraStatus status;
    IrsIrCameraInternalStatus internal_status;

    if (!_irsIsAppletForeground()) return MAKERESULT(205, 150);

    rc = irsGetIrCameraStatus(handle, &status);
    if (R_FAILED(rc)) return rc;

    if (status == IrsIrCameraStatus_Available) {
        if (_irsGetVersionCheckFlag(handle)) {
            rc = _irsCheckFirmwareVersion(handle, g_irsRequiredMcuVersion);
            if (R_SUCCEEDED(rc)) _irsSetVersionCheckFlag(handle, false);
        }

        rc = _irsGetIrCameraInternalStatus(handle, &internal_status);
        if (R_SUCCEEDED(rc)) {
            bool flag;
            if (internal_status == IrsIrCameraInternalStatus_FirmwareVersionIsInvalid)
                flag = 1;
            else if (internal_status == IrsIrCameraInternalStatus_Ready || internal_status == IrsIrCameraInternalStatus_Setting)
                flag = 0;
            else if (internal_status == IrsIrCameraInternalStatus_Stopped)
                flag = 0;
            else
                rc = MAKERESULT(205, 150);

            if (R_SUCCEEDED(rc)) {
                _irsSetVersionCheckFlag(handle, true);
                *out = flag;
            }
        }
    }
    else if (status == IrsIrCameraStatus_Unsupported) {
        rc = MAKERESULT(205, 111);
    }
    else if (status == IrsIrCameraStatus_Unconnected) {
        rc = MAKERESULT(205, 110);
    }

    return rc;
}

Result irsGetImageProcessorStatus(IrsIrCameraHandle handle, IrsImageProcessorStatus *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    IrsIrCameraInternalStatus tmp;

    rc = _irsGetIrCameraInternalStatus(handle, &tmp);
    if (R_SUCCEEDED(rc)) {
        if (tmp == IrsIrCameraInternalStatus_FirmwareVersionRequested || tmp == IrsIrCameraInternalStatus_FirmwareVersionIsInvalid || tmp == IrsIrCameraInternalStatus_Ready)
            *out = IrsImageProcessorStatus_Stopped;
        else
            *out = IrsImageProcessorStatus_Running;
    }
    return rc;
}

static IrsProcessorState *_irsGetRingLifo(IrsIrCameraHandle handle, IrsIrSensorMode mode) {
    if (handle.player_number >= IRS_MAX_CAMERAS) return NULL;
    IrsDeviceFormat *device = &_irsGetStatusManager()->device_format[handle.player_number];
    u32 tmp = atomic_load_explicit(&device->ir_sensor_mode, memory_order_acquire);
    if (tmp != mode) return NULL;

    return &device->processor_state;
}

// sdknso has multiple funcs for this (for each *ProcessorState), but we'll just use one func instead.
static s32 _irsRingLifoRead(IrsProcessorState *lifo, void* out, s32 count, IrsValidationCb validation_cb, void* userdata, size_t entrysize, s64 max_entrycount) {
    IrsTeraPluginProcessorState tmpdata; // validation_cb is only used with TeraPluginProcessor.
    if (validation_cb && entrysize > sizeof(tmpdata)) return 0;

    s64 max_first_entryindex = 0-max_entrycount; // sdknso uses {inparam}-{max_entrycount}, but the inparam is always 0 anyway.
    u8 *out8 = (u8*)out;
    if (count <= 0) return 0;
    if (count > max_entrycount) count = max_entrycount; // sdknso does this in the callers, but we'll do it here instead.

    s32 total_entries=0;

    s64 sampling_number0=0, sampling_number1=0;
    s64 prev_samplenum=0;
    u32 num_samples=0;

    do {
        sampling_number0 = atomic_load_explicit(&lifo->start, memory_order_acquire);
        sampling_number1 = atomic_load_explicit(&lifo->start, memory_order_acquire);

        s64 start_samplenum = max_first_entryindex + sampling_number1;
        s64 max_prev_samplenum = sampling_number0 - (max_entrycount+1);
        s64 timediff = sampling_number1 - (s64)atomic_load_explicit(&lifo->count, memory_order_acquire);
        s64 tmp0 = timediff > start_samplenum ? timediff : start_samplenum;
        prev_samplenum = max_prev_samplenum > tmp0 ? max_prev_samplenum : tmp0;

        s64 tmp = sampling_number1 - prev_samplenum;
        if (tmp <= 0) break;
        num_samples = tmp;

        s64 entryindex = prev_samplenum + (s64)num_samples - 1;
        total_entries = 0;
        s64 entrycount = entryindex - prev_samplenum;

        if (entrycount >= 0) {
            entryindex = prev_samplenum + entrycount;
            s64 next_samplenum = prev_samplenum+(max_entrycount+1);

            for (s64 i=0; i<=entrycount; i++) {
                u8 *data_src = &lifo->data[((entryindex-i) % (max_entrycount+1)) * entrysize];

                if (validation_cb) memcpy(&tmpdata, data_src, entrysize);

                sampling_number0 = atomic_load_explicit(&lifo->start, memory_order_acquire);

                if (atomic_load_explicit(&lifo->count, memory_order_acquire) < num_samples || sampling_number0 <= prev_samplenum || next_samplenum <= sampling_number0) break;

                bool is_valid=true;
                if (validation_cb) is_valid = validation_cb(userdata, &tmpdata);
                if (is_valid) {
                    memcpy(&out8[total_entries*entrysize], data_src, entrysize);
                    total_entries++;
                }

                if (total_entries >= count) break;
            }
        }

        sampling_number0 = atomic_load_explicit(&lifo->start, memory_order_acquire);
    } while (sampling_number0 <= prev_samplenum || atomic_load_explicit(&lifo->count, memory_order_acquire) < num_samples || prev_samplenum+max_entrycount+1 <= sampling_number0);

    return total_entries;
}

static Result _irsActivateIrsensor(bool activate) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(&g_irsSrv, activate ? 302 : 303, AppletResourceUserId,
        .in_send_pid = true,
    );
}

static Result _irsGetIrsensorSharedMemoryHandle(Handle* handle_out) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(&g_irsSrv, 304, AppletResourceUserId,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _irsStopImageProcessor(IrsIrCameraHandle handle) {
    const struct {
        IrsIrCameraHandle handle;
        u64 AppletResourceUserId;
    } in = { handle, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 305, in,
        .in_send_pid = true,
    );
}

static Result _irsRunMomentProcessor(IrsIrCameraHandle handle, const IrsPackedMomentProcessorConfig *config) {
    const struct {
        IrsIrCameraHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
        IrsPackedMomentProcessorConfig config;
    } in = { handle, 0, appletGetAppletResourceUserId(), *config };

    return serviceDispatchIn(&g_irsSrv, 306, in,
        .in_send_pid = true,
    );
}

static Result _irsRunClusteringProcessor(IrsIrCameraHandle handle, const IrsPackedClusteringProcessorConfig *config) {
    const struct {
        IrsIrCameraHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
        IrsPackedClusteringProcessorConfig config;
    } in = { handle, 0, appletGetAppletResourceUserId(), *config };

    return serviceDispatchIn(&g_irsSrv, 307, in,
        .in_send_pid = true,
    );
}

static Result _irsRunImageTransferProcessor(IrsIrCameraHandle handle, const IrsPackedImageTransferProcessorConfig *config, TransferMemory *tmem) {
    const struct {
        IrsIrCameraHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
        IrsPackedImageTransferProcessorConfig config;
        u64 TransferMemory_size;
    } in = { handle, 0, appletGetAppletResourceUserId(), *config, tmem->size };

    return serviceDispatchIn(&g_irsSrv, 308, in,
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
}

static Result _irsGetImageTransferProcessorState(IrsIrCameraHandle handle, void* buffer, size_t size, IrsImageTransferProcessorState *state) {
    const struct {
        IrsIrCameraHandle handle;
        u64 AppletResourceUserId;
    } in = { handle, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_irsSrv, 309, in, *state,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

static Result _irsRunTeraPluginProcessor(IrsIrCameraHandle handle, const IrsPackedTeraPluginProcessorConfig *config) {
    const struct {
        IrsIrCameraHandle handle;
        IrsPackedTeraPluginProcessorConfig config;
        u64 AppletResourceUserId;
    } in = { handle, *config, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 310, in,
        .in_send_pid = true,
    );
}

static Result _irsRunPointingProcessor(IrsIrCameraHandle handle, const IrsPackedPointingProcessorConfig *config) {
    const struct {
        IrsIrCameraHandle handle;
        IrsPackedPointingProcessorConfig config;
        u64 AppletResourceUserId;
    } in = { handle, *config, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 312, in,
        .in_send_pid = true,
    );
}

static Result _irsSuspendImageProcessor(IrsIrCameraHandle handle) {
    const struct {
        IrsIrCameraHandle handle;
        u64 AppletResourceUserId;
    } in = { handle, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 313, in,
        .in_send_pid = true,
    );
}

static Result _irsCheckFirmwareVersion(IrsIrCameraHandle handle, IrsPackedMcuVersion version) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        IrsIrCameraHandle handle;
        IrsPackedMcuVersion version;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, version, 0, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 314, in,
        .in_send_pid = true,
    );
}

static Result _irsRunImageTransferExProcessor(IrsIrCameraHandle handle, const IrsPackedImageTransferProcessorExConfig *config, TransferMemory *tmem) {
    const struct {
        IrsIrCameraHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
        IrsPackedImageTransferProcessorExConfig config;
        u64 TransferMemory_size;
    } in = { handle, 0, appletGetAppletResourceUserId(), *config, tmem->size };

    return serviceDispatchIn(&g_irsSrv, 316, in,
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
}

static Result _irsRunIrLedProcessor(IrsIrCameraHandle handle, const IrsPackedIrLedProcessorConfig *config) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        IrsIrCameraHandle handle;
        IrsPackedIrLedProcessorConfig config;
        u64 AppletResourceUserId;
    } in = { handle, *config, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 317, in,
        .in_send_pid = true,
    );
}

static Result _irsStopImageProcessorAsync(IrsIrCameraHandle handle) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        IrsIrCameraHandle handle;
        u64 AppletResourceUserId;
    } in = { handle, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 318, in,
        .in_send_pid = true,
    );
}

static Result _irsActivateIrsensorWithFunctionLevel(IrsPackedFunctionLevel level) {
    const struct {
        IrsPackedFunctionLevel level;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { level, 0, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_irsSrv, 319, in,
        .in_send_pid = true,
    );
}

Result irsStopImageProcessor(IrsIrCameraHandle handle) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;

    if (!serviceIsActive(&g_irsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    _irsSetInternalStatus(handle, IrsIrCameraInternalStatus_Stopped);
    _irsSetVersionCheckFlag(handle, true);

    bool old_sysver = hosversionBefore(4,0,0);

    if (old_sysver)
        rc = _irsStopImageProcessor(handle);
    else
        rc = _irsStopImageProcessorAsync(handle);
    if (R_SUCCEEDED(rc)) entry->mode = IrsIrSensorMode_None;

    if (R_SUCCEEDED(rc) && !old_sysver) {
        for (u32 i=0; i<0x14d; i++) {
            IrsImageProcessorStatus status;
            irsGetImageProcessorStatus(handle, &status);
            if (status == IrsImageProcessorStatus_Stopped) break;
            svcSleepThread(15000000);
        }
    }

    _irsCameraEntryFree(entry);
    return rc;
}

Result irsStopImageProcessorAsync(IrsIrCameraHandle handle) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;

    if (!serviceIsActive(&g_irsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    _irsSetInternalStatus(handle, IrsIrCameraInternalStatus_Stopped);
    _irsSetVersionCheckFlag(handle, true);

    rc = _irsStopImageProcessorAsync(handle);
    if (R_SUCCEEDED(rc)) entry->mode = IrsIrSensorMode_None;

    _irsCameraEntryFree(entry);
    return rc;
}

Result irsRunMomentProcessor(IrsIrCameraHandle handle, const IrsMomentProcessorConfig *config) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedMomentProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.exposure_time = config->exposure_time;
    packed_config.light_target = config->light_target;
    packed_config.gain = config->gain;
    packed_config.is_negative_image_used = config->is_negative_image_used;
    packed_config.window_of_interest = config->window_of_interest;
    packed_config.required_mcu_version = g_irsRequiredMcuVersion;
    packed_config.preprocess = config->preprocess;
    packed_config.preprocess_intensity_threshold = config->preprocess_intensity_threshold;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && (entry->mode != IrsIrSensorMode_None && entry->mode != IrsIrSensorMode_MomentProcessor)) {
        rc = _irsSuspendImageProcessor(handle);
    }

    if (R_SUCCEEDED(rc)) rc = _irsRunMomentProcessor(handle, &packed_config);

    if (R_SUCCEEDED(rc)) {
        entry->mode = IrsIrSensorMode_MomentProcessor;
        entry->is_negative_image_used = packed_config.is_negative_image_used;
        entry->window_of_interest = packed_config.window_of_interest;
    }

    return rc;
}

Result irsGetMomentProcessorStates(IrsIrCameraHandle handle, IrsMomentProcessorState *states, s32 count, s32 *total_out) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    *total_out = 0;
    // sdknso would fill states with default values here, but we won't do that.

    rc = MAKERESULT(205, 160);
    if (!_irsIsAppletForeground()) return rc;

    Result rc2 = _irsCheckInternalStatus(handle);
    if (R_FAILED(rc2))
        return rc2;

    if (entry->mode != IrsIrSensorMode_MomentProcessor && entry->mode != IrsIrSensorMode_IrLedProcessor) return rc;
    IrsProcessorState *lifo = _irsGetRingLifo(handle, IrsIrSensorMode_MomentProcessor);
    if (lifo==NULL) return rc;
    rc = 0;

    *total_out = _irsRingLifoRead(lifo, states, count, NULL, NULL, sizeof(*states), 5);

    if (!*total_out) {
        IrsIrCameraStatus status;
        rc = irsGetIrCameraStatus(handle, &status);
        if (R_SUCCEEDED(rc)) rc = g_irsCameraStatusResults[status]; // sdknso would verify that status is within bounds first, but that's redundant since irsGetIrCameraStatus() already does so.
        return rc;
    }

    if (entry->window_of_interest.width != 320 || entry->window_of_interest.height != 240) { // sdknso doesn't check this but we will, since multiplying by 1.0f is pointless (when the width and height are the defaults checked here).
        float scale = 76800.0f / (float)(entry->window_of_interest.width * entry->window_of_interest.height); // 76800 == 320*240
        for (s32 statei=0; statei<*total_out; statei++) {
            for (s32 stati=0; stati<0x30; stati++) states[statei].statistic[stati].average_intensity *= scale;
        }
    }

    return rc;
}

IrsMomentStatistic irsCalculateMomentRegionStatistic(const IrsMomentProcessorState *state, IrsRect rect, s32 region_x, s32 region_y, s32 region_width, s32 region_height) {
    s16 width = rect.width / 8;
    s16 height = rect.height / 6;
    float widthf = (float)width;
    float heightf = (float)height;
    double sum0=0, sum1=0, sum2=0;

    // sdknso doesn't have this set of validation.
    if (region_x < 0) region_x = 0;
    if (region_y < 0) region_y = 0;
    if (region_x > 5) region_x = 5;
    if (region_y > 7) region_y = 7;
    if (region_x+region_width > 6) region_width = 6 - region_x;
    if (region_y+region_height > 8) region_height = 8 - region_y;

    if (region_width >= 1 && region_height >= 1) {
        for (s32 x=region_x; x<region_x+region_width; x++) {
            for (s32 y=region_y; y<region_y+region_height; y++) {
                const IrsMomentStatistic *stat = &state->statistic[y + x*8];
                float intensity = stat->average_intensity*widthf*heightf;
                sum0+= (double)(intensity*stat->centroid_x);
                sum1+= (double)intensity;
                sum2+= (double)(intensity*stat->centroid_y);
            }
        }
    }

    double tmp = sum1 / (double)(region_width*region_height*width*height);
    if (sum1 == 0.0f) {
        return (IrsMomentStatistic){.average_intensity = (float)tmp};
    }
    return (IrsMomentStatistic){.average_intensity = (float)tmp, .centroid_x = (float)(sum0 / sum1), .centroid_y = (float)(sum2 / sum1)};
}

Result irsRunClusteringProcessor(IrsIrCameraHandle handle, const IrsClusteringProcessorConfig *config) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedClusteringProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.exposure_time = config->exposure_time;
    packed_config.light_target = config->light_target;
    packed_config.gain = config->gain;
    packed_config.is_negative_image_used = config->is_negative_image_used;
    packed_config.window_of_interest = config->window_of_interest;
    packed_config.required_mcu_version = g_irsRequiredMcuVersion;
    packed_config.object_pixel_count_min = config->object_pixel_count_min;
    packed_config.object_pixel_count_max = config->object_pixel_count_max;
    packed_config.object_intensity_min = config->object_intensity_min;
    packed_config.is_external_light_filter_enabled = config->is_external_light_filter_enabled;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && (entry->mode != IrsIrSensorMode_None && entry->mode != IrsIrSensorMode_ClusteringProcessor)) {
        rc = _irsSuspendImageProcessor(handle);
    }

    if (R_SUCCEEDED(rc)) rc = _irsRunClusteringProcessor(handle, &packed_config);

    if (R_SUCCEEDED(rc)) {
        entry->mode = IrsIrSensorMode_ClusteringProcessor;
        entry->is_negative_image_used = packed_config.is_negative_image_used;
        entry->window_of_interest = packed_config.window_of_interest;
    }

    return rc;
}

Result irsGetClusteringProcessorStates(IrsIrCameraHandle handle, IrsClusteringProcessorState *states, s32 count, s32 *total_out) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    *total_out = 0;
    // sdknso would fill states with default values here, but we won't do that.

    rc = MAKERESULT(205, 160);
    if (!_irsIsAppletForeground()) return rc;

    Result rc2 = _irsCheckInternalStatus(handle);
    if (R_FAILED(rc2))
        return rc2;

    if (entry->mode != IrsIrSensorMode_ClusteringProcessor) return rc;
    IrsProcessorState *lifo = _irsGetRingLifo(handle, IrsIrSensorMode_ClusteringProcessor);
    if (lifo==NULL) return rc;
    rc = 0;

    *total_out = _irsRingLifoRead(lifo, states, count, NULL, NULL, sizeof(*states), 5);

    if (!*total_out) {
        IrsIrCameraStatus status;
        rc = irsGetIrCameraStatus(handle, &status);
        if (R_SUCCEEDED(rc)) rc = g_irsCameraStatusResults[status]; // sdknso would verify that status is within bounds first, but that's redundant since irsGetIrCameraStatus() already does so.
    }

    return rc;
}

Result irsRunImageTransferProcessor(IrsIrCameraHandle handle, const IrsImageTransferProcessorConfig *config, size_t size) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedImageTransferProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.exposure_time = config->exposure_time;
    packed_config.light_target = config->light_target;
    packed_config.gain = config->gain;
    packed_config.is_negative_image_used = config->is_negative_image_used;
    packed_config.required_mcu_version = g_irsRequiredMcuVersion;
    packed_config.format = config->format;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && entry->mode != IrsIrSensorMode_None) {
        rc = _irsSuspendImageProcessor(handle);
        if (R_SUCCEEDED(rc)) _irsCameraEntryFree(entry);
    }

    if (R_SUCCEEDED(rc)) {
        rc = tmemCreate(&entry->transfermem, size, Perm_None);
        if (R_FAILED(rc)) return rc;

        rc = _irsRunImageTransferProcessor(handle, &packed_config, &entry->transfermem);
    }

    if (R_SUCCEEDED(rc)) {
        entry->mode = IrsIrSensorMode_ImageTransferProcessor;
        entry->is_negative_image_used = packed_config.is_negative_image_used;
        entry->format = packed_config.format;
    }

    if (R_FAILED(rc)) _irsCameraEntryFree(entry);

    return rc;
}

Result irsRunImageTransferExProcessor(IrsIrCameraHandle handle, const IrsImageTransferProcessorExConfig *config, size_t size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedImageTransferProcessorExConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.exposure_time = config->exposure_time;
    packed_config.light_target = config->light_target;
    packed_config.gain = config->gain;
    packed_config.is_negative_image_used = config->is_negative_image_used;
    packed_config.required_mcu_version = g_irsRequiredMcuVersion;
    packed_config.orig_format = config->orig_format;
    packed_config.trimming_format = config->trimming_format;
    packed_config.trimming_start_x = config->trimming_start_x;
    packed_config.trimming_start_y = config->trimming_start_y;
    packed_config.is_external_light_filter_enabled = config->is_external_light_filter_enabled;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && entry->mode != IrsIrSensorMode_None) {
        rc = _irsSuspendImageProcessor(handle);
        if (R_SUCCEEDED(rc)) _irsCameraEntryFree(entry);
    }

    if (R_SUCCEEDED(rc)) {
        rc = tmemCreate(&entry->transfermem, size, Perm_None);
        if (R_FAILED(rc)) return rc;

        rc = _irsRunImageTransferExProcessor(handle, &packed_config, &entry->transfermem);
    }

    if (R_SUCCEEDED(rc)) {
        entry->mode = IrsIrSensorMode_ImageTransferProcessor;
        entry->is_negative_image_used = packed_config.is_negative_image_used;
        entry->format = packed_config.trimming_format;
    }

    if (R_FAILED(rc)) _irsCameraEntryFree(entry);

    return rc;
}

Result irsGetImageTransferProcessorState(IrsIrCameraHandle handle, void* buffer, size_t size, IrsImageTransferProcessorState *state) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;

    if (!_irsIsAppletForeground()) return MAKERESULT(205, 160);

    rc = _irsCheckInternalStatus(handle);
    if (R_FAILED(rc))
        return rc;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    if (entry->mode != IrsIrSensorMode_ImageTransferProcessor) return MAKERESULT(205, 160);

    rc = _irsGetImageTransferProcessorState(handle, buffer, size, state);

    if (R_SUCCEEDED(rc) && entry->is_negative_image_used) {
        if (entry->format >= sizeof(g_irsImageFormatSizes)/sizeof(g_irsImageFormatSizes[0]))
            rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);

        if (R_SUCCEEDED(rc)) {
            u8 *bufptr = (u8*)buffer;
            size_t tmpsize = g_irsImageFormatSizes[entry->format];
            if (tmpsize > size) tmpsize = size;

            for (size_t i=0; i<tmpsize; i++) bufptr[i] = ~bufptr[i];
        }
    }

    if (R_FAILED(rc)) {
        IrsIrCameraStatus status;
        rc = irsGetIrCameraStatus(handle, &status);
        if (R_SUCCEEDED(rc)) rc = g_irsCameraStatusResults[status]; // sdknso would verify that status is within bounds first, but that's redundant since irsGetIrCameraStatus() already does so.
    }

    return rc;
}

Result irsRunPointingProcessor(IrsIrCameraHandle handle) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedPointingProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.window_of_interest.width = 320;
    packed_config.window_of_interest.height = 240;
    packed_config.required_mcu_version = g_irsRequiredMcuVersion;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && (entry->mode != IrsIrSensorMode_None && entry->mode != IrsIrSensorMode_PointingProcessor)) {
        rc = _irsSuspendImageProcessor(handle);
    }

    if (R_SUCCEEDED(rc)) rc = _irsRunPointingProcessor(handle, &packed_config);

    if (R_SUCCEEDED(rc)) entry->mode = IrsIrSensorMode_PointingProcessor;

    return rc;
}

Result irsGetPointingProcessorMarkerStates(IrsIrCameraHandle handle, IrsPointingProcessorMarkerState *states, s32 count, s32 *total_out) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    *total_out = 0;
    // sdknso would fill states with default values here, but we won't do that.

    rc = MAKERESULT(205, 160);
    if (!_irsIsAppletForeground()) return rc;

    Result rc2 = _irsCheckInternalStatus(handle);
    if (R_FAILED(rc2))
        return rc2;

    if (entry->mode != IrsIrSensorMode_PointingProcessor) return rc;
    IrsProcessorState *lifo = _irsGetRingLifo(handle, IrsIrSensorMode_PointingProcessor);
    if (lifo==NULL) return rc;
    rc = 0;

    *total_out = _irsRingLifoRead(lifo, states, count, NULL, NULL, sizeof(*states), 6);

    if (!*total_out) {
        IrsIrCameraStatus status;
        rc = irsGetIrCameraStatus(handle, &status);
        if (R_SUCCEEDED(rc)) rc = g_irsCameraStatusResults[status]; // sdknso would verify that status is within bounds first, but that's redundant since irsGetIrCameraStatus() already does so.
    }

    return rc;
}

Result irsGetPointingProcessorStates(IrsIrCameraHandle handle, IrsPointingProcessorState *states, s32 count, s32 *total_out) {
    Result rc=0;
    IrsPointingProcessorMarkerState tmp_states[6];

    *total_out = 0;
    rc = irsGetPointingProcessorMarkerStates(handle, tmp_states, count, total_out);

    if (R_SUCCEEDED(rc)) { // sdknso doesn't check this, but we will.
        for (s32 i=0; i<*total_out; i++) {
            float pos_x = 0.0f, pos_y = 0.0f;
            u32 poscount=0;

            states[i].sampling_number = tmp_states[i].sampling_number;
            states[i].timestamp = tmp_states[i].timestamp;

            for (u32 pointi=0; pointi<3; pointi++) {
                if (tmp_states[i].data[pointi].pointing_status) {
                    pos_x+= tmp_states[i].data[pointi].position_x;
                    pos_y+= tmp_states[i].data[pointi].position_y;
                    poscount++;
                }
            }

            states[i].pointing_status = poscount < 3;

            if (!poscount) {
                states[i].position_x = 0.0f;
                states[i].position_y = 0.0f;
            }
            else {
                states[i].position_x = (pos_x / (float)poscount / -160.0f) + 1.0f;
                states[i].position_y = (pos_y / (float)poscount / 120.0f) + 1.0f;
            }
        }
    }

    return rc;
}

Result irsRunTeraPluginProcessor(IrsIrCameraHandle handle, const IrsTeraPluginProcessorConfig *config) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedTeraPluginProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.required_mcu_version = g_irsRequiredMcuVersion;
    packed_config.mode = config->mode;

    if (hosversionAtLeast(6,0,0)) {
        packed_config.unk_x5 = 0x2 | (config->unk_x1 << 7);
        packed_config.unk_x6 = config->unk_x2;
        packed_config.unk_x7 = config->unk_x3;
    }

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && entry->mode != IrsIrSensorMode_None) {
        rc = _irsSuspendImageProcessor(handle);
    }

    // sdknso would assert here when g_irsFunctionLevel.ir_sensor_function_level is >= {certain value} - but that can't happen since it's above the value set during init, so we won't impl that.

    if (R_SUCCEEDED(rc)) rc = _irsRunTeraPluginProcessor(handle, &packed_config);

    if (R_SUCCEEDED(rc)) entry->mode = IrsIrSensorMode_TeraPluginProcessor;

    return rc;
}

static bool _irsValidateTeraPluginProcessorState(void* userdata, void* arg) {
    IrsTeraFilterArg *filter = (IrsTeraFilterArg*)userdata;
    IrsTeraPluginProcessorState *state = arg;

    // sdknso would call a parsing func here, but the output from it is unused so we won't impl that.

    for (u32 i=0; i<filter->prefix_bitcount; i++) {
        u8 data = state->plugin_data[i>>3] >> (i & 0x7);
        u8 prefix = filter->prefix_data >> i;
        if ((data & 1) != (prefix & 1)) return false;
    }

    return state->sampling_number >= filter->sampling_number;
}

Result irsGetTeraPluginProcessorStates(IrsIrCameraHandle handle, IrsTeraPluginProcessorState *states, s32 count, s64 sampling_number, u32 prefix_data, u32 prefix_bitcount, s32 *total_out) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsTeraFilterArg userdata={.sampling_number = sampling_number, .prefix_data = prefix_data, .prefix_bitcount = prefix_bitcount};

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;

    *total_out = 0;
    // sdknso would fill states with default values here, but we won't do that.

    rc = MAKERESULT(205, 160);
    if (!_irsIsAppletForeground()) return rc;

    Result rc2 = _irsCheckInternalStatus(handle);
    if (R_FAILED(rc2))
        return rc2;

    if (entry->mode != IrsIrSensorMode_TeraPluginProcessor) return rc;
    IrsProcessorState *lifo = _irsGetRingLifo(handle, IrsIrSensorMode_TeraPluginProcessor);
    if (lifo==NULL) return rc;
    rc = 0;

    *total_out = _irsRingLifoRead(lifo, states, count, _irsValidateTeraPluginProcessorState, &userdata, sizeof(*states), 5);

    if (!*total_out) {
        IrsIrCameraStatus status;
        rc = irsGetIrCameraStatus(handle, &status);
        if (R_SUCCEEDED(rc)) rc = g_irsCameraStatusResults[status]; // sdknso would verify that status is within bounds first, but that's redundant since irsGetIrCameraStatus() already does so.
    }

    return rc;
}

Result irsRunIrLedProcessor(IrsIrCameraHandle handle, const IrsIrLedProcessorConfig *config) {
    Result rc=0;
    IrsCameraEntry *entry = NULL;
    IrsPackedIrLedProcessorConfig packed_config;

    memset(&packed_config, 0, sizeof(packed_config));

    packed_config.required_mcu_version = g_irsRequiredMcuVersion;
    packed_config.light_target = config->light_target;

    rc = _irsCameraEntryGet(handle, &entry);
    if (R_FAILED(rc))
        return rc;
    entry->handle = handle;

    if (g_irsFunctionLevel.ir_sensor_function_level >= 0x1 && (entry->mode != IrsIrSensorMode_None && entry->mode != IrsIrSensorMode_IrLedProcessor)) {
        rc = _irsSuspendImageProcessor(handle);
    }

    if (R_SUCCEEDED(rc)) rc = _irsRunIrLedProcessor(handle, &packed_config);

    if (R_SUCCEEDED(rc)) entry->mode = IrsIrSensorMode_IrLedProcessor;

    return rc;
}

Result irsRunAdaptiveClusteringProcessor(IrsIrCameraHandle handle, const IrsAdaptiveClusteringProcessorConfig *config) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IrsTeraPluginProcessorConfig tmp_config={0};

    tmp_config.mode = config->mode == 1 ? 0x10 : 0xf;

    if (hosversionAtLeast(6,0,0)) {
        IrsAdaptiveClusteringTargetDistance tmp = config->target_distance;
        // sdknso would set some tmp_config fields to 0 again in some cases, but we won't do that.
        if (tmp == IrsAdaptiveClusteringTargetDistance_Middle) {
            tmp_config.unk_x1 = 0x1;
            tmp_config.unk_x2 = 0x3;
        }
        else if (tmp == IrsAdaptiveClusteringTargetDistance_Far) {
            tmp_config.unk_x1 = 0x1;
            tmp_config.unk_x2 = 0x8;
        }
        else if (tmp != IrsAdaptiveClusteringTargetDistance_Near)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    return irsRunTeraPluginProcessor(handle, &tmp_config);
}

// sdknso DecodeMarkerDetectionState (which is called by GetAdaptiveClusteringProcessorStates) uses nerd_gillette_internal* functionality.

Result irsRunHandAnalysis(IrsIrCameraHandle handle, const IrsHandAnalysisConfig *config) {
    IrsTeraPluginProcessorConfig tmp_config={0};

    u32 mode = config->mode;

    if (mode < IrsHandAnalysisMode_Silhouette || mode > IrsHandAnalysisMode_SilhouetteOnly)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (hosversionBefore(4,0,0)) {
        if (mode == IrsHandAnalysisMode_SilhouetteOnly)
            return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
        tmp_config.mode = mode-0x1;
    }
    else {
        tmp_config.mode = mode != IrsHandAnalysisMode_SilhouetteOnly ? mode+0x4 : 0xa;
    }

    return irsRunTeraPluginProcessor(handle, &tmp_config);
}

// The remaining HandAnalysis funcs in sdknso uses nerd_gillette_internal* functionality.

void irsGetMomentProcessorDefaultConfig(IrsMomentProcessorConfig *config) {
    memset(config, 0, sizeof(*config));

    config->exposure_time = 300000;
    config->gain = 8;
    config->window_of_interest.width = 320;
    config->window_of_interest.height = 240;
    config->preprocess = 1;
    config->preprocess_intensity_threshold = 0x50;
}

void irsGetClusteringProcessorDefaultConfig(IrsClusteringProcessorConfig *config) {
    memset(config, 0, sizeof(*config));

    config->exposure_time = 200000;
    config->gain = 2;
    config->window_of_interest.width = 320;
    config->window_of_interest.height = 240;
    config->object_pixel_count_min = 0x3;
    config->object_pixel_count_max = 0x12C00;
    config->object_intensity_min = 150;
    config->is_external_light_filter_enabled = 1;
}

void irsGetDefaultImageTransferProcessorConfig(IrsImageTransferProcessorConfig *config) {
    memset(config, 0, sizeof(*config));

    config->exposure_time = 300000;
    config->gain = 8;
}

void irsGetDefaultImageTransferProcessorExConfig(IrsImageTransferProcessorExConfig *config) {
    memset(config, 0, sizeof(*config));

    config->exposure_time = 300000;
    config->gain = 8;
}

