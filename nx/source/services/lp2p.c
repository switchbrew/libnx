#include <string.h>
#include <unistd.h>
#include "service_guard.h"
#include "sf/sessionmgr.h"
#include "runtime/hosversion.h"
#include "services/lp2p.h"

static Lp2pServiceType g_lp2pServiceType;
static Service g_lp2pSrv;
static Service g_lp2pINetworkService;
static Service g_lp2pINetworkServiceMonitor;
static SessionMgr g_lp2pSessionMgr;

static Result _lp2pCreateNetworkService(Service* srv, Service* srv_out, u32 inval);
static Result _lp2pCreateNetworkServiceMonitor(Service* srv, Service* srv_out);

NX_INLINE bool _lp2pObjectIsChild(Service* s)
{
    return s->session == g_lp2pSrv.session;
}

NX_INLINE Result _lp2pObjectDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
) {
    int slot = -1;
    if (_lp2pObjectIsChild(s)) {
        slot = sessionmgrAttachClient(&g_lp2pSessionMgr);
        if (slot < 0) __builtin_unreachable();
        disp.target_session = sessionmgrGetClientSession(&g_lp2pSessionMgr, slot);
        serviceAssumeDomain(s);
    }

    Result rc = serviceDispatchImpl(s, request_id, in_data, in_data_size, out_data, out_data_size, disp);

    if (slot >= 0) {
        sessionmgrDetachClient(&g_lp2pSessionMgr, slot);
    }

    return rc;
}

#define _lp2pObjectDispatch(_s,_rid,...) \
    _lp2pObjectDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _lp2pObjectDispatchIn(_s,_rid,_in,...) \
    _lp2pObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _lp2pObjectDispatchOut(_s,_rid,_out,...) \
    _lp2pObjectDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _lp2pObjectDispatchInOut(_s,_rid,_in,_out,...) \
    _lp2pObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

NX_GENERATE_SERVICE_GUARD_PARAMS(lp2p, (Lp2pServiceType service_type), (service_type));

Result _lp2pInitialize(Lp2pServiceType service_type) {
    const char *serv_name = NULL;
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    Service srv={0};

    g_lp2pServiceType = service_type;
    switch (g_lp2pServiceType) {
        case Lp2pServiceType_App:
            serv_name = "lp2p:app";
            rc = 0;
            break;
        case Lp2pServiceType_System:
            serv_name = "lp2p:sys";
            rc = 0;
            break;
    }

    if (R_FAILED(rc)) return rc;

    if (hosversionBefore(9,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = smGetService(&g_lp2pSrv, serv_name);

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_lp2pSrv);
    }
    if (R_SUCCEEDED(rc))
        rc = sessionmgrCreate(&g_lp2pSessionMgr, g_lp2pSrv.session, 0x4);

    if (R_SUCCEEDED(rc))
        rc = _lp2pCreateNetworkService(&g_lp2pSrv, &g_lp2pINetworkService, 0x1);

    if (R_SUCCEEDED(rc)) {
        rc = smGetService(&srv, serv_name);
        if (R_SUCCEEDED(rc)) rc = _lp2pCreateNetworkServiceMonitor(&srv, &g_lp2pINetworkServiceMonitor);
        serviceClose(&srv);
    }

    return rc;
}

void _lp2pCleanup(void) {
    serviceClose(&g_lp2pINetworkServiceMonitor);

    // Close extra sessions
    sessionmgrClose(&g_lp2pSessionMgr);

    // We can't assume g_lp2pSrv is a domain here because serviceConvertToDomain might have failed
    serviceClose(&g_lp2pSrv);
}

Service* lp2pGetServiceSession_INetworkService(void) {
    return &g_lp2pINetworkService;
}

Service* lp2pGetServiceSession_INetworkServiceMonitor(void) {
    return &g_lp2pINetworkServiceMonitor;
}

static Result _lp2pCreateNetworkService(Service* srv, Service* srv_out, u32 inval) {
    const struct {
        u32 inval;
        u32 pad;
        u64 pid_placeholder;
    } in = { inval, 0, 0 };

    return _lp2pObjectDispatchIn(srv, 0, in,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _lp2pCreateNetworkServiceMonitor(Service* srv, Service* srv_out) {
    u64 pid_placeholder=0;
    return serviceDispatchIn(srv, 8, pid_placeholder,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _lp2pObjCmdNoIO(u32 cmd_id) {
    return _lp2pObjectDispatch(&g_lp2pINetworkService, cmd_id);
}

static Result _lp2pCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

void lp2pCreateGroupInfo(Lp2pGroupInfo *info) {
    memset(info, 0, sizeof(*info));

    info->flags_count = 1;
    info->flags[0] = 1;
    info->supported_platform = 1;
    info->unk_x82 = 0x2;
    info->network_mode = 1;
    info->performance_requirement = 3;
    info->priority = 90;
}

void lp2pCreateGroupInfoScan(Lp2pGroupInfo *info) {
    memset(info, 0, sizeof(*info));

    info->supported_platform = 1;
    info->priority = 90;
}

void lp2pGroupInfoSetServiceName(Lp2pGroupInfo *info, const char *name) {
    strncpy(info->service_name, name, sizeof(info->service_name)-1);
    info->service_name[sizeof(info->service_name)-1] = 0;
}

void lp2pGroupInfoSetFlags(Lp2pGroupInfo *info, s8 *flags, size_t count) {
    info->flags_count = count;
    if (count < 1) return;
    for (s8 i=0; i<count && i<sizeof(info->flags); i++)
        info->flags[i] = flags[i];
}

void lp2pGroupInfoSetPresharedKey(Lp2pGroupInfo *info, const void* key, size_t size) {
    info->preshared_key_binary_size = sizeof(info->preshared_key);
    if (size > info->preshared_key_binary_size) size = info->preshared_key_binary_size;
    memcpy(info->preshared_key, key, size);
}

// INetworkService

Result lp2pScan(const Lp2pGroupInfo *info, Lp2pScanResult *results, s32 count, s32 *total_out) {
    return _lp2pObjectDispatchOut(&g_lp2pINetworkService, 512, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { info, sizeof(*info) },
            { results, count*sizeof(Lp2pScanResult) },
        },
    );
}

Result lp2pCreateGroup(const Lp2pGroupInfo *info) {
    return _lp2pObjectDispatch(&g_lp2pINetworkService, 768,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { info, sizeof(*info) } },
    );
}

Result lp2pDestroyGroup(void) {
    return _lp2pObjCmdNoIO(776);
}

Result lp2pSetAdvertiseData(const void* buffer, size_t size) {
    return _lp2pObjectDispatch(&g_lp2pINetworkService, 784,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result lp2pSendToOtherGroup(const void* buffer, size_t size, Lp2pMacAddress addr, Lp2pGroupId group_id, s16 frequency, s16 channel, u32 flags) {
    const struct {
        Lp2pMacAddress addr;
        Lp2pGroupId group_id;
        s16 frequency;
        s16 channel;
        u32 flags;
    } in = { addr, group_id, frequency, channel, flags };

    return _lp2pObjectDispatchIn(&g_lp2pINetworkService, 1536, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result lp2pRecvFromOtherGroup(void* buffer, size_t size, u32 flags, Lp2pMacAddress *addr, u16 *unk0, s32 *unk1, u64 *out_size, s32 *unk2) {
    struct {
        Lp2pMacAddress addr;
        u16 unk0;
        s16 unk1;
        u16 pad;
        u32 out_size;
        s32 unk2;
    } out;

    Result rc = _lp2pObjectDispatchInOut(&g_lp2pINetworkService, 1544, flags, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (addr) *addr = out.addr;
        if (unk0) *unk0 = out.unk0;
        if (unk1) *unk1 = out.unk1;
        if (out_size) *out_size = out.out_size;
        if (unk2) *unk2 = out.unk2;
    }
    return rc;
}

Result lp2pAddAcceptableGroupId(Lp2pGroupId group_id) {
    return _lp2pObjectDispatchIn(&g_lp2pINetworkService, 1552, group_id);
}

Result lp2pRemoveAcceptableGroupId(void) {
    return _lp2pObjCmdNoIO(1560);
}

// INetworkServiceMonitor

Result lp2pAttachNetworkInterfaceStateChangeEvent(Event* out_event) {
    return _lp2pCmdGetEvent(&g_lp2pINetworkServiceMonitor, out_event, false, 256);
}

Result lp2pGetNetworkInterfaceLastError(void) {
    return serviceDispatch(&g_lp2pINetworkServiceMonitor, 264);
}

Result lp2pGetRole(u8 *out) {
    return serviceDispatchOut(&g_lp2pINetworkServiceMonitor, 272, *out);
}

static Result _lp2pGetAdvertiseData(void* buffer, size_t size, u16 *transfer_size, u16 *original_size, u32 cmd_id) {
    struct {
        u16 transfer_size;
        u16 original_size;
    } out;

    Result rc = serviceDispatchOut(&g_lp2pINetworkServiceMonitor, cmd_id, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (transfer_size) *transfer_size = out.transfer_size;
        if (original_size) *original_size = out.original_size;
    }
    return rc;
}

Result lp2pGetAdvertiseData(void* buffer, size_t size, u16 *transfer_size, u16 *original_size) {
    return _lp2pGetAdvertiseData(buffer, size, transfer_size, original_size, 280);
}

Result lp2pGetAdvertiseData2(void* buffer, size_t size, u16 *transfer_size, u16 *original_size) {
    return _lp2pGetAdvertiseData(buffer, size, transfer_size, original_size, 281);
}

Result lp2pGetGroupInfo(Lp2pGroupInfo *out) {
    return serviceDispatch(&g_lp2pINetworkServiceMonitor, 288,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result lp2pJoin(Lp2pGroupInfo *out, const Lp2pGroupInfo *info) {
    return serviceDispatch(&g_lp2pINetworkServiceMonitor, 296,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out | SfBufferAttr_FixedSize,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In | SfBufferAttr_FixedSize,
        },
        .buffers = {
            { out, sizeof(*out) },
            { info, sizeof(*info) },
        },
    );
}

Result lp2pGetGroupOwner(Lp2pNodeInfo *out) {
    return serviceDispatchOut(&g_lp2pINetworkServiceMonitor, 304, *out);
}

Result lp2pGetIpConfig(Lp2pIpConfig *out) {
    return serviceDispatch(&g_lp2pINetworkServiceMonitor, 312,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result lp2pLeave(u32 *out) {
    return serviceDispatchOut(&g_lp2pINetworkServiceMonitor, 320, *out);
}

Result lp2pAttachJoinEvent(Event* out_event) {
    return _lp2pCmdGetEvent(&g_lp2pINetworkServiceMonitor, out_event, false, 328);
}

Result lp2pGetMembers(Lp2pNodeInfo *members, s32 count, s32 *total_out) {
    return serviceDispatchOut(&g_lp2pINetworkServiceMonitor, 336, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { members, count*sizeof(Lp2pNodeInfo) } },
    );
}

