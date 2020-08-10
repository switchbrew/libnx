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

static Result _btmCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, inval);
}

static Result _btmCmdInBleAdvertisePacketParameterNoOut(BtdrvBleAdvertisePacketParameter param, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, param);
}

static Result _btmGetBleScanResults(BtdrvBleScanResult *results, u8 count, u8 *total_out, u32 cmd_id) {
    return serviceDispatchOut(&g_btmSrv, cmd_id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { results, sizeof(BtdrvBleScanResult)*count } },
    );
}

static Result _btmBlePairDevice(BtdrvBleAdvertisePacketParameter param, u32 id, u32 cmd_id) {
    const struct {
        BtdrvBleAdvertisePacketParameter param;
        u32 id;
    } in = { param, id };

    return serviceDispatchIn(&g_btmSrv, cmd_id, in);
}

static Result _btmGetGattServiceData(u32 id, u16 unk1, void* buffer, size_t entrysize, u8 count, u8 *out, u32 cmd_id) {
    const struct {
        u16 unk1;
        u16 pad;
        u32 unk0;
    } in = { unk1, 0, id };

    return serviceDispatchInOut(&g_btmSrv, cmd_id, in, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, entrysize*count } },
    );
}

static Result _btmRegisterBleGattDataPath(const BtmBleDataPath *path, u32 cmd_id) {
    return serviceDispatchIn(&g_btmSrv, cmd_id, *path);
}

Result btmAcquireBleScanEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmCmdGetEventOutFlag(out_event, true, 23);
}

Result btmGetBleScanParameterGeneral(u16 unk, BtdrvBleAdvertisePacketParameter *out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btmSrv, 24, unk, *out);
}

Result btmGetBleScanParameterSmartDevice(u16 unk, BtdrvGattAttributeUuid *out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_btmSrv, 25, unk, *out);
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

Result btmBleDisconnect(u32 id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 25 : 37;

    return _btmCmdInU32NoOut(id, cmd_id);
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

Result btmBlePairDevice(BtdrvBleAdvertisePacketParameter param, u32 id) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmBlePairDevice(param, id, 41);
}

Result btmBleUnpairDeviceOnBoth(BtdrvBleAdvertisePacketParameter param, u32 id) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmBlePairDevice(param, id, 42);
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

Result btmGetGattServices(u32 id, BtmGattService *services, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 29 : 46;

    return serviceDispatchInOut(&g_btmSrv, cmd_id, id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { services, sizeof(BtmGattService)*count } },
    );
}

Result btmGetGattService(u32 id, const BtdrvGattAttributeUuid *uuid, BtmGattService *service, bool *flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 30 : 47;

    const struct {
        u32 id;
        BtdrvGattAttributeUuid uuid;
    } in = { id, *uuid };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_btmSrv, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { service, sizeof(*service) } },
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
    return rc;
}

Result btmGetGattIncludedServices(u32 id, u16 unk1, BtmGattService *services, u8 count, u8 *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 31 : 48;

    return _btmGetGattServiceData(id, unk1, services, sizeof(BtmGattService), count, out, cmd_id);
}

Result btmGetBelongingService(u32 id, u16 unk1, BtmGattService *service, bool *flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 32 : 49;

    const struct {
        u16 unk1;
        u16 pad;
        u32 id;
    } in = { unk1, 0, id };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_btmSrv, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { service, sizeof(*service) } },
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
    return rc;
}

Result btmGetGattCharacteristics(u32 id, u16 unk1, BtmGattCharacteristic *characteristics, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 33 : 50;

    return _btmGetGattServiceData(id, unk1, characteristics, sizeof(BtmGattCharacteristic), count, total_out, cmd_id);
}

Result btmGetGattDescriptors(u32 id, u16 unk1, BtmGattDescriptor *descriptors, u8 count, u8 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 34 : 51;

    return _btmGetGattServiceData(id, unk1, descriptors, sizeof(BtmGattDescriptor), count, total_out, cmd_id);
}

Result btmAcquireBleMtuConfigEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 35 : 52;

    return _btmCmdGetEventOutFlag(out_event, true, cmd_id);
}

Result btmConfigureBleMtu(u32 id, u16 mtu) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 36 : 53;

    const struct {
        u16 mtu;
        u16 pad;
        u32 id;
    } in = { mtu, 0, id };

    return serviceDispatchIn(&g_btmSrv, cmd_id, in);
}

Result btmGetBleMtu(u32 id, u16 *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(5,1,0) ? 37 : 54;

    return serviceDispatchInOut(&g_btmSrv, cmd_id, id, *out);
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

