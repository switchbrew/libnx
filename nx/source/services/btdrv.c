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

static Result _btdrvCmdTwoInBoolsNoOut(bool flag0, bool flag1, u32 cmd_id) {
    const struct {
        u8 flag;
        u8 flag2;
    } in = { flag0!=0, flag1!=0 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

static Result _btdrvCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, inval);
}

static Result _btdrvCmdInAddrNoOut(BtdrvAddress addr, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, addr);
}

static Result _btdrvCmdInAddrU8NoOut(BtdrvAddress addr, u8 inval, u32 cmd_id) {
    const struct {
        BtdrvAddress addr;
        u8 inval;
    } in = { addr, inval };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

static Result _btdrvCmdInAddrU32NoOut(BtdrvAddress addr, u32 inval, u32 cmd_id) {
    const struct {
        BtdrvAddress addr;
        u8 pad[2];
        u32 inval;
    } in = { addr, {0}, inval };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

static Result _btdrvCmdInAddrU32U32NoOut(BtdrvAddress addr, u32 inval0, u32 inval1, u32 cmd_id) {
    const struct {
        BtdrvAddress addr;
        u8 pad[2];
        u32 inval0;
        u32 inval1;
    } in = { addr, {0}, inval0, inval1 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

static Result _btdrvCmdInLeConnectionParameterNoOut(const BtdrvLeConnectionParams *param, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, *param);
}

static Result _btdrvCmdInUuidNoOut(const BtdrvGattAttributeUuid *uuid, u32 cmd_id) {
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, *uuid);
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

static Result _btdrvCmdInU32OutU32(u32 inval, u32 *out, u32 cmd_id) {
    return serviceDispatchInOut(&g_btdrvSrv, cmd_id, inval, *out);
}

static Result _btdrvCmdGetEvent(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _btdrvCmdGetHandle(&tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btdrvCmdInU32OutEvent(u32 inval, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;

    Result rc = serviceDispatchIn(&g_btdrvSrv, cmd_id, inval,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btdrvCmdInBufPtrFixed(const void* buffer, size_t size, u32 cmd_id) {
    return serviceDispatch(&g_btdrvSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
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

static Result _btdrvGattNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, u32 cmd_id) {
    const struct {
        u8 is_primary;
        u8 pad[3];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
    } in = { is_primary!=0, {0}, connection_handle, *serv_id, *char_id };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvInitializeBluetooth(Event* out_event) {
    return _btdrvCmdGetEvent(out_event, true, 1);
}

Result btdrvEnableBluetooth(void) {
    return _btdrvCmdNoIO(2);
}

Result btdrvDisableBluetooth(void) {
    return _btdrvCmdNoIO(3);
}

Result btdrvFinalizeBluetooth(void) {
    return _btdrvCmdNoIO(4);
}

Result btdrvLegacyGetAdapterProperties(BtdrvAdapterPropertyOld *properties) {
    if (hosversionAtLeast(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_btdrvSrv, 5,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { properties, sizeof(*properties) } },
    );
}

Result btdrvGetAdapterProperties(BtdrvAdapterPropertySet *properties) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_btdrvSrv, 5,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { properties, sizeof(*properties) } },
    );
}

Result btdrvLegacyGetAdapterProperty(BtdrvBluetoothPropertyType type, void* buffer, size_t size) {
    if (hosversionAtLeast(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=type;
    return serviceDispatchIn(&g_btdrvSrv, 6, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result btdrvGetAdapterProperty(BtdrvAdapterPropertyType type, BtdrvAdapterProperty *property) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=type;
    return serviceDispatchIn(&g_btdrvSrv, 6, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { property, sizeof(*property) } },
    );
}

Result btdrvLegacySetAdapterProperty(BtdrvBluetoothPropertyType type, const void* buffer, size_t size) {
    if (hosversionAtLeast(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=type;
    return serviceDispatchIn(&g_btdrvSrv, 7, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvSetAdapterProperty(BtdrvAdapterPropertyType type, const BtdrvAdapterProperty *property) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=type;
    return serviceDispatchIn(&g_btdrvSrv, 7, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { property, sizeof(*property) } },
    );
}

Result btdrvLegacyStartInquiry(void) {
    if (hosversionAtLeast(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(8);
}

Result btdrvStartInquiry(u32 services, s64 duration) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 services;
        s64 duration;
    } in = { services, duration };

    return serviceDispatchIn(&g_btdrvSrv, 8, in);
}

Result btdrvStopInquiry(void) {
    return _btdrvCmdNoIO(9);
}

Result btdrvCreateBond(BtdrvAddress addr, u32 type) {
    if (hosversionBefore(9,0,0)) {
        return serviceDispatchIn(&g_btdrvSrv, 10, addr,
            .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
            .buffers = { { &type, sizeof(type) } },
        );
    }

    return _btdrvCmdInAddrU32NoOut(addr, type, 10);
}

Result btdrvRemoveBond(BtdrvAddress addr) {
    return _btdrvCmdInAddrNoOut(addr, 11);
}

Result btdrvCancelBond(BtdrvAddress addr) {
    return _btdrvCmdInAddrNoOut(addr, 12);
}

Result btdrvLegacyRespondToPinRequest(BtdrvAddress addr, bool flag, const BtdrvBluetoothPinCode *pin_code, u8 length) {
    if (hosversionAtLeast(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        BtdrvAddress addr;
        u8 flag;
        u8 length;
        BtdrvBluetoothPinCode pin_code;
    } in = { addr, flag!=0, length, *pin_code };

    return serviceDispatchIn(&g_btdrvSrv, 13, in);
}

Result btdrvRespondToPinRequest(BtdrvAddress addr, const BtdrvPinCode *pin_code) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        BtdrvAddress addr;
        BtdrvPinCode pin_code;
    } in = { addr, *pin_code };

    return serviceDispatchIn(&g_btdrvSrv, 13, in);
}

Result btdrvRespondToSspRequest(BtdrvAddress addr, u32 variant, bool accept, u32 passkey) {
    if (hosversionBefore(12,0,0)) {
        const struct {
            BtdrvAddress addr;
            u8 variant;
            u8 accept;
            u32 passkey;
        } in = { addr, variant, accept!=0, passkey };

        return serviceDispatchIn(&g_btdrvSrv, 14, in);
    }
    else {
        const struct {
            BtdrvAddress addr;
            u8 accept;
            u32 variant;
            u32 passkey;
        } in = { addr, accept!=0, variant, passkey };

        return serviceDispatchIn(&g_btdrvSrv, 14, in);
    }
}

Result btdrvGetEventInfo(void* buffer, size_t size, BtdrvEventType *type) {
    return _btdrvCmdOutU32OutBuf(buffer, size, type, 15);
}

Result btdrvInitializeHid(Event* out_event) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;
    u16 tmp=0x1;

    if (R_SUCCEEDED(rc)) {
        rc = serviceDispatchIn(&g_btdrvSrv, 16, tmp,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = &tmp_handle,
        );
    }
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, true);
    return rc;
}

Result btdrvOpenHidConnection(BtdrvAddress addr) {
    return _btdrvCmdInAddrNoOut(addr, 17);
}

Result btdrvCloseHidConnection(BtdrvAddress addr) {
    return _btdrvCmdInAddrNoOut(addr, 18);
}

Result btdrvWriteHidData(BtdrvAddress addr, const BtdrvHidReport *buffer) {
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

Result btdrvSetHidReport(BtdrvAddress addr, BtdrvBluetoothHhReportType type, const BtdrvHidReport *buffer) {
    size_t size = hosversionBefore(9,0,0) ? sizeof(BtdrvHidData) : sizeof(BtdrvHidReport);

    const struct {
        BtdrvAddress addr;
        u8 pad[2];
        u32 type;
    } in = { addr, {0}, type };

    return serviceDispatchIn(&g_btdrvSrv, 21, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
}

Result btdrvGetHidReport(BtdrvAddress addr, u8 report_id, BtdrvBluetoothHhReportType type) {
    const struct {
        BtdrvAddress addr;
        u8 report_id;
        u8 pad;
        u32 type;
    } in = { addr, report_id, 0, type };

    return serviceDispatchIn(&g_btdrvSrv, 22, in);
}

Result btdrvTriggerConnection(BtdrvAddress addr, u16 timeout) {
    if (hosversionBefore(9,0,0)) return _btdrvCmdInAddrNoOut(addr, 23);

    const struct {
        BtdrvAddress addr;
        u16 timeout;
    } in = { addr, timeout };

    return serviceDispatchIn(&g_btdrvSrv, 23, in);
}

Result btdrvAddPairedDeviceInfo(const SetSysBluetoothDevicesSettings *settings) {
    return _btdrvCmdInBufPtrFixed(settings, sizeof(*settings), 24);
}

Result btdrvGetPairedDeviceInfo(BtdrvAddress addr, SetSysBluetoothDevicesSettings *settings) {
    return serviceDispatchIn(&g_btdrvSrv, 25, addr,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { settings, sizeof(*settings) } },
    );
}

Result btdrvFinalizeHid(void) {
    return _btdrvCmdNoIO(26);
}

Result btdrvGetHidEventInfo(void* buffer, size_t size, BtdrvHidEventType *type) {
    u32 tmp=0;
    Result rc = _btdrvCmdOutU32OutBuf(buffer, size, &tmp, 27);
    if (R_SUCCEEDED(rc) && type) *type = tmp;
    return rc;
}

Result btdrvSetTsi(BtdrvAddress addr, u8 tsi) {
    return _btdrvCmdInAddrU8NoOut(addr, tsi, 28);
}

Result btdrvEnableBurstMode(BtdrvAddress addr, bool flag) {
    return _btdrvCmdInAddrU8NoOut(addr, flag!=0, 29);
}

Result btdrvSetZeroRetransmission(BtdrvAddress addr, u8 *report_ids, u8 count) {
    return serviceDispatchIn(&g_btdrvSrv, 30, addr,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { report_ids, count } },
    );
}

Result btdrvEnableMcMode(bool flag) {
    return _btdrvCmdInBoolNoOut(flag, 31);
}

Result btdrvEnableLlrScan(void) {
    return _btdrvCmdNoIO(32);
}

Result btdrvDisableLlrScan(void) {
    return _btdrvCmdNoIO(33);
}

Result btdrvEnableRadio(bool flag) {
    return _btdrvCmdInBoolNoOut(flag, 34);
}

Result btdrvSetVisibility(bool inquiry_scan, bool page_scan) {
    return _btdrvCmdTwoInBoolsNoOut(inquiry_scan, page_scan, 35);
}

Result btdrvEnableTbfcScan(bool flag) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBoolNoOut(flag, 36);
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

    u32 cmd_id = hosversionBefore(4,0,0) ? 36 : 37;
    if (R_SUCCEEDED(rc)) rc = _btdrvCmdGetEvent(out_event, true, cmd_id); // RegisterHidReportEvent
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
            if (*type == BtdrvHidEventType_Data) {
                if (armTicksToNs(armGetSystemTick() - data_ptr->hdr.tick) >= 100000001) continue;
            }
            break;
        }
        if (data_ptr == NULL) {
            *type = BtdrvHidEventType_Data;
            return 0;
        }
        bool is_old = hosversionBefore(12,0,0);
        if ((is_old && *type == BtdrvHidEventTypeOld_GetReport) || (!is_old && *type == BtdrvHidEventType_GetReport)) {
            if (hosversionBefore(9,0,0)) memcpy(info->get_report.v1.rawdata, data_ptr->data.get_report.v1.rawdata, sizeof(info->get_report.v1.rawdata));
            else memcpy(info->get_report.v9.rawdata, data_ptr->data.get_report.v9.rawdata, sizeof(info->get_report.v9.rawdata));
        }
        else if ((is_old && *type == BtdrvHidEventTypeOld_SetReport) || (!is_old && *type == BtdrvHidEventType_SetReport)) memcpy(info->set_report.rawdata, data_ptr->data.set_report.rawdata, sizeof(info->set_report.rawdata));
        else if ((is_old && *type == BtdrvHidEventTypeOld_Data) || (!is_old && *type == BtdrvHidEventType_Data)) {
            u16 tmpsize = hosversionBefore(9,0,0) ? data_ptr->data.data_report.v7.report.size : data_ptr->data.data_report.v9.report.size;
            if (size < 0xE) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
            if (tmpsize > size-0xE) tmpsize = size-0xE;
            if (hosversionBefore(9,0,0)) info->data_report.v7.report.size = tmpsize;
            else {
                info->data_report.v9.res = 0;
                info->data_report.v9.report.size = tmpsize;
            }
            if (hosversionBefore(9,0,0)) memcpy(info->data_report.v7.report.data, data_ptr->data.data_report.v7.report.data, tmpsize);
            else memcpy(info->data_report.v9.report.data, data_ptr->data.data_report.v9.report.data, tmpsize);

            if (hosversionBefore(9,0,0)) memcpy(&info->data_report.v7.addr, &data_ptr->data.data_report.v7.addr, sizeof(BtdrvAddress));
            else memcpy(&info->data_report.v9.addr, &data_ptr->data.data_report.v9.addr, sizeof(BtdrvAddress));
        }
        else return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen); // sdknso would Abort here.
        btdrvCircularBufferFree(g_btdrvCircularBuffer);
        return 0;
    }

    u32 tmp_type=0;
    u32 cmd_id = hosversionBefore(4,0,0) ? 37 : 38;
    Result rc = _btdrvCmdOutU32OutBuf(buffer, size, &tmp_type, cmd_id);
    if (R_SUCCEEDED(rc) && type) *type = tmp_type;
    return rc;
}

void* btdrvGetHidReportEventInfoSharedmemAddr(void) {
    return g_btdrvCircularBuffer;
}

Result btdrvGetLatestPlr(BtdrvPlrList *out) {
    u32 cmd_id = hosversionBefore(4,0,0) ? 38 : 39;
    size_t size = hosversionBefore(9,0,0) ? sizeof(BtdrvPlrStatistics) : sizeof(BtdrvPlrList);

    return _btdrvCmdOutBufAliasFixed(out, size, cmd_id);
}

Result btdrvGetPendingConnections(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(4,0,0) ? 39 : 40;

    return _btdrvCmdNoIO(cmd_id);
}

Result btdrvGetChannelMap(BtdrvChannelMapList *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(4,0,0) ? 40 : 41;

    return _btdrvCmdOutBufAliasFixed(out, sizeof(*out), cmd_id);
}

Result btdrvEnableTxPowerBoostSetting(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(4,0,0) ? 41 : 42;

    return _btdrvCmdInBoolNoOut(flag, cmd_id);
}

Result btdrvIsTxPowerBoostSettingEnabled(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(4,0,0) ? 42 : 43;

    return _btdrvCmdNoInOutBool(out, cmd_id);
}

Result btdrvEnableAfhSetting(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(4,0,0) ? 43 : 44;

    return _btdrvCmdInBoolNoOut(flag, cmd_id);
}

Result btdrvIsAfhSettingEnabled(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(4,0,0) ? 44 : 45;

    return _btdrvCmdNoInOutBool(out, cmd_id);
}

Result btdrvInitializeBle(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdGetEvent(out_event, true, 46);
}

Result btdrvEnableBle(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(47);
}

Result btdrvDisableBle(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(48);
}

Result btdrvFinalizeBle(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(49);
}

Result btdrvSetBleVisibility(bool discoverable, bool connectable) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdTwoInBoolsNoOut(discoverable, connectable, 50);
}

Result btdrvSetLeConnectionParameter(const BtdrvLeConnectionParams *param) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInLeConnectionParameterNoOut(param, 51);
}

Result btdrvSetBleConnectionParameter(BtdrvAddress addr, const BtdrvBleConnectionParameter *param, bool flag) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        BtdrvAddress addr;
        u8 flag;
        u8 pad;
        BtdrvBleConnectionParameter param;
    } in = { addr, flag!=0, 0, *param };

    return serviceDispatchIn(&g_btdrvSrv, 51, in);
}

Result btdrvSetLeDefaultConnectionParameter(const BtdrvLeConnectionParams *param) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInLeConnectionParameterNoOut(param, 52);
}

Result btdrvSetBleDefaultConnectionParameter(const BtdrvBleConnectionParameter *param) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_btdrvSrv, 52, *param);
}

Result btdrvSetBleAdvertiseData(const BtdrvBleAdvertisePacketData *data) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBufPtrFixed(data, sizeof(*data), 53);
}

Result btdrvSetBleAdvertiseParameter(BtdrvAddress addr, u16 min_interval, u16 max_interval) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        BtdrvAddress addr;
        u16 min_interval;
        u16 max_interval;
    } in = { addr, min_interval, max_interval };

    return serviceDispatchIn(&g_btdrvSrv, 54, in);
}

Result btdrvStartBleScan(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(55);
}

Result btdrvStopBleScan(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(56);
}

Result btdrvAddBleScanFilterCondition(const BtdrvBleAdvertiseFilter *filter) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBufPtrFixed(filter, sizeof(*filter), 57);
}

Result btdrvDeleteBleScanFilterCondition(const BtdrvBleAdvertiseFilter *filter) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBufPtrFixed(filter, sizeof(*filter), 58);
}

Result btdrvDeleteBleScanFilter(u8 index) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInU8NoOut(index, 59);
}

Result btdrvClearBleScanFilters(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(60);
}

Result btdrvEnableBleScanFilter(bool flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInBoolNoOut(flag, 61);
}

Result btdrvRegisterGattClient(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInUuidNoOut(uuid, 62);
}

Result btdrvUnregisterGattClient(u8 client_if) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInU8NoOut(client_if, 63);
}

Result btdrvUnregisterAllGattClients(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoIO(64);
}

Result btdrvConnectGattServer(u8 client_if, BtdrvAddress addr, bool is_direct, u64 AppletResourceUserId) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 client_if;
        BtdrvAddress addr;
        u8 is_direct;
        u64 AppletResourceUserId;
    } in = { client_if, addr, is_direct!=0, AppletResourceUserId };

    return serviceDispatchIn(&g_btdrvSrv, 65, in);
}

Result btdrvCancelConnectGattServer(u8 client_if, BtdrvAddress addr, bool is_direct) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 client_if;
        BtdrvAddress addr;
        u8 is_direct;
    } in = { client_if, addr, is_direct!=0 };

    return serviceDispatchIn(&g_btdrvSrv, 66, in);
}

Result btdrvDisconnectGattServer(u32 conn_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 66 : 67;

    return _btdrvCmdInU32NoOut(conn_id, cmd_id);
}

Result btdrvGetGattAttribute(BtdrvAddress addr, u32 conn_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 67 : 68;

    if (hosversionBefore(9,0,0)) {
        const struct {
            BtdrvAddress addr;
            u32 conn_id;
        } in = { addr, conn_id };

        return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
    }
    return _btdrvCmdInU32NoOut(conn_id, cmd_id);
}

Result btdrvGetGattService(u32 conn_id, const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 68 : 69;

    const struct {
        u32 conn_id;
        BtdrvGattAttributeUuid uuid;
    } in = { conn_id, *uuid };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvConfigureAttMtu(u32 conn_id, u16 mtu) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 69 : 70;

    const struct {
        u16 mtu;
        u16 pad;
        u32 conn_id;
    } in = { mtu, 0, conn_id };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvRegisterGattServer(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 70 : 71;

    return _btdrvCmdInUuidNoOut(uuid, cmd_id);
}

Result btdrvUnregisterGattServer(u8 server_if) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 71 : 72;

    return _btdrvCmdInU8NoOut(server_if, cmd_id);
}

Result btdrvConnectGattClient(u8 server_if, BtdrvAddress addr, bool is_direct) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 72 : 73;

    const struct {
        u8 server_if;
        BtdrvAddress addr;
        u8 is_direct;
    } in = { server_if, addr, is_direct!=0 };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvDisconnectGattClient(u8 conn_id, BtdrvAddress addr) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 73 : 74;

    if (hosversionBefore(9,0,0)) {
        const struct {
            u8 conn_id;
            BtdrvAddress addr;
        } in = { conn_id, addr };

        return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
    }
    return serviceDispatchIn(&g_btdrvSrv, cmd_id, conn_id);
}

Result btdrvAddGattService(u8 server_if, const BtdrvGattAttributeUuid *uuid, u8 num_handle, bool is_primary) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 server_if;
        u8 num_handle;
        u8 is_primary;
        u8 pad;
        BtdrvGattAttributeUuid uuid;
    } in = { server_if, num_handle, is_primary!=0, 0, *uuid };

    return serviceDispatchIn(&g_btdrvSrv, 75, in);
}

Result btdrvEnableGattService(u8 server_if, const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 74 : 76;

    const struct {
        u8 server_if;
        u8 pad[3];
        BtdrvGattAttributeUuid uuid;
    } in = { server_if, {0}, *uuid };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvAddGattCharacteristic(u8 server_if, const BtdrvGattAttributeUuid *serv_uuid, const BtdrvGattAttributeUuid *char_uuid, u16 permissions, u8 property) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 server_if;
        u8 property;
        u16 permissions;
        BtdrvGattAttributeUuid serv_uuid;
        BtdrvGattAttributeUuid char_uuid;
    } in = { server_if, property, permissions, *serv_uuid, *char_uuid };

    return serviceDispatchIn(&g_btdrvSrv, 77, in);
}

Result btdrvAddGattDescriptor(u8 server_if, const BtdrvGattAttributeUuid *serv_uuid, const BtdrvGattAttributeUuid *desc_uuid, u16 permissions) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 76 : 78;

    const struct {
        u8 server_if;
        u8 pad;
        u16 permissions;
        BtdrvGattAttributeUuid serv_uuid;
        BtdrvGattAttributeUuid desc_uuid;
    } in = { server_if, 0, permissions, *serv_uuid, *desc_uuid };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvGetBleManagedEventInfo(void* buffer, size_t size, BtdrvBleEventType *type) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 78 : 79;

    return _btdrvCmdOutU32OutBuf(buffer, size, type, cmd_id);
}

Result btdrvGetGattFirstCharacteristic(u32 conn_id, const BtdrvGattId *serv_id, bool is_primary, const BtdrvGattAttributeUuid *filter_uuid, u8 *out_property, BtdrvGattId *out_char_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 79 : 80;

    const struct {
        u8 is_primary;
        u8 pad[3];
        u32 conn_id;
        BtdrvGattId serv_id;
        BtdrvGattAttributeUuid filter_uuid;
    } in = { is_primary!=0, {0}, conn_id, *serv_id, *filter_uuid };

    struct {
        u8 property;
        u8 pad[3];
        BtdrvGattId id;
    } out;

    Result rc = serviceDispatchInOut(&g_btdrvSrv, cmd_id, in, out);
    if (R_SUCCEEDED(rc)) {
        if (out_property) *out_property = out.property;
        if (out_char_id) *out_char_id = out.id;
    }
    return rc;
}

Result btdrvGetGattNextCharacteristic(u32 conn_id, const BtdrvGattId *serv_id, bool is_primary, const BtdrvGattId *char_id, const BtdrvGattAttributeUuid *filter_uuid, u8 *out_property, BtdrvGattId *out_char_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 80 : 81;

    const struct {
        u8 is_primary;
        u8 pad[3];
        u32 conn_id;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattAttributeUuid filter_uuid;
    } in = { is_primary!=0, {0}, conn_id, *serv_id, *char_id, *filter_uuid };

    struct {
        u8 property;
        u8 pad[3];
        BtdrvGattId id;
    } out;

    Result rc = serviceDispatchInOut(&g_btdrvSrv, cmd_id, in, out);
    if (R_SUCCEEDED(rc)) {
        if (out_property) *out_property = out.property;
        if (out_char_id) *out_char_id = out.id;
    }
    return rc;
}

Result btdrvGetGattFirstDescriptor(u32 conn_id, const BtdrvGattId *serv_id, bool is_primary, const BtdrvGattId *char_id, const BtdrvGattAttributeUuid *filter_uuid, BtdrvGattId *out_desc_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 81 : 82;

    const struct {
        u8 is_primary;
        u8 pad[3];
        u32 conn_id;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattAttributeUuid filter_uuid;
    } in = { is_primary!=0, {0}, conn_id, *serv_id, *char_id, *filter_uuid };

    return serviceDispatchInOut(&g_btdrvSrv, cmd_id, in, *out_desc_id);
}

Result btdrvGetGattNextDescriptor(u32 conn_id, const BtdrvGattId *serv_id, bool is_primary, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, const BtdrvGattAttributeUuid *filter_uuid, BtdrvGattId *out_desc_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 82 : 83;

    const struct {
        u8 is_primary;
        u8 pad[3];
        u32 conn_id;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattId desc_id;
        BtdrvGattAttributeUuid filter_uuid;
    } in = { is_primary!=0, {0}, conn_id, *serv_id, *char_id, *desc_id, *filter_uuid };

    return serviceDispatchInOut(&g_btdrvSrv, cmd_id, in, *out_desc_id);
}

Result btdrvRegisterGattManagedDataPath(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInUuidNoOut(uuid, 84);
}

Result btdrvUnregisterGattManagedDataPath(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInUuidNoOut(uuid, 85);
}

Result btdrvRegisterGattHidDataPath(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInUuidNoOut(uuid, 86);
}

Result btdrvUnregisterGattHidDataPath(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInUuidNoOut(uuid, 87);
}

Result btdrvRegisterGattDataPath(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInUuidNoOut(uuid, 88);
}

Result btdrvUnregisterGattDataPath(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 83 : 89;

    return _btdrvCmdInUuidNoOut(uuid, cmd_id);
}

Result btdrvReadGattCharacteristic(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, u8 auth_req) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 89 : 90;

    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
    } in = { is_primary!=0, auth_req, {0}, connection_handle, *serv_id, *char_id};

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvReadGattDescriptor(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, u8 auth_req) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 90 : 91;

    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattId desc_id;
    } in = { is_primary!=0, auth_req, {0}, connection_handle, *serv_id, *char_id, *desc_id };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in);
}

Result btdrvWriteGattCharacteristic(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const void* buffer, size_t size, u8 auth_req, bool with_response) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 91 : 92;

    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 with_response;
        u8 pad;
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
    } in = { is_primary!=0, auth_req, with_response!=0, 0, connection_handle, *serv_id, *char_id };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvWriteGattDescriptor(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, const void* buffer, size_t size, u8 auth_req) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 92 : 93;

    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattId desc_id;
    } in = { is_primary!=0, auth_req, {0}, connection_handle, *serv_id, *char_id, *desc_id };

    return serviceDispatchIn(&g_btdrvSrv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvRegisterGattNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvGattNotification(connection_handle, is_primary, serv_id, char_id, 94);
}

Result btdrvUnregisterGattNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 93 : 95;

    return _btdrvGattNotification(connection_handle, is_primary, serv_id, char_id, cmd_id);
}

Result btdrvGetLeHidEventInfo(void* buffer, size_t size, BtdrvBleEventType *type) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 95 : 96;

    return _btdrvCmdOutU32OutBuf(buffer, size, type, cmd_id);
}

Result btdrvRegisterBleHidEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 96 : 97;

    return _btdrvCmdGetEvent(out_event, true, cmd_id);
}

Result btdrvSetBleScanParameter(u16 scan_interval, u16 scan_window) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u16 scan_interval;
        u16 scan_window;
    } in = { scan_interval, scan_window };

    return serviceDispatchIn(&g_btdrvSrv, 98, in);
}

Result btdrvMoveToSecondaryPiconet(BtdrvAddress addr) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrNoOut(addr, 99);
}

Result btdrvIsBluetoothEnabled(bool *out) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdNoInOutBool(out, 100);
}

Result btdrvAcquireAudioEvent(Event* out_event, bool autoclear) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdGetEvent(out_event, autoclear, 128);
}

Result btdrvGetAudioEventInfo(void* buffer, size_t size, BtdrvAudioEventType *type) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _btdrvCmdOutU32OutBuf(buffer, size, &tmp, 129);
    if (R_SUCCEEDED(rc) && type) *type = tmp;
    return rc;
}

Result btdrvOpenAudioConnection(BtdrvAddress addr) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrNoOut(addr, 130);
}

Result btdrvCloseAudioConnection(BtdrvAddress addr) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrNoOut(addr, 131);
}

Result btdrvOpenAudioOut(BtdrvAddress addr, u32 *audio_handle) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btdrvSrv, 132, addr, *audio_handle);
}

Result btdrvCloseAudioOut(u32 audio_handle) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInU32NoOut(audio_handle, 133);
}

Result btdrvAcquireAudioOutStateChangedEvent(u32 audio_handle, Event* out_event, bool autoclear) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInU32OutEvent(audio_handle, out_event, autoclear, 134);
}

Result btdrvStartAudioOut(u32 audio_handle, const BtdrvPcmParameter *pcm_param, s64 in_latency, s64 *out_latency, u64 *out1) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 audio_handle;
        BtdrvPcmParameter pcm_param;
        s64 latency;
    } in = { audio_handle, *pcm_param, in_latency };

    struct {
        s64 latency;
        u64 out1;
    } out;

    Result rc = serviceDispatchInOut(&g_btdrvSrv, 135, in, out);
    if (R_SUCCEEDED(rc)) {
        if (out_latency) *out_latency = out.latency;
        if (out1) *out1 = out.out1;
    }
    return rc;
}

Result btdrvStopAudioOut(u32 audio_handle) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInU32NoOut(audio_handle, 136);
}

Result btdrvGetAudioOutState(u32 audio_handle, BtdrvAudioOutState *out) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _btdrvCmdInU32OutU32(audio_handle, &tmp, 137);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result btdrvGetAudioOutFeedingCodec(u32 audio_handle, BtdrvAudioCodec *out) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _btdrvCmdInU32OutU32(audio_handle, &tmp, 138);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result btdrvGetAudioOutFeedingParameter(u32 audio_handle, BtdrvPcmParameter *out) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btdrvSrv, 139, audio_handle, *out);
}

Result btdrvAcquireAudioOutBufferAvailableEvent(u32 audio_handle, Event* out_event, bool autoclear) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInU32OutEvent(audio_handle, out_event, autoclear, 140);
}

Result btdrvSendAudioData(u32 audio_handle, const void* buffer, size_t size, u64 *transferred_size) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btdrvSrv, 141, audio_handle, *transferred_size,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result btdrvAcquireAudioControlInputStateChangedEvent(Event* out_event, bool autoclear) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdGetEvent(out_event, autoclear, 142);
}

Result btdrvGetAudioControlInputState(BtdrvAudioControlButtonState *states, s32 count, s32 *total_out) {
    if (hosversionBefore(12,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdOutU32OutBuf(states, count*sizeof(BtdrvAudioControlButtonState), (u32*)total_out, 143);
}

Result btdrvAcquireAudioConnectionStateChangedEvent(Event* out_event, bool autoclear) {
    if (!hosversionBetween(12, 14))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdGetEvent(out_event, autoclear, 144);
}

Result btdrvGetConnectedAudioDevice(BtdrvAddress *addrs, s32 count, s32 *total_out) {
    if (!hosversionBetween(12, 14))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdOutU32OutBuf(addrs, count*sizeof(BtdrvAddress), (u32*)total_out, 145);
}

Result btdrvCloseAudioControlInput(BtdrvAddress addr) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrNoOut(addr, 146);
}

Result btdrvRegisterAudioControlNotification(BtdrvAddress addr, u32 event_type) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrU32NoOut(addr, event_type, 147);
}

Result btdrvSendAudioControlPassthroughCommand(BtdrvAddress addr, u32 op_id, u32 state_type) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrU32U32NoOut(addr, op_id, state_type, 148);
}

Result btdrvSendAudioControlSetAbsoluteVolumeCommand(BtdrvAddress addr, s32 val) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btdrvCmdInAddrU32NoOut(addr, (u32)val, 149);
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
    if (read_offset >= sizeof(c->data)) return false;

    u64 tmpsize = read_offset + 0x18;
    if (tmpsize < sizeof(c->data)) tmpsize += *((u64*)&data_ptr[0x10]);
    if (tmpsize >= sizeof(c->data)) tmpsize = 0;
    atomic_store_explicit(&c->read_offset, tmpsize, memory_order_release);

    return true;
}

