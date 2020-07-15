#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/btdrv.h"

static Service g_btdrvSrv;

NX_GENERATE_SERVICE_GUARD(btdrv);

Result _btdrvInitialize(void) {
    return smGetService(&g_btdrvSrv, "btdrv");
}

void _btdrvCleanup(void) {
    serviceClose(&g_btdrvSrv);
}

Service* btdrvGetServiceSession(void) {
    return &g_btdrvSrv;
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

Result btdrvGetLeEventInfo(void* buffer, size_t size, u32 *type) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 95 : 96;

    return serviceDispatchOut(&g_btdrvSrv, cmd_id, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result btdrvRegisterBleHidEvent(Event* out_event) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u32 cmd_id = hosversionBefore(6,0,0) ? 96 : 97;

    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = serviceDispatch(&g_btdrvSrv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, true);
    return rc;
}

