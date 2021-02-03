#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/btm.h"

static Service g_btmSrv;

NX_GENERATE_SERVICE_GUARD(btm);

Result _btmInitialize(void) {
    return smGetService(&g_btmSrv, "btm");
}

void _btmCleanup(void) {
    serviceClose(&g_btmSrv);
}

Service* btmGetServiceSession(void) {
    return &g_btmSrv;
}

static Result _btmCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_btmSrv, cmd_id);
}

static Result _btmCmdGetEventOutFlag(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    if (hosversionBefore(3,0,0)) {
        u8 out=0;
        rc = serviceDispatchOut(&g_btmSrv, cmd_id, out,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = &tmp_handle,
        );
        if (R_SUCCEEDED(rc) && !out) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    }
    else {
        rc = serviceDispatch(&g_btmSrv, cmd_id,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = &tmp_handle,
        );
    }
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btmCmdInU8NoOut(u8 inval, u64 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, inval);
}

static Result _btmCmdInBoolNoOut(bool inval, u32 cmd_id) {
    return _btmCmdInU8NoOut(inval!=0, cmd_id);
}

static Result _btmCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, inval);
}

static Result _btmCmdInU64NoOut(u64 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, inval);
}

static Result _btmCmdNoInOutU8(u8 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_btmSrv, cmd_id, *out);
}

static Result _btmCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _btmCmdNoInOutU8(&tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _btmCmdNoInOutU32(u32 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_btmSrv, cmd_id, *out);
}

static Result _btmCmdInAddrNoOut(BtdrvAddress addr, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, addr);
}

static Result _btmCmdInAddrBoolNoOut(BtdrvAddress addr, bool flag, u32 cmd_id) {
    const struct {
        BtdrvAddress addr;
        u8 flag;
    } in = { addr, flag!=0 };

    return serviceDispatchIn(&g_btmSrv, cmd_id, in);
}

static Result _btmCmdInBleAdvertisePacketParameterNoOut(BtdrvBleAdvertisePacketParameter param, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, param);
}

static Result _btmCmdInBufPtrFixed(const void* buffer, size_t size, u32 cmd_id) {
    return serviceDispatch(&g_btmSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
}

static Result _btmCmdOutBufPtrFixed(void* buffer, size_t size, u32 cmd_id) {
    return serviceDispatch(&g_btmSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { buffer, size } },
    );
}

static Result _btmGetBleScanResults(BtdrvBleScanResult *results, u8 count, u8 *total_out, u32 cmd_id) {
    return serviceDispatchOut(&g_btmSrv, cmd_id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { results, sizeof(BtdrvBleScanResult)*count } },
    );
}

static Result _btmBlePairDevice(u32 connection_handle, BtdrvBleAdvertisePacketParameter param, u32 cmd_id) {
    const struct {
        BtdrvBleAdvertisePacketParameter param;
        u32 connection_handle;
    } in = { param, connection_handle };

    return serviceDispatchIn(&g_btmSrv, cmd_id, in);
}

static Result _btmGetGattServiceData(u32 connection_handle, u16 handle, void* buffer, size_t entrysize, u8 count, u8 *out, u32 cmd_id) {
    const struct {
        u16 handle;
        u16 pad;
        u32 connection_handle;
    } in = { handle, 0, connection_handle };

    return serviceDispatchInOut(&g_btmSrv, cmd_id, in, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, entrysize*count } },
    );
}

static Result _btmRegisterBleGattDataPath(const BtmBleDataPath *path, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, *path);
}

Result btmGetState(BtmState *out) {
    u32 tmp=0;
    Result rc = _btmCmdNoInOutU32(&tmp, 0);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result btmGetHostDeviceProperty(BtmHostDeviceProperty *out) {
    return serviceDispatchOut(&g_btmSrv, 1, *out);
}

Result btmAcquireDeviceConditionEvent(Event* out_event) {
    return _btmCmdGetEventOutFlag(out_event, true, 2);
}

Result btmGetDeviceCondition(BtmDeviceCondition *out) {
    return _btmCmdOutBufPtrFixed(out, sizeof(*out), 3);
}

Result btmSetBurstMode(BtdrvAddress addr, bool flag) {
    return _btmCmdInAddrBoolNoOut(addr, flag, 4);
}

Result btmSetSlotMode(const BtmDeviceSlotModeList *list) {
    return _btmCmdInBufPtrFixed(list, sizeof(*list), 5);
}

Result btmSetBluetoothMode(BtmBluetoothMode mode) {
    if (hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdInU32NoOut(mode, 6);
}

Result btmSetWlanMode(BtmWlanMode mode) {
    return _btmCmdInU32NoOut(mode, 7);
}

Result btmAcquireDeviceInfoEvent(Event* out_event) {
    return _btmCmdGetEventOutFlag(out_event, true, 8);
}

Result btmGetDeviceInfo(BtmDeviceInfoList *out) {
    return _btmCmdOutBufPtrFixed(out, sizeof(*out), 9);
}

Result btmAddDeviceInfo(const BtmDeviceInfo *info) {
    return serviceDispatchIn(&g_btmSrv, 10, *info);
}

Result btmRemoveDeviceInfo(BtdrvAddress addr) {
    return _btmCmdInAddrNoOut(addr, 11);
}

Result btmIncreaseDeviceInfoOrder(BtdrvAddress addr) {
    return _btmCmdInAddrNoOut(addr, 12);
}

Result btmLlrNotify(BtdrvAddress addr, s32 unk) {
    if (hosversionBefore(9,0,0))
        return _btmCmdInAddrNoOut(addr, 13);

    const struct {
        BtdrvAddress addr;
        u8 pad[2];
        s32 unk;
    } in = { addr, {0}, unk };

    return serviceDispatchIn(&g_btmSrv, 13, in);
}

Result btmEnableRadio(void) {
    return _btmCmdNoIO(14);
}

Result btmDisableRadio(void) {
    return _btmCmdNoIO(15);
}

Result btmHidDisconnect(BtdrvAddress addr) {
    return _btmCmdInAddrNoOut(addr, 16);
}

Result btmHidSetRetransmissionMode(BtdrvAddress addr, const BtmZeroRetransmissionList *list) {
    return serviceDispatchIn(&g_btmSrv, 17, addr,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { list, sizeof(*list) } },
    );
}

Result btmAcquireAwakeReqEvent(Event* out_event) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdGetEventOutFlag(out_event, true, 18);
}

Result btmAcquireLlrStateEvent(Event* out_event) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdGetEventOutFlag(out_event, true, 19);
}

Result btmIsLlrStarted(bool *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdNoInOutBool(out, 20);
}

Result btmEnableSlotSaving(bool flag) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdInBoolNoOut(flag, 21);
}

Result btmProtectDeviceInfo(BtdrvAddress addr, bool flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdInAddrBoolNoOut(addr, flag, 22);
}

Result btmAcquireBleScanEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdGetEventOutFlag(out_event, true, 23);
}

Result btmGetBleScanParameterGeneral(u16 parameter_id, BtdrvBleAdvertisePacketParameter *out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btmSrv, 24, parameter_id, *out);
}

Result btmGetBleScanParameterSmartDevice(u16 parameter_id, BtdrvGattAttributeUuid *out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btmSrv, 25, parameter_id, *out);
}

Result btmStartBleScanForGeneral(BtdrvBleAdvertisePacketParameter param) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdInBleAdvertisePacketParameterNoOut(param, 26);
}

Result btmStopBleScanForGeneral(void) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdNoIO(27);
}

Result btmGetBleScanResultsForGeneral(BtdrvBleScanResult *results, u8 count, u8 *total_out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmGetBleScanResults(results, count, total_out, 28);
}

Result btmStartBleScanForPaired(BtdrvBleAdvertisePacketParameter param) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdInBleAdvertisePacketParameterNoOut(param, 29);
}

Result btmStopBleScanForPaired(void) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdNoIO(30);
}

Result btmStartBleScanForSmartDevice(const BtdrvGattAttributeUuid *uuid) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_btmSrv, 31, *uuid);
}

Result btmStopBleScanForSmartDevice(void) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdNoIO(32);
}

Result btmGetBleScanResultsForSmartDevice(BtdrvBleScanResult *results, u8 count, u8 *total_out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmGetBleScanResults(results, count, total_out, 33);
}

Result btmAcquireBleConnectionEvent(Event* out_event) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdGetEventOutFlag(out_event, true, 34);
}

Result btmBleConnect(BtdrvAddress addr) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 24 : 35;

    return serviceDispatchIn(&g_btmSrv, cmd_id, addr);
}

Result btmBleOverrideConnection(u32 id) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdInU32NoOut(id, 36);
}

Result btmBleDisconnect(u32 connection_handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 25 : 37;

    return _btmCmdInU32NoOut(connection_handle, cmd_id);
}

Result btmBleGetConnectionState(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 26 : 38;

    return serviceDispatchOut(&g_btmSrv, cmd_id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { info, sizeof(BtdrvBleConnectionInfo)*count } },
    );
}

Result btmBleGetGattClientConditionList(BtmGattClientConditionList *list) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 27 : 39;

    return serviceDispatch(&g_btmSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { list, sizeof(*list) } },
    );
}

Result btmAcquireBlePairingEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 28 : 40;

    return _btmCmdGetEventOutFlag(out_event, true, cmd_id);
}

Result btmBlePairDevice(u32 connection_handle, BtdrvBleAdvertisePacketParameter param) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmBlePairDevice(connection_handle, param, 41);
}

Result btmBleUnpairDeviceOnBoth(u32 connection_handle, BtdrvBleAdvertisePacketParameter param) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmBlePairDevice(connection_handle, param, 42);
}

Result btmBleUnPairDevice(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        BtdrvAddress addr;
        BtdrvBleAdvertisePacketParameter param;
    } in = { addr, param };

    return serviceDispatchIn(&g_btmSrv, 43, in);
}

Result btmBleGetPairedAddresses(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btmSrv, 44, param, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { addrs, sizeof(BtdrvAddress)*count } },
    );
}

Result btmAcquireBleServiceDiscoveryEvent(Event* out_event) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdGetEventOutFlag(out_event, true, 45);
}

Result btmGetGattServices(u32 connection_handle, BtmGattService *services, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 29 : 46;

    return serviceDispatchInOut(&g_btmSrv, cmd_id, connection_handle, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { services, sizeof(BtmGattService)*count } },
    );
}

Result btmGetGattService(u32 connection_handle, const BtdrvGattAttributeUuid *uuid, BtmGattService *service, bool *flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 30 : 47;

    const struct {
        u32 connection_handle;
        BtdrvGattAttributeUuid uuid;
    } in = { connection_handle, *uuid };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_btmSrv, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { service, sizeof(*service) } },
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
    return rc;
}

Result btmGetGattIncludedServices(u32 connection_handle, u16 service_handle, BtmGattService *services, u8 count, u8 *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 31 : 48;

    return _btmGetGattServiceData(connection_handle, service_handle, services, sizeof(BtmGattService), count, out, cmd_id);
}

Result btmGetBelongingService(u32 connection_handle, u16 attribute_handle, BtmGattService *service, bool *flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 32 : 49;

    const struct {
        u16 attribute_handle;
        u16 pad;
        u32 connection_handle;
    } in = { attribute_handle, 0, connection_handle };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_btmSrv, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { service, sizeof(*service) } },
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
    return rc;
}

Result btmGetGattCharacteristics(u32 connection_handle, u16 service_handle, BtmGattCharacteristic *characteristics, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 33 : 50;

    return _btmGetGattServiceData(connection_handle, service_handle, characteristics, sizeof(BtmGattCharacteristic), count, total_out, cmd_id);
}

Result btmGetGattDescriptors(u32 connection_handle, u16 char_handle, BtmGattDescriptor *descriptors, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 34 : 51;

    return _btmGetGattServiceData(connection_handle, char_handle, descriptors, sizeof(BtmGattDescriptor), count, total_out, cmd_id);
}

Result btmAcquireBleMtuConfigEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 35 : 52;

    return _btmCmdGetEventOutFlag(out_event, true, cmd_id);
}

Result btmConfigureBleMtu(u32 connection_handle, u16 mtu) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 36 : 53;

    const struct {
        u16 mtu;
        u16 pad;
        u32 connection_handle;
    } in = { mtu, 0, connection_handle };

    return serviceDispatchIn(&g_btmSrv, cmd_id, in);
}

Result btmGetBleMtu(u32 connection_handle, u16 *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 37 : 54;

    return serviceDispatchInOut(&g_btmSrv, cmd_id, connection_handle, *out);
}

Result btmRegisterBleGattDataPath(const BtmBleDataPath *path) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 38 : 55;

    return _btmRegisterBleGattDataPath(path, cmd_id);
}

Result btmUnregisterBleGattDataPath(const BtmBleDataPath *path) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 39 : 56;

    return _btmRegisterBleGattDataPath(path, cmd_id);
}

Result btmRegisterAppletResourceUserId(u64 AppletResourceUserId, u32 unk) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 40 : 57;

    const struct {
        u32 unk;
        u64 AppletResourceUserId;
    } in = { unk, AppletResourceUserId };

    return serviceDispatchIn(&g_btmSrv, cmd_id, in);
}

Result btmUnregisterAppletResourceUserId(u64 AppletResourceUserId) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 41 : 58;

    return _btmCmdInU64NoOut(AppletResourceUserId, cmd_id);
}

Result btmSetAppletResourceUserId(u64 AppletResourceUserId) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 42 : 59;

    return _btmCmdInU64NoOut(AppletResourceUserId, cmd_id);
}

