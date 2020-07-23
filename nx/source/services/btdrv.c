#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <stdatomic.h>
#include "service_guard.h"
#include "arm/counter.h"
#include "kernel/shmem.h"
#include "runtime/hosversion.h"
#include "services/btdrv.h"

static Service g_btdrvSrv;

static bool g_btdrvSharedmemInitialized;
static SharedMemory g_btdrvSharedmem;
static BtdrvCircularBuffer *g_btdrvCircularBuffer;

static Result _btdrvCmdNoIO(u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(btdrv);

Result _btdrvInitialize(void) {
    Result rc=0;

    rc = smGetService(&g_btdrvSrv, "btdrv");
    if (R_SUCCEEDED(rc)) rc = _btdrvCmdNoIO(0); // InitializeBluetoothDriver
    return rc;
}

void _btdrvCleanup(void) {
    g_btdrvCircularBuffer = NULL;
    shmemClose(&g_btdrvSharedmem);
    serviceClose(&g_btdrvSrv);
}

Service* btdrvGetServiceSession(void) {
    return &g_btdrvSrv;
}

static Result _btdrvCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_btdrvSrv, cmd_id);
}

static Result _btdrvCmdGetHandle(Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(&g_btdrvSrv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _btdrvCmdInU8NoOut(u8 inval, u64 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, inval);
}

static Result _btdrvCmdInBoolNoOut(bool inval, u32 cmd_id) {
    return _btdrvCmdInU8NoOut(inval!=0, cmd_id);
}

static Result _btdrvCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, inval);
}

static Result _btdrvCmdInAddrNoOut(BtdrvAddress addr, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, addr);
}

static Result _btdrvCmdNoInOutU8(u8 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_btdrvSrv, cmd_id, *out);
}

static Result _btdrvCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _btdrvCmdNoInOutU8(&tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _btdrvCmdGetEvent(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _btdrvCmdGetHandle(&tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btdrvCmdOutBufAliasFixed(void* buffer, size_t size, u32 cmd_id) {
    return serviceDispatch(&g_btdrvSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
}

static Result _btdrvCmdOutU32OutBuf(void* buffer, size_t size, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_btdrvSrv, cmd_id, *out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result btdrvGetAdapterProperties(BtdrvAdapterProperty *property) {
    return serviceDispatch(&g_btdrvSrv, 5,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { property, sizeof(*property) } },
    );
}

Result btdrvGetAdapterProperty(BtdrvBluetoothPropertyType type, void* buffer, size_t size) {
    u32 tmp=type;
    return serviceDispatchIn(&g_btdrvSrv, 6, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result btdrvSetAdapterProperty(BtdrvBluetoothPropertyType type, const void* buffer, size_t size) {
    u32 tmp=type;
    return serviceDispatchIn(&g_btdrvSrv, 7, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvWriteHidData(BtdrvAddress addr, BtdrvHidReport *buffer) {
    size_t size = hosversionBefore(9,0,0) ? sizeof(BtdrvHidData) : sizeof(BtdrvHidReport);
    return serviceDispatchIn(&g_btdrvSrv, 19, addr,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
}

Result btdrvWriteHidData2(BtdrvAddress addr, const void* buffer, size_t size) {
    return serviceDispatchIn(&g_btdrvSrv, 20, addr,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvSetHidReport(BtdrvAddress addr, u32 type, BtdrvHidReport *buffer) {
    size_t size = hosversionBefore(9,0,0) ? sizeof(BtdrvHidData) : sizeof(BtdrvHidReport);

    const struct {
        BtdrvAddress addr;
        u32 type;
    } in = { addr, type };

    return serviceDispatchIn(&g_btdrvSrv, 21, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
}

Result btdrvGetHidReport(BtdrvAddress addr, u8 unk, u32 type) {
    const struct {
        BtdrvAddress addr;
        u8 unk;
        u8 pad[3];
        u32 type;
    } in = { addr, unk, {0}, type };

    return serviceDispatchIn(&g_btdrvSrv, 22, in);
}

Result btdrvRegisterHidReportEvent(Event* out_event) {
    Result rc=0;
    Handle tmphandle=0;

    if (hosversionAtLeast(7,0,0)) {
        if (!g_btdrvSharedmemInitialized) {
            rc = _btdrvCmdGetHandle(&tmphandle, 38); // GetHidReportEventInfo

            if (R_SUCCEEDED(rc)) {
                shmemLoadRemote(&g_btdrvSharedmem, tmphandle, 0x3000, Perm_Rw);

                rc = shmemMap(&g_btdrvSharedmem);
            }

            if (R_SUCCEEDED(rc)) {
                g_btdrvSharedmemInitialized = true;
                g_btdrvCircularBuffer = shmemGetAddr(&g_btdrvSharedmem);
            }
            else shmemClose(&g_btdrvSharedmem);
        }
        else {
            while (btdrvCircularBufferRead(g_btdrvCircularBuffer)) btdrvCircularBufferFree(g_btdrvCircularBuffer);
        }
    }

    if (R_SUCCEEDED(rc)) rc = _btdrvCmdGetEvent(out_event, true, 37); // RegisterHidReportEvent
    return rc;
}

Result btdrvGetHidReportEventInfo(void* buffer, size_t size, BtdrvHidEventType *type) {
    if (hosversionAtLeast(7,0,0)) {
        BtdrvHidReportEventInfo *info = (BtdrvHidReportEventInfo*)buffer;
        BtdrvHidReportEventInfoBufferData *data_ptr = NULL;
        memset(buffer, 0, size);
        if (g_btdrvCircularBuffer==NULL) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
        for (; (data_ptr = btdrvCircularBufferRead(g_btdrvCircularBuffer)); btdrvCircularBufferFree(g_btdrvCircularBuffer)) {
            *type = data_ptr->hdr.type;
            if (*type == BtdrvHidEventType_Unknown4) {
                if (armTicksToNs(armGetSystemTick() - data_ptr->hdr.tick) >= 100000001) continue;
            }
            break;
        }
        if (data_ptr == NULL) {
            *type = BtdrvHidEventType_Unknown4;
            return 0;
        }
        if (*type == BtdrvHidEventType_Unknown9) {
            if (hosversionBefore(9,0,0)) memcpy(info->type9.hid_data.rawdata, data_ptr->data.type9.hid_data.rawdata, sizeof(info->type9.hid_data.rawdata));
            else memcpy(info->type9.hid_report.rawdata, data_ptr->data.type9.hid_report.rawdata, sizeof(info->type9.hid_report.rawdata));
        }
        else if (*type == BtdrvHidEventType_Unknown8) memcpy(info->type8.data, data_ptr->data.type8.data, sizeof(info->type8.data));
        else if (*type == BtdrvHidEventType_Unknown4) {
            u16 tmpsize = hosversionBefore(9,0,0) ? data_ptr->data.type4.v1.size : data_ptr->data.type4.v9.size;
            if (size < 0xE) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
            if (tmpsize > size-0xE) tmpsize = size-0xE;
            info->type4.unk_x0 = 0;
            info->type4.size = tmpsize;
            if (hosversionBefore(9,0,0)) memcpy(info->type4.data, data_ptr->data.type4.v1.data, tmpsize);
            else memcpy(info->type4.data, data_ptr->data.type4.v9.data, tmpsize);

            if (hosversionBefore(9,0,0)) memcpy(&info->type4.addr, &data_ptr->data.type4.v1.addr, sizeof(BtdrvAddress));
            else memcpy(&info->type4.addr, &data_ptr->data.type4.v9.addr, sizeof(BtdrvAddress));
        }
        else return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen); // sdknso would Abort here.
        btdrvCircularBufferFree(g_btdrvCircularBuffer);
        return 0;
    }

    u32 tmp_type=0;
    Result rc = _btdrvCmdOutU32OutBuf(buffer, size, &tmp_type, 38);
    if (R_SUCCEEDED(rc) && type) *type = tmp_type;
    return rc;
}

void* btdrvGetHidReportEventInfoSharedmemAddr(void) {
    return g_btdrvCircularBuffer;
}

Result btdrvGetLatestPlr(BtdrvPlrList *out) {
    size_t size = hosversionBefore(9,0,0) ? sizeof(BtdrvPlrStatistics) : sizeof(BtdrvPlrList);

    return _btdrvCmdOutBufAliasFixed(out, size, 39);
}

Result btdrvEnableTxPowerBoostSetting(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBoolNoOut(flag, 42);
}

Result btdrvIsTxPowerBoostSettingEnabled(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoInOutBool(out, 43);
}

Result btdrvEnableAfhSetting(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBoolNoOut(flag, 44);
}

Result btdrvIsAfhSettingEnabled(bool *out) {
    return _btdrvCmdNoInOutBool(out, 45);
}

Result btdrvGetChannelMap(BtdrvChannelMapList *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdOutBufAliasFixed(out, sizeof(*out), 41);
}

Result btdrvReadGattCharacteristic(bool flag, u8 unk, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 89 : 90;

    const struct {
        u8 flag;
        u8 unk;
        u8 pad[2];
        u32 unk2;
        BtdrvGattId id0;
        BtdrvGattId id1;
    } in = { flag!=0, unk, {0}, unk2, *id0, *id1};

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvReadGattDescriptor(bool flag, u8 unk, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 90 : 91;

    const struct {
        u8 flag;
        u8 unk;
        u8 pad[2];
        u32 unk2;
        BtdrvGattId id0;
        BtdrvGattId id1;
        BtdrvGattId id2;
    } in = { flag!=0, unk, {0}, unk2, *id0, *id1, *id2 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvWriteGattCharacteristic(bool flag, u8 unk, bool flag2, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1, const void* buffer, size_t size) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 91 : 92;

    const struct {
        u8 flag;
        u8 unk;
        u8 flag2;
        u8 pad;
        u32 unk2;
        BtdrvGattId id0;
        BtdrvGattId id1;
    } in = { flag!=0, unk, flag2!=0, 0, unk2, *id0, *id1 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvWriteGattDescriptor(bool flag, u8 unk, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, const void* buffer, size_t size) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 92 : 93;

    const struct {
        u8 flag;
        u8 unk;
        u8 pad[2];
        u32 unk2;
        BtdrvGattId id0;
        BtdrvGattId id1;
        BtdrvGattId id2;
    } in = { flag!=0, unk, {0}, unk2, *id0, *id1, *id2 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _btdrvGattNotification(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1, u32 cmd_id) {
    const struct {
        u8 flag;
        u8 pad[3];
        u32 unk;
        BtdrvGattId id0;
        BtdrvGattId id1;
    } in = { flag!=0, {0}, unk, *id0, *id1 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvRegisterGattNotification(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvGattNotification(flag, unk, id0, id1, 94);
}

Result btdrvUnregisterGattNotification(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 93 : 95;

    return _btdrvGattNotification(flag, unk, id0, id1, cmd_id);
}

Result btdrvGetLeHidEventInfo(void* buffer, size_t size, u32 *type) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 95 : 96;

    return _btdrvCmdOutU32OutBuf(buffer, size, type, cmd_id);
}

Result btdrvRegisterBleHidEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 96 : 97;

    return _btdrvCmdGetEvent(out_event, true, cmd_id);
}

Result btdrvMoveToSecondaryPiconet(BtdrvAddress addr) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrNoOut(addr, 99);
}

Result btdrvIsManufacturingMode(bool *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoInOutBool(out, 256);
}

Result btdrvEmulateBluetoothCrash(BtdrvFatalReason reason) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 inval = reason;
    return _btdrvCmdInU32NoOut(inval, 257);
}

Result btdrvGetBleChannelMap(BtdrvChannelMapList *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdOutBufAliasFixed(out, sizeof(*out), 258);
}

// CircularBuffer

void* btdrvCircularBufferRead(BtdrvCircularBuffer *c) {
    if (c==NULL || !c->initialized) return NULL;

    do {
        s32 read_offset = atomic_load_explicit(&c->read_offset, memory_order_acquire);
        s32 write_offset = atomic_load_explicit(&c->write_offset, memory_order_acquire);
        if (read_offset == write_offset) return NULL;

        u8 *data_ptr = &c->data[read_offset];
        if (read_offset >= sizeof(c->data)) return NULL;

        if (*data_ptr != 0xFF) return data_ptr;
        else {
            if (!c->initialized) return NULL;
            read_offset = atomic_load_explicit(&c->read_offset, memory_order_acquire);
            write_offset = atomic_load_explicit(&c->write_offset, memory_order_acquire);
            if (read_offset == write_offset) continue;

            data_ptr = &c->data[read_offset];
            u64 tmpsize = read_offset + 0x18;
            if (tmpsize < sizeof(c->data)) tmpsize += *((u64*)&data_ptr[0x10]);
            if (tmpsize >= sizeof(c->data)) tmpsize = 0;
            atomic_store_explicit(&c->read_offset, tmpsize, memory_order_release);
        }
    } while (c->initialized);

    return NULL;
}

bool btdrvCircularBufferFree(BtdrvCircularBuffer *c) {
    if (c==NULL || !c->initialized) return false;

    s32 read_offset = atomic_load_explicit(&c->read_offset, memory_order_acquire);
    s32 write_offset = atomic_load_explicit(&c->write_offset, memory_order_acquire);
    if (read_offset == write_offset) return false;

    u8 *data_ptr = &c->data[read_offset];
    if (read_offset >= sizeof(c->data)) false;

    u64 tmpsize = read_offset + 0x18;
    if (tmpsize < sizeof(c->data)) tmpsize += *((u64*)&data_ptr[0x10]);
    if (tmpsize >= sizeof(c->data)) tmpsize = 0;
    atomic_store_explicit(&c->read_offset, tmpsize, memory_order_release);

    return true;
}

