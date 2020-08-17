#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/bt.h"
#include "services/applet.h"

static Service g_btSrv;

NX_GENERATE_SERVICE_GUARD(bt);

Result _btInitialize(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_btSrv, "bt");
}

void _btCleanup(void) {
    serviceClose(&g_btSrv);
}

Service* btGetServiceSession(void) {
    return &g_btSrv;
}

Result btLeClientReadCharacteristic(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, u8 unk) {
    const struct {
        u8 primary_service;
        u8 unk;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId id0;
        BtdrvGattId id1;
        u64 AppletResourceUserId;
    } in = { primary_service!=0, unk, {0}, connection_handle, *id0, *id1, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 0, in,
        .in_send_pid = true,
    );
}

Result btLeClientReadDescriptor(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, u8 unk) {
    const struct {
        u8 primary_service;
        u8 unk;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId id0;
        BtdrvGattId id1;
        BtdrvGattId id2;
        u64 AppletResourceUserId;
    } in = { primary_service!=0, unk, {0}, connection_handle, *id0, *id1, *id2, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 1, in,
        .in_send_pid = true,
    );
}

Result btLeClientWriteCharacteristic(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const void* buffer, size_t size, u8 unk, bool flag) {
    const struct {
        u8 primary_service;
        u8 unk;
        u8 flag;
        u8 pad;
        u32 connection_handle;
        BtdrvGattId id0;
        BtdrvGattId id1;
        u64 AppletResourceUserId;
    } in = { primary_service!=0, unk, flag!=0, 0, connection_handle, *id0, *id1, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 2, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btLeClientWriteDescriptor(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, const void* buffer, size_t size, u8 unk) {
    const struct {
        u8 primary_service;
        u8 unk;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId id0;
        BtdrvGattId id1;
        BtdrvGattId id2;
        u64 AppletResourceUserId;
    } in = { primary_service!=0, unk, {0}, connection_handle, *id0, *id1, *id2, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 3, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

static Result _btLeClientNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, u32 cmd_id) {
    const struct {
        u8 primary_service;
        u8 pad[3];
        u32 connection_handle;
        BtdrvGattId id0;
        BtdrvGattId id1;
        u64 AppletResourceUserId;
    } in = { primary_service!=0, {0}, connection_handle, *id0, *id1, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

Result btLeClientRegisterNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1) {
    return _btLeClientNotification(connection_handle, primary_service, id0, id1, 4);
}

Result btLeClientDeregisterNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1) {
    return _btLeClientNotification(connection_handle, primary_service, id0, id1, 5);
}

Result btSetLeResponse(u8 unk, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1, const void* buffer, size_t size) {
    const struct {
        u8 unk;
        u8 pad[3];
        BtdrvGattAttributeUuid uuid0;
        BtdrvGattAttributeUuid uuid1;
        u8 pad2[4];
        u64 AppletResourceUserId;
    } in = { unk, {0}, *uuid0, *uuid1, {0}, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 6, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btLeSendIndication(u8 unk, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1, const void* buffer, size_t size, bool flag) {
    const struct {
        u8 unk;
        u8 flag;
        u8 pad[2];
        BtdrvGattAttributeUuid uuid0;
        BtdrvGattAttributeUuid uuid1;
        u8 pad2[4];
        u64 AppletResourceUserId;
    } in = { unk, flag!=0, {0}, *uuid0, *uuid1, {0}, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 7, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btGetLeEventInfo(void* buffer, size_t size, u32 *type) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();
    return serviceDispatchInOut(&g_btSrv, 8, AppletResourceUserId, *type,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btRegisterBleEvent(Event* out_event) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    rc = serviceDispatchIn(&g_btSrv, 9, AppletResourceUserId,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, true);
    return rc;
}

