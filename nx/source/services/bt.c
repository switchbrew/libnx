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

Result btLeClientReadCharacteristic(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, u8 auth_req) {
    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        u64 AppletResourceUserId;
    } in = { is_primary!=0, auth_req, {0}, connection_handle, *serv_id, *char_id, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 0, in,
        .in_send_pid = true,
    );
}

Result btLeClientReadDescriptor(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, u8 auth_req) {
    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattId desc_id;
        u64 AppletResourceUserId;
    } in = { is_primary!=0, auth_req, {0}, connection_handle, *serv_id, *char_id, *desc_id, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 1, in,
        .in_send_pid = true,
    );
}

Result btLeClientWriteCharacteristic(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const void* buffer, size_t size, u8 auth_req, bool with_response) {
    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 with_response;
        u8 pad;
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        u64 AppletResourceUserId;
    } in = { is_primary!=0, auth_req, with_response!=0, 0, connection_handle, *serv_id, *char_id, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 2, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btLeClientWriteDescriptor(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, const void* buffer, size_t size, u8 auth_req) {
    const struct {
        u8 is_primary;
        u8 auth_req;
        u8 pad[2];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        BtdrvGattId desc_id;
        u64 AppletResourceUserId;
    } in = { is_primary!=0, auth_req, {0}, connection_handle, *serv_id, *char_id, *desc_id, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 3, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

static Result _btLeClientNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, u32 cmd_id) {
    const struct {
        u8 is_primary;
        u8 pad[3];
        u32 connection_handle;
        BtdrvGattId serv_id;
        BtdrvGattId char_id;
        u64 AppletResourceUserId;
    } in = { is_primary!=0, {0}, connection_handle, *serv_id, *char_id, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

Result btLeClientRegisterNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id) {
    return _btLeClientNotification(connection_handle, is_primary, serv_id, char_id, 4);
}

Result btLeClientDeregisterNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id) {
    return _btLeClientNotification(connection_handle, is_primary, serv_id, char_id, 5);
}

Result btSetLeResponse(u8 server_if, const BtdrvGattAttributeUuid *serv_uuid, const BtdrvGattAttributeUuid *char_uuid, const void* buffer, size_t size) {
    const struct {
        u8 server_if;
        u8 pad[3];
        BtdrvGattAttributeUuid serv_uuid;
        BtdrvGattAttributeUuid char_uuid;
        u8 pad2[4];
        u64 AppletResourceUserId;
    } in = { server_if, {0}, *serv_uuid, *char_uuid, {0}, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 6, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btLeSendIndication(u8 server_if, const BtdrvGattAttributeUuid *serv_uuid, const BtdrvGattAttributeUuid *char_uuid, const void* buffer, size_t size, bool noconfirm) {
    const struct {
        u8 server_if;
        u8 noconfirm;
        u8 pad[2];
        BtdrvGattAttributeUuid serv_uuid;
        BtdrvGattAttributeUuid char_uuid;
        u8 pad2[4];
        u64 AppletResourceUserId;
    } in = { server_if, noconfirm!=0, {0}, *serv_uuid, *char_uuid, {0}, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_btSrv, 7, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

Result btGetLeEventInfo(void* buffer, size_t size, BtdrvBleEventType *type) {
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

