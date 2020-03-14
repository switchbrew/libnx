#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <stdatomic.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "kernel/shmem.h"
#include "services/hidbus.h"
#include "services/applet.h"
#include "applets/hid_la.h"

typedef struct {
    Mutex mutex;
    Event event;
    HidbusBusHandle handle;
    void* workbuf;
} HidBusDeviceEntry;

static HidBusDeviceEntry g_hidbusDevices[0x13];

static Mutex g_hidbusSharedmemMutex, g_hidbusUpdateMutex;
static bool g_hidbusUpdateFlag;
static u32 g_hidbusSharedmemRefCount;
static SharedMemory g_hidbusSharedmem;

Result hidbusGetServiceSession(Service* srv_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(srv_out, "hidbus");
}

static Result _hidBusVerifyBusHandle(HidbusBusHandle handle) {
    u8 max_count = hosversionBefore(6,0,0) ? 0x10 : 0x13; // sdknso uses value 0x11 on 5.x, but that's off-by-one with sharedmem because 0x10*sizeof(HidbusStatusManagerEntryV5) == sizeof(sharedmem). Hence, we check for 0x10 instead.

    return handle.internal_index < max_count ? 0 : MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
}

static Result _hidbusCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _hidbusGetBusHandle(Service* srv, u32 id, u64 bus_type, bool *flag, HidbusBusHandle *handle) {
    const struct {
        u32 id;
        u32 pad;
        u64 bus_type;
        u64 appletResourceUserId;
    } in = { id, 0, bus_type, appletGetAppletResourceUserId() };

    struct {
        u8 flag;
        u8 pad[7];
        HidbusBusHandle handle;
    } out;

    Result rc = serviceDispatchInOut(srv, 1, in, out);
    if (R_SUCCEEDED(rc)) {
        if (flag) *flag = out.flag & 1;
        if (handle) *handle = out.handle;
    }
    return rc;
}

static Result _hidbusInBusHandleResIdNoOut(Service* srv, HidbusBusHandle handle, u32 cmd_id) {
    const struct {
        HidbusBusHandle handle;
        u64 appletResourceUserId;
    } in = { handle, appletGetAppletResourceUserId() };

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _hidbusEnableExternalDevice(Service* srv, HidbusBusHandle handle, bool flag, u64 inval) {
    const struct {
        u8 flag;
        u8 pad[7];
        HidbusBusHandle handle;
        u64 inval;
        u64 appletResourceUserId;
    } in = { flag!=0, {0}, handle, inval, appletGetAppletResourceUserId() };

    return serviceDispatchIn(srv, 5, in);
}

static Result _hidbusGetExternalDeviceId(Service* srv, HidbusBusHandle handle, u32 *out) {
    return serviceDispatchInOut(srv, 6, handle, *out);
}

static Result _hidbusSendCommandAsync(Service* srv, HidbusBusHandle handle, const void* buffer, size_t size) {
    return serviceDispatchIn(srv, 7, handle,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _hidbusGetSendCommandAsynceResult(Service* srv, HidbusBusHandle handle, void* buffer, size_t size, u32 *out_size) {
    return serviceDispatchInOut(srv, 8, handle, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static Result _hidbusSetEventForSendCommandAsycResult(Service* srv, HidbusBusHandle handle, Event* out_event) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(srv, 9, handle,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, false);

    return rc;
}

static Result _hidbusEnableJoyPollingReceiveMode(Service* srv, HidbusBusHandle handle, u32 polling_mode, const void* inbuf, size_t inbuf_size, TransferMemory *tmem) { // [8.0.0+]
    const struct {
        u32 size;
        u32 polling_mode;
        HidbusBusHandle handle;
    } in = { tmem->size, polling_mode, handle };

    Result rc = serviceDispatchIn(srv, 11, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { inbuf, inbuf_size } },
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );

    return rc;
}

static Result _hidbusDisableJoyPollingReceiveMode(Service* srv, HidbusBusHandle handle) {
    return serviceDispatchIn(srv, 12, handle);
}

static Result _hidbusSetStatusManagerType(Service* srv, u32 inval) { // [6.0.0+]
    return serviceDispatchIn(srv, 14, inval);
}

// Official sw just checks whether a global flag is set, and runs initialization if not set. The code for cleanup is not used. Since we want to actually handle cleanup, use refcounting for sharedmem.
static Result _hidbusSharedmemInitialize(Service* srv) {
    Result rc=0;
    Handle sharedmem_handle;
    mutexLock(&g_hidbusSharedmemMutex);
    if ((g_hidbusSharedmemRefCount++) == 0) {
        rc = _hidbusCmdGetHandle(srv, &sharedmem_handle, 10); // GetSharedMemoryHandle
        if (R_SUCCEEDED(rc)) {
            shmemLoadRemote(&g_hidbusSharedmem, sharedmem_handle, 0x1000, Perm_R);
            rc = shmemMap(&g_hidbusSharedmem);
            if (R_FAILED(rc)) shmemClose(&g_hidbusSharedmem);
        }
        if (R_FAILED(rc)) g_hidbusSharedmemRefCount--;
        _hidbusSetStatusManagerType(srv, 0x2); // Official sw ignores errors from this.
    }
    mutexUnlock(&g_hidbusSharedmemMutex);

    return rc;
}

static void _hidbusSharedmemExit(void) {
    mutexLock(&g_hidbusSharedmemMutex);
    if (g_hidbusSharedmemRefCount && (--g_hidbusSharedmemRefCount) == 0) {
        shmemClose(&g_hidbusSharedmem);
    }
    mutexUnlock(&g_hidbusSharedmemMutex);
}

void* hidbusGetSharedmemAddr(void) {
    return shmemGetAddr(&g_hidbusSharedmem);
}

static HidbusStatusManagerEntryCommon* _hidbusGetStatusManagerEntryCommon(u8 internal_index) {
    if (hosversionBefore(6,0,0))
        return &((HidbusStatusManagerV5*)hidbusGetSharedmemAddr())->entries[internal_index].common;
    else
        return &((HidbusStatusManager*)hidbusGetSharedmemAddr())->entries[internal_index].common;
}

static bool _hidbusGetStatusManagerEntryFlag_x0(u8 internal_index) {
    return atomic_load_explicit(&_hidbusGetStatusManagerEntryCommon(internal_index)->flag_x0, memory_order_acquire) & 1;
}

static Result _hidbusGetStatusManagerEntryRes(u8 internal_index) {
    return _hidbusGetStatusManagerEntryCommon(internal_index)->res;
}

static bool _hidbusGetStatusManagerEntryDeviceEnabled(u8 internal_index) {
    return atomic_load_explicit(&_hidbusGetStatusManagerEntryCommon(internal_index)->device_enabled, memory_order_acquire) & 1;
}

static bool _hidbusGetStatusManagerEntryIsValid(u8 internal_index) {
    return atomic_load_explicit(&_hidbusGetStatusManagerEntryCommon(internal_index)->is_valid, memory_order_acquire) & 1;
}

static bool _hidbusGetStatusManagerEntryPollingEnabled(u8 internal_index) {
    return atomic_load_explicit(&_hidbusGetStatusManagerEntryCommon(internal_index)->polling_enabled, memory_order_acquire) & 1;
}

static HidbusJoyPollingMode _hidbusGetStatusManagerEntryPollingMode(u8 internal_index) {
    return atomic_load_explicit(&_hidbusGetStatusManagerEntryCommon(internal_index)->polling_mode, memory_order_acquire);
}

Result hidbusGetBusHandle(HidbusBusHandle *handle, bool *flag, HidControllerID id, HidbusBusType bus_type) {
    Service srv={0};
    Result rc = hidbusGetServiceSession(&srv);
    *flag = 0;
    if (R_FAILED(rc)) return rc;

    HidbusBusHandle tmphandle={0};
    bool tmpflag=0;
    rc = _hidbusGetBusHandle(&srv, hidControllerIDToOfficial(id), bus_type, &tmpflag, &tmphandle);
    if (R_SUCCEEDED(rc)) {
        if (!tmpflag) *flag = tmpflag;
        else {
            rc = _hidBusVerifyBusHandle(tmphandle);
            if (R_SUCCEEDED(rc)) {
                *flag = tmpflag;
                *handle = tmphandle;

                g_hidbusDevices[tmphandle.internal_index].handle = tmphandle;
            }
        }
    }

    serviceClose(&srv);
    return rc;
}

Result hidbusInitialize(HidbusBusHandle handle) {
    Service srv={0};
    bool setup_event=1;
    bool sharedmem_init=0;
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    HidBusDeviceEntry *entry = &g_hidbusDevices[handle.internal_index];
    mutexLock(&entry->mutex);
    if (memcmp(&entry->handle, &handle, sizeof(handle))!=0) rc = MAKERESULT(218, 4);

    if (R_SUCCEEDED(rc)) rc = hidbusGetServiceSession(&srv);

    if (R_SUCCEEDED(rc)) {
        rc = _hidbusSharedmemInitialize(&srv);
        if (R_SUCCEEDED(rc)) sharedmem_init = 1;
    }

    if (R_SUCCEEDED(rc)) {
        rc = _hidbusInBusHandleResIdNoOut(&srv, handle, 3); // Initialize
        if (R_VALUE(rc) == MAKERESULT(218, 10)) {
            rc = 0;
            setup_event = 0;
        }
    }

    if (R_SUCCEEDED(rc) && setup_event) {
        eventClose(&entry->event);
        rc = _hidbusSetEventForSendCommandAsycResult(&srv, handle, &entry->event);
    }

    if ((R_FAILED(rc) || !setup_event) && sharedmem_init) _hidbusSharedmemExit();

    mutexUnlock(&entry->mutex);
    serviceClose(&srv);
    return rc;
}

Result hidbusFinalize(HidbusBusHandle handle) {
    Service srv={0};
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    HidBusDeviceEntry *entry = &g_hidbusDevices[handle.internal_index];
    mutexLock(&entry->mutex);

    if (memcmp(&entry->handle, &handle, sizeof(handle))==0) {
        rc = hidbusGetServiceSession(&srv);

        if (R_SUCCEEDED(rc)) rc = _hidbusInBusHandleResIdNoOut(&srv, handle, 4); // Finalize
        eventClose(&entry->event);

        _hidbusSharedmemExit();
    }

    mutexUnlock(&entry->mutex);
    serviceClose(&srv);
    return rc;
}

Result hidbusEnableExternalDevice(HidbusBusHandle handle, bool flag, u32 device_id) {
    Service srv={0};
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    u64 inval = hosversionBefore(7,0,0) ? 0x38900050018 : 0x3A600050018;
    u32 index = handle.internal_index;
    HidBusDeviceEntry *entry = &g_hidbusDevices[index];
    mutexLock(&entry->mutex);
    if (memcmp(&entry->handle, &handle, sizeof(handle))!=0) rc = MAKERESULT(218, 4);

    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryIsValid(index)) rc = MAKERESULT(218, 2);
    if (R_SUCCEEDED(rc)) rc = _hidbusGetStatusManagerEntryRes(index);
    if (R_SUCCEEDED(rc)) {
        if (!_hidbusGetStatusManagerEntryFlag_x0(index) && !flag) rc = MAKERESULT(218, 5);
    }

    if (R_SUCCEEDED(rc)) rc = hidbusGetServiceSession(&srv);

    if (R_SUCCEEDED(rc)) {
        rc = _hidbusEnableExternalDevice(&srv, handle, flag, inval);
        if (R_FAILED(rc)) {
            // sdknso asserts when rc is MAKERESULT(218, 12), we won't do an equivalent.
            if (R_VALUE(rc) == MAKERESULT(202, 547) || R_VALUE(rc) == MAKERESULT(108, 426)) {
                mutexLock(&g_hidbusUpdateMutex);
                bool updateflag = g_hidbusUpdateFlag;
                if (!updateflag) g_hidbusUpdateFlag = true;
                mutexUnlock(&g_hidbusUpdateMutex);
                if (updateflag) rc = MAKERESULT(218, 2);
                else {
                    HidLaControllerFirmwareUpdateArg arg;
                    hidLaCreateControllerFirmwareUpdateArg(&arg);
                    arg.enable_force_update = 1;
                    rc = hidLaShowControllerFirmwareUpdate(&arg);

                    if (R_FAILED(rc)) rc = R_VALUE(rc) == MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit) ? MAKERESULT(218, 3) : rc;
                    else rc = _hidbusEnableExternalDevice(&srv, handle, flag, inval);

                    mutexLock(&g_hidbusUpdateMutex);
                    g_hidbusUpdateFlag = false;
                    mutexUnlock(&g_hidbusUpdateMutex);
                }
            }
        }
        if (R_SUCCEEDED(rc) && flag) {
            u32 tmpout=0;
            rc = _hidbusGetExternalDeviceId(&srv, handle, &tmpout);
            if (R_SUCCEEDED(rc) && tmpout!=device_id) {
                rc = hidbusEnableExternalDevice(handle, false, device_id);
                rc = R_SUCCEEDED(rc) ? MAKERESULT(218, 9) : rc;
            }
        }
    }

    mutexUnlock(&entry->mutex);
    serviceClose(&srv);
    return rc;
}

Result hidbusSendAndReceive(HidbusBusHandle handle, const void* inbuf, size_t inbuf_size, void* outbuf, size_t outbuf_size, u64 *out_size) {
    Service srv={0};
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    if (inbuf_size >= 0x26) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    u32 index = handle.internal_index;
    HidBusDeviceEntry *entry = &g_hidbusDevices[index];
    mutexLock(&entry->mutex);
    if (memcmp(&entry->handle, &handle, sizeof(handle))!=0) rc = MAKERESULT(218, 4);

    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryDeviceEnabled(index)) rc = MAKERESULT(218, 8);

    if (R_SUCCEEDED(rc)) rc = hidbusGetServiceSession(&srv);

    if (R_SUCCEEDED(rc)) {
        eventClear(&entry->event); // This was added with sdknso 6.x+, but we'll do it on 5.x regardless.
        rc = _hidbusSendCommandAsync(&srv, handle, inbuf, inbuf_size);
    }
    if (R_SUCCEEDED(rc)) {
        eventWait(&entry->event, U64_MAX);
        eventClear(&entry->event);
        u32 tmpout=0;
        rc = _hidbusGetSendCommandAsynceResult(&srv, handle, outbuf, outbuf_size, &tmpout);
        if (R_SUCCEEDED(rc) && out_size) *out_size = tmpout;
    }

    mutexUnlock(&entry->mutex);
    serviceClose(&srv);
    return rc;
}

Result hidbusEnableJoyPollingReceiveMode(HidbusBusHandle handle, const void* inbuf, size_t inbuf_size, void* workbuf, size_t workbuf_size, HidbusJoyPollingMode polling_mode) {
    Service srv={0};
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    if (inbuf_size >= 0x26) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    u32 index = handle.internal_index;
    HidBusDeviceEntry *entry = &g_hidbusDevices[index];
    mutexLock(&entry->mutex);
    if (memcmp(&entry->handle, &handle, sizeof(handle))!=0) rc = MAKERESULT(218, 4);

    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryDeviceEnabled(index)) rc = MAKERESULT(218, 8);

    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryPollingEnabled(index)) {
        rc = hidbusGetServiceSession(&srv);

        if (R_SUCCEEDED(rc)) {
            TransferMemory tmem={0};
            rc = tmemCreateFromMemory(&tmem, workbuf, workbuf_size, Perm_R);
            if (R_SUCCEEDED(rc)) rc = _hidbusEnableJoyPollingReceiveMode(&srv, handle, polling_mode, inbuf, inbuf_size, &tmem);
            if (R_SUCCEEDED(rc)) entry->workbuf = workbuf; // sdknso does this before using the cmd.
            tmemClose(&tmem);
        }
    }

    mutexUnlock(&entry->mutex);
    serviceClose(&srv);
    return rc;
}

Result hidbusDisableJoyPollingReceiveMode(HidbusBusHandle handle) {
    Service srv={0};
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    u32 index = handle.internal_index;
    HidBusDeviceEntry *entry = &g_hidbusDevices[index];
    mutexLock(&entry->mutex);
    if (memcmp(&entry->handle, &handle, sizeof(handle))!=0) rc = MAKERESULT(218, 4);

    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryDeviceEnabled(index)) rc = MAKERESULT(218, 8);

    if (R_SUCCEEDED(rc)) rc = hidbusGetServiceSession(&srv);

    if (R_SUCCEEDED(rc)) rc = _hidbusDisableJoyPollingReceiveMode(&srv, handle);

    mutexUnlock(&entry->mutex);
    serviceClose(&srv);
    return rc;
}

Result hidbusGetJoyPollingReceivedData(HidbusBusHandle handle, HidbusJoyPollingReceivedData *recv_data, s32 count) {
    Result rc = _hidBusVerifyBusHandle(handle);
    if (R_FAILED(rc)) return rc;
    u32 index = handle.internal_index;
    HidBusDeviceEntry *entry = &g_hidbusDevices[index];
    if (memcmp(&entry->handle, &handle, sizeof(handle))!=0) rc = MAKERESULT(218, 4);

    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryDeviceEnabled(index)) rc = MAKERESULT(218, 8);

    if (R_SUCCEEDED(rc)) rc = _hidbusGetStatusManagerEntryRes(index);
    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryFlag_x0(index))  rc = MAKERESULT(218, 8);

    if (R_SUCCEEDED(rc) && count >= 1) memset(recv_data, 0, sizeof(HidbusJoyPollingReceivedData)*count);
    if (R_SUCCEEDED(rc) && !_hidbusGetStatusManagerEntryPollingEnabled(index)) {
        return 0;
    }
    if (R_FAILED(rc)) return rc;

    if (count > 0xa) count = 0xa;

    HidbusJoyDisableSixAxisPollingDataAccessor *joydisable_accessor = entry->workbuf;
    HidbusJoyEnableSixAxisPollingDataAccessor *joyenable_accessor = entry->workbuf;
    HidbusJoyButtonOnlyPollingDataAccessor *joybutton_accessor = entry->workbuf;
    HidbusDataAccessorHeader *accessor_header;

    HidbusJoyPollingMode polling_mode = _hidbusGetStatusManagerEntryPollingMode(index);
    if (polling_mode == HidbusJoyPollingMode_JoyDisableSixAxisPollingData) {
        accessor_header = &joydisable_accessor->hdr;
    }
    else if (polling_mode == HidbusJoyPollingMode_JoyEnableSixAxisPollingData) {
        accessor_header = &joyenable_accessor->hdr;
    }
    else if (hosversionAtLeast(6,0,0) && polling_mode == HidbusJoyPollingMode_JoyButtonOnlyPollingData) {
        accessor_header = &joybutton_accessor->hdr;
    }
    else {
        return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    }

    s32 total_entries = (s32)atomic_load_explicit(&accessor_header->total_entries, memory_order_acquire);
    if (total_entries < 0) total_entries = 0;
    s32 newcount = count < total_entries ? count : total_entries;
    s32 latest_entry = (s32)atomic_load_explicit(&accessor_header->latest_entry, memory_order_acquire);

    union {
        HidbusJoyDisableSixAxisPollingDataAccessorEntryData joydisable[0xa];
        HidbusJoyEnableSixAxisPollingDataAccessorEntryData joyenable[0xa];
        HidbusJoyButtonOnlyPollingDataAccessorEntryData joybutton[0xa];
    } tmp_entries;

    if (polling_mode == HidbusJoyPollingMode_JoyDisableSixAxisPollingData) {
        memset(tmp_entries.joydisable, 0, sizeof(tmp_entries.joydisable));
    }
    else if (polling_mode == HidbusJoyPollingMode_JoyEnableSixAxisPollingData) {
        memset(tmp_entries.joyenable, 0, sizeof(tmp_entries.joyenable));
    }
    else if (hosversionAtLeast(6,0,0) && polling_mode == HidbusJoyPollingMode_JoyButtonOnlyPollingData) {
        memset(tmp_entries.joybutton, 0, sizeof(tmp_entries.joybutton));
    }

    for (s32 i=0; i<newcount; i++) {
        s32 entrypos = (((latest_entry + 0xc) - newcount) + i) % 0xb;

        u64 timestamp0=0, timestamp1=0;
        bool retry=false;
        if (polling_mode == HidbusJoyPollingMode_JoyDisableSixAxisPollingData) {
            timestamp0 = atomic_load_explicit(&joydisable_accessor->entries[entrypos].timestamp, memory_order_acquire);
            memcpy(&tmp_entries.joydisable[newcount-i-1], &joydisable_accessor->entries[entrypos].data, sizeof(HidbusJoyDisableSixAxisPollingDataAccessorEntryData));
            timestamp1 = atomic_load_explicit(&joydisable_accessor->entries[entrypos].timestamp, memory_order_acquire);

            if (timestamp0 != timestamp1 || (i>0 && joydisable_accessor->entries[entrypos].data.timestamp - tmp_entries.joydisable[newcount-i].timestamp != 1))
                retry=true;
        }
        else if (polling_mode == HidbusJoyPollingMode_JoyEnableSixAxisPollingData) {
            timestamp0 = atomic_load_explicit(&joyenable_accessor->entries[entrypos].timestamp, memory_order_acquire);
            memcpy(&tmp_entries.joyenable[newcount-i-1], &joyenable_accessor->entries[entrypos].data, sizeof(HidbusJoyEnableSixAxisPollingDataAccessorEntryData));
            timestamp1 = atomic_load_explicit(&joyenable_accessor->entries[entrypos].timestamp, memory_order_acquire);

            if (timestamp0 != timestamp1 || (i>0 && joyenable_accessor->entries[entrypos].data.timestamp - tmp_entries.joyenable[newcount-i].timestamp != 1))
                retry=true;
        }
        else if (hosversionAtLeast(6,0,0) && polling_mode == HidbusJoyPollingMode_JoyButtonOnlyPollingData) {
            timestamp0 = atomic_load_explicit(&joybutton_accessor->entries[entrypos].timestamp, memory_order_acquire);
            memcpy(&tmp_entries.joybutton[newcount-i-1], &joybutton_accessor->entries[entrypos].data, sizeof(HidbusJoyButtonOnlyPollingDataAccessorEntryData));
            timestamp1 = atomic_load_explicit(&joybutton_accessor->entries[entrypos].timestamp, memory_order_acquire);

            if (timestamp0 != timestamp1 || (i>0 && joybutton_accessor->entries[entrypos].data.timestamp - tmp_entries.joybutton[newcount-i].timestamp != 1))
                retry=true;
        }

        if (retry) {
            total_entries = (s32)atomic_load_explicit(&accessor_header->total_entries, memory_order_acquire);
            s32 tmpcount = newcount < total_entries ? total_entries : newcount;
            newcount = tmpcount < count ? tmpcount : count;
            latest_entry = (s32)atomic_load_explicit(&accessor_header->latest_entry, memory_order_acquire);

            i=-1;
        }
    }

    bool dataready=false;
    if (polling_mode == HidbusJoyPollingMode_JoyDisableSixAxisPollingData) {
        dataready = tmp_entries.joydisable[count-1].timestamp != 0;
    }
    else if (polling_mode == HidbusJoyPollingMode_JoyEnableSixAxisPollingData) {
        dataready = tmp_entries.joyenable[count-1].timestamp != 0;
    }
    else if (hosversionAtLeast(6,0,0) && polling_mode == HidbusJoyPollingMode_JoyButtonOnlyPollingData) {
        dataready = tmp_entries.joybutton[count-1].timestamp != 0;
    }
    if (!dataready) rc = MAKERESULT(218, 7);
    if (R_SUCCEEDED(rc)) rc = accessor_header->res;
    if (R_FAILED(rc)) return rc;

    for (s32 i=0; i<count; i++) {
        u8 size=0;

        if (polling_mode == HidbusJoyPollingMode_JoyDisableSixAxisPollingData) {
            size = tmp_entries.joydisable[i].size;
            if (size > sizeof(tmp_entries.joydisable[i].data)) return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
            memcpy(recv_data[i].data, tmp_entries.joydisable[i].data, size);
            recv_data[i].size = size;
            recv_data[i].timestamp = tmp_entries.joydisable[i].timestamp;
        }
        else if (polling_mode == HidbusJoyPollingMode_JoyEnableSixAxisPollingData) {
            size = tmp_entries.joyenable[i].size;
            if (size > sizeof(tmp_entries.joyenable[i].data)) return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
            memcpy(recv_data[i].data, tmp_entries.joyenable[i].data, size);
            recv_data[i].size = size;
            recv_data[i].timestamp = tmp_entries.joyenable[i].timestamp;
        }
        else if (hosversionAtLeast(6,0,0) && polling_mode == HidbusJoyPollingMode_JoyButtonOnlyPollingData) {
            size = tmp_entries.joybutton[i].size;
            if (size > sizeof(tmp_entries.joybutton[i].data)) return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
            memcpy(recv_data[i].data, tmp_entries.joybutton[i].data, size);
            recv_data[i].size = size;
            recv_data[i].timestamp = tmp_entries.joybutton[i].timestamp;
        }
    }

    return rc;
}

