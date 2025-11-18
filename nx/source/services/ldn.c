#include <string.h>
#include "service_guard.h"
#include "sf/sessionmgr.h"
#include "services/ldn.h"
#include "runtime/hosversion.h"

static LdnServiceType g_ldnServiceType;

__attribute__((weak)) s32 __nx_ldn_priority = LDN_PRIORITY_SYSTEM;

static Service g_ldnSrvCreator;
static Service g_ldnSrv;
static Service g_ldnmSrv;

static Service g_ldnIClientProcessMonitor;

static SessionMgr g_ldnSessionMgr;

static Result _ldnGetSession(Service* srv, Service* srv_out, u32 cmd_id);

static Result _ldnCmdNoIO(Service* srv, u32 cmd_id);

static Result _ldnCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id);

static Result _ldnCmdInitialize(Service* srv, u32 cmd_id);
static Result _ldnCmdInitializeWithVersion(s32 version);
static Result _ldnCmdInitializeWithPriority(s32 version, s32 priority);

static Result _ldnGetNetworkInfo(Service* srv, LdnNetworkInfo *out);

static Result _ldnGetIpv4Address(Service* srv, LdnIpv4Address *addr, LdnSubnetMask *mask);

static Result _ldnGetSecurityParameter(Service* srv, LdnSecurityParameter *out);
static Result _ldnGetNetworkConfig(Service* srv, LdnNetworkConfig *out);

NX_INLINE bool _ldnObjectIsChild(Service* s)
{
    return s->session == g_ldnSrvCreator.session;
}

static void _ldnObjectClose(Service* s) {
    if (!_ldnObjectIsChild(s)) {
        serviceClose(s);
    }
    else {
        int slot = sessionmgrAttachClient(&g_ldnSessionMgr);
        uint32_t object_id = serviceGetObjectId(s);
        serviceAssumeDomain(s);
        cmifMakeCloseRequest(armGetTls(), object_id);
        svcSendSyncRequest(sessionmgrGetClientSession(&g_ldnSessionMgr, slot));
        sessionmgrDetachClient(&g_ldnSessionMgr, slot);
    }
}

NX_INLINE Result _ldnObjectDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
) {
    int slot = -1;
    if (_ldnObjectIsChild(s)) {
        slot = sessionmgrAttachClient(&g_ldnSessionMgr);
        if (slot < 0) __builtin_unreachable();
        disp.target_session = sessionmgrGetClientSession(&g_ldnSessionMgr, slot);
        serviceAssumeDomain(s);
    }

    Result rc = serviceDispatchImpl(s, request_id, in_data, in_data_size, out_data, out_data_size, disp);

    if (slot >= 0) {
        sessionmgrDetachClient(&g_ldnSessionMgr, slot);
    }

    return rc;
}

#define _ldnObjectDispatch(_s,_rid,...) \
    _ldnObjectDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _ldnObjectDispatchIn(_s,_rid,_in,...) \
    _ldnObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _ldnObjectDispatchOut(_s,_rid,_out,...) \
    _ldnObjectDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _ldnObjectDispatchInOut(_s,_rid,_in,_out,...) \
    _ldnObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

// ldn:m

NX_GENERATE_SERVICE_GUARD(ldnm);

Result _ldnmInitialize(void) {
    Service srv_creator={0};
    Result rc = smGetService(&srv_creator, "ldn:m");

    if (R_SUCCEEDED(rc)) rc = _ldnGetSession(&srv_creator, &g_ldnmSrv, 0); // CreateMonitorService
    serviceClose(&srv_creator);

    if (R_SUCCEEDED(rc)) rc = _ldnCmdNoIO(&g_ldnmSrv, 100); // InitializeMonitor

    return rc;
}

void _ldnmCleanup(void) {
    if (serviceIsActive(&g_ldnmSrv)) _ldnCmdNoIO(&g_ldnmSrv, 101); // FinalizeMonitor
    serviceClose(&g_ldnmSrv);
}

Service* ldnmGetServiceSession_MonitorService(void) {
    return &g_ldnmSrv;
}

Result ldnmGetState(LdnState *out) {
    u32 tmp=0;
    Result rc = _ldnCmdNoInOutU32(&g_ldnmSrv, &tmp, 0);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result ldnmGetNetworkInfo(LdnNetworkInfo *out) {
    return _ldnGetNetworkInfo(&g_ldnmSrv, out);
}

Result ldnmGetIpv4Address(LdnIpv4Address *addr, LdnSubnetMask *mask) {
    return _ldnGetIpv4Address(&g_ldnmSrv, addr, mask);
}

Result ldnmGetSecurityParameter(LdnSecurityParameter *out) {
    return _ldnGetSecurityParameter(&g_ldnmSrv, out);
}

Result ldnmGetNetworkConfig(LdnNetworkConfig *out) {
    return _ldnGetNetworkConfig(&g_ldnmSrv, out);
}

// ldn

NX_GENERATE_SERVICE_GUARD_PARAMS(ldn, (LdnServiceType service_type), (service_type));

Result _ldnInitialize(LdnServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    s32 version=0;
    g_ldnServiceType = service_type;
    switch (g_ldnServiceType) {
        case LdnServiceType_User:
            rc = smGetService(&g_ldnSrvCreator, "ldn:u");
            break;
        case LdnServiceType_System:
            rc = smGetService(&g_ldnSrvCreator, "ldn:s");
            break;
    }

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_ldnSrvCreator);

    if (R_SUCCEEDED(rc))
        rc = sessionmgrCreate(&g_ldnSessionMgr, g_ldnSrvCreator.session, 0x3);

    if (R_SUCCEEDED(rc)) rc = _ldnGetSession(&g_ldnSrvCreator, &g_ldnSrv, 0); // CreateSystemLocalCommunicationService/CreateUserLocalCommunicationService

    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(7,0,0)) {
            version = 0x1;

            if (hosversionAtLeast(18,0,0))
                version = 0x2;
            if (hosversionAtLeast(19,0,0))
                version = 0x3;
            if (hosversionAtLeast(20,0,0))
                version = 0x4;

            if (g_ldnServiceType == LdnServiceType_System && hosversionAtLeast(19,0,0))
                rc = _ldnCmdInitializeWithPriority(version, __nx_ldn_priority);
            else
                rc = _ldnCmdInitializeWithVersion(version);
        }
        else
            rc = _ldnCmdInitialize(&g_ldnSrv, 400);
    }

    if (R_SUCCEEDED(rc) && hosversionAtLeast(20,0,0))
        rc = ldnSetProtocol(LdnProtocol_NX);

    if (R_SUCCEEDED(rc) && hosversionAtLeast(18,0,0)) {
        rc = _ldnGetSession(&g_ldnSrvCreator, &g_ldnIClientProcessMonitor, 1); // CreateClientProcessMonitor
        if (R_SUCCEEDED(rc))
            rc = _ldnCmdInitialize(&g_ldnIClientProcessMonitor, 0); // RegisterClient
    }

    _ldnObjectClose(&g_ldnSrvCreator); // Close just the creator object.

    return rc;
}

void _ldnCleanup(void) {
    if (serviceIsActive(&g_ldnSrv)) _ldnCmdNoIO(&g_ldnSrv, 401); // Finalize(System)

    _ldnObjectClose(&g_ldnSrv);
    _ldnObjectClose(&g_ldnIClientProcessMonitor);

    // Close extra sessions
    sessionmgrClose(&g_ldnSessionMgr);

    // We can't assume g_ldnSrvCreator is a domain here because serviceConvertToDomain might have failed
    serviceClose(&g_ldnSrvCreator);
}

Service* ldnGetServiceSession_LocalCommunicationService(void) {
    return &g_ldnSrv;
}

Service* ldnGetServiceSession_IClientProcessMonitor(void) {
    return &g_ldnIClientProcessMonitor;
}

static Result _ldnGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return _ldnObjectDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _ldnCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = _ldnObjectDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _ldnCmdNoIO(Service* srv, u32 cmd_id) {
    return _ldnObjectDispatch(srv, cmd_id);
}

static Result _ldnCmdInU32NoOut(Service* srv, u32 in, u32 cmd_id) {
    return _ldnObjectDispatchIn(srv, cmd_id, in);
}

static Result _ldnCmdInU16NoOut(Service* srv, u16 in, u32 cmd_id) {
    return _ldnObjectDispatchIn(srv, cmd_id, in);
}

static Result _ldnCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return _ldnObjectDispatchOut(srv, cmd_id, *out);
}

static Result _ldnCmdInitialize(Service* srv, u32 cmd_id) {
    u64 reserved=0;
    return _ldnObjectDispatchIn(srv, cmd_id, reserved,
        .in_send_pid = true,
    );
}

static Result _ldnCmdInitializeWithVersion(s32 version) {
    const struct {
        s32 version;
        u32 pad;
        u64 reserved;
    } in = { version, 0, 0};

    u32 cmd_id = g_ldnServiceType == LdnServiceType_User ? 402 : 403;
    return _ldnObjectDispatchIn(&g_ldnSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _ldnCmdInitializeWithPriority(s32 version, s32 priority) {
    const struct {
        s32 version;
        s32 priority;
        u64 reserved;
    } in = { version, priority, 0};

    return _ldnObjectDispatchIn(&g_ldnSrv, 404, in,
        .in_send_pid = true,
    );
}

static Result _ldnGetNetworkInfo(Service* srv, LdnNetworkInfo *out) {
    return _ldnObjectDispatch(srv, 1,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

static Result _ldnGetIpv4Address(Service* srv, LdnIpv4Address *addr, LdnSubnetMask *mask) {
    struct {
        LdnIpv4Address addr;
        LdnSubnetMask mask;
    } out;

    Result rc = _ldnObjectDispatchOut(srv, 2, out);
    if (R_SUCCEEDED(rc)) {
        if (addr) *addr = out.addr;
        if (mask) *mask = out.mask;
    }
    return rc;
}

static Result _ldnGetDisconnectReason(Service* srv, LdnDisconnectReason *out) {
    s16 tmp=0;
    Result rc = _ldnObjectDispatchOut(srv, 3, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

static Result _ldnGetSecurityParameter(Service* srv, LdnSecurityParameter *out) {
    return _ldnObjectDispatchOut(srv, 4, *out);
}

static Result _ldnGetNetworkConfig(Service* srv, LdnNetworkConfig *out) {
    return _ldnObjectDispatchOut(srv, 5, *out);
}

static Result _ldnScan(s32 channel, const LdnScanFilter *filter, LdnNetworkInfo *network_info, s32 count, s32 *total_out, u32 cmd_id) {
    const struct {
        s16 channel;
        u8 pad[6];
        LdnScanFilter filter;
    } in = { channel, {0}, *filter};

    s16 out=0;
    Result rc = _ldnObjectDispatchInOut(&g_ldnSrv, cmd_id, in, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { network_info, count*sizeof(LdnNetworkInfo) } },
    );
    if (R_SUCCEEDED(rc) && total_out) *total_out = out;
    return rc;
}

static void _ldnCopyNetworkConfig(const LdnNetworkConfig *in, LdnNetworkConfig *out) {
    memset(out, 0, sizeof(*out));

    out->intent_id.local_communication_id = in->intent_id.local_communication_id;
    out->intent_id.scene_id = in->intent_id.scene_id;
    out->channel = in->channel;
    out->node_count_max = in->node_count_max;
    out->local_communication_version = in->local_communication_version;
}

static s16 _ldnChannelToOldBand(s16 channel) {
    if (channel < 15) return 24;
    else return 50;
}

static u16 _ldnChannelToChannelBand(s16 channel) {
    if (!channel) return channel;
    u16 tmp_channel = channel & 0x3FF; // Official sw just masks with u16.

    u16 band = 0x3F; // Invalid
    if (tmp_channel < 15) band = 2;
    else if (tmp_channel >= 32 && tmp_channel < 178) band = 5;

    return channel | (band<<10);
}

static s16 _ldnChannelBandToChannel(u16 val) {
    return val & 0x3FF;
}

Result ldnGetState(LdnState *out) {
    u32 tmp=0;
    Result rc = _ldnCmdNoInOutU32(&g_ldnSrv, &tmp, 0);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result ldnGetNetworkInfo(LdnNetworkInfo *out) {
    return _ldnGetNetworkInfo(&g_ldnSrv, out);
}

Result ldnGetIpv4Address(LdnIpv4Address *addr, LdnSubnetMask *mask) {
    return _ldnGetIpv4Address(&g_ldnSrv, addr, mask);
}

Result ldnGetDisconnectReason(LdnDisconnectReason *out) {
    return _ldnGetDisconnectReason(&g_ldnSrv, out);
}

Result ldnGetSecurityParameter(LdnSecurityParameter *out) {
    return _ldnGetSecurityParameter(&g_ldnSrv, out);
}

Result ldnGetNetworkConfig(LdnNetworkConfig *out) {
    return _ldnGetNetworkConfig(&g_ldnSrv, out);
}

Result ldnGetStateChangeEvent(Event* out_event) {
    return _ldnCmdGetEvent(&g_ldnSrv, out_event, true, 100);
}

Result ldnGetNetworkInfoAndHistory(LdnNetworkInfo *network_info, LdnNodeLatestUpdate *nodes, s32 count) {
    return _ldnObjectDispatch(&g_ldnSrv, 101,
        .buffer_attrs = {
            SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
        },
        .buffers = {
            { network_info, sizeof(*network_info) },
            { nodes, count*sizeof(LdnNodeLatestUpdate) },
        },
    );
}

Result ldnScan(s32 channel, const LdnScanFilter *filter, LdnNetworkInfo *network_info, s32 count, s32 *total_out) {
    LdnScanFilter tmp_filter = *filter;
    tmp_filter.flags &= 0x37;
    return _ldnScan(channel, &tmp_filter, network_info, count, total_out, 102);
}

Result ldnScanPrivate(s32 channel, const LdnScanFilter *filter, LdnNetworkInfo *network_info, s32 count, s32 *total_out) {
    LdnScanFilter tmp_filter = *filter;
    tmp_filter.flags &= 0x3F;
    return _ldnScan(channel, filter, network_info, count, total_out, 103);
}

Result ldnSetWirelessControllerRestriction(LdnWirelessControllerRestriction restriction) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdInU32NoOut(&g_ldnSrv, restriction, 104);
}

Result ldnSetProtocol(LdnProtocol protocol) {
    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdInU32NoOut(&g_ldnSrv, protocol, 106);
}

Result ldnOpenAccessPoint(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 200);
}

Result ldnCloseAccessPoint(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 201);
}

Result ldnCreateNetwork(const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, const LdnNetworkConfig *network_config) {
    LdnNetworkConfig tmp_network_config;
    LdnUserConfig tmp_user={0};
    memcpy(tmp_user.user_name, user_config->user_name, sizeof(tmp_user.user_name)-1);
    _ldnCopyNetworkConfig(network_config, &tmp_network_config);

    const struct {
        LdnSecurityConfig sec_config;
        LdnUserConfig user_config;
        u32 pad;
        LdnNetworkConfig network_config;
    } in = { *sec_config, tmp_user, 0, tmp_network_config };

    return _ldnObjectDispatchIn(&g_ldnSrv, 202, in);
}

Result ldnCreateNetworkPrivate(const LdnSecurityConfig *sec_config, const LdnSecurityParameter *sec_param, const LdnUserConfig *user_config, const LdnNetworkConfig *network_config, const LdnAddressEntry *addrs, s32 count) {
    LdnNetworkConfig tmp_network_config;
    LdnUserConfig tmp_user={0};
    memcpy(tmp_user.user_name, user_config->user_name, sizeof(tmp_user.user_name)-1);
    _ldnCopyNetworkConfig(network_config, &tmp_network_config);

    const struct {
        LdnSecurityConfig sec_config;
        LdnSecurityParameter sec_param;
        LdnUserConfig user_config;
        u32 pad;
        LdnNetworkConfig network_config;
    } in = { *sec_config, *sec_param, tmp_user, 0, tmp_network_config };

    return _ldnObjectDispatchIn(&g_ldnSrv, 203, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { addrs, count*sizeof(LdnAddressEntry) } },
    );
}

Result ldnDestroyNetwork(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 204);
}

Result ldnReject(LdnIpv4Address addr) {
    return _ldnObjectDispatchIn(&g_ldnSrv, 205, addr);
}

Result ldnSetAdvertiseData(const void* buffer, size_t size) {
    return _ldnObjectDispatch(&g_ldnSrv, 206,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result ldnSetStationAcceptPolicy(LdnAcceptPolicy policy) {
    u8 tmp=policy;
    return _ldnObjectDispatchIn(&g_ldnSrv, 207, tmp);
}

Result ldnAddAcceptFilterEntry(LdnMacAddress addr) {
    return _ldnObjectDispatchIn(&g_ldnSrv, 208, addr);
}

Result ldnClearAcceptFilter(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 209);
}

Result ldnOpenStation(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 300);
}

Result ldnCloseStation(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 301);
}

Result ldnConnect(const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, s32 version, u32 option, const LdnNetworkInfo *network_info) {
    LdnUserConfig tmp_user={0};
    memcpy(tmp_user.user_name, user_config->user_name, sizeof(tmp_user.user_name)-1);

    const struct {
        LdnSecurityConfig sec_config;
        LdnUserConfig user_config;
        s32 version;
        u32 option;
    } in = { *sec_config, tmp_user, version, option };

    return _ldnObjectDispatchIn(&g_ldnSrv, 302, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { network_info, sizeof(*network_info) } },
    );
}

Result ldnConnectPrivate(const LdnSecurityConfig *sec_config, const LdnSecurityParameter *sec_param, const LdnUserConfig *user_config, s32 version, u32 option, const LdnNetworkConfig *network_config) {
    LdnNetworkConfig tmp_network_config;
    LdnUserConfig tmp_user={0};
    memcpy(tmp_user.user_name, user_config->user_name, sizeof(tmp_user.user_name)-1);
    _ldnCopyNetworkConfig(network_config, &tmp_network_config);

    const struct {
        LdnSecurityConfig sec_config;
        LdnSecurityParameter sec_param;
        LdnUserConfig user_config;
        s32 version;
        u32 option;
        u32 pad;
        LdnNetworkConfig network_config;
    } in = { *sec_config, *sec_param, tmp_user, version, option, 0, tmp_network_config };

    return _ldnObjectDispatchIn(&g_ldnSrv, 303, in);
}

Result ldnDisconnect(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 304);
}

Result ldnSetOperationMode(LdnOperationMode mode) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if ((g_ldnServiceType == LdnServiceType_System && hosversionBefore(4,0,0)) ||
        (g_ldnServiceType == LdnServiceType_User && hosversionBefore(19,0,0)))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdInU32NoOut(&g_ldnSrv, mode, g_ldnServiceType == LdnServiceType_System ? 402 : 403);
}

Result ldnEnableActionFrame(const LdnActionFrameSettings *settings) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnObjectDispatchIn(&g_ldnSrv, 500, *settings);
}

Result ldnDisableActionFrame(void) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdNoIO(&g_ldnSrv, 501);
}

Result ldnSendActionFrame(const void* data, size_t size, LdnMacAddress destination, LdnMacAddress bssid, s16 channel, u32 flags) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    s16 tmp_band = 0, tmp_channel = 0;
    if (hosversionBefore(20,0,0)) {
        tmp_channel = channel;
        tmp_band = _ldnChannelToOldBand(channel);
    }
    else
        tmp_band = _ldnChannelToChannelBand(channel);

    const struct {
        LdnMacAddress destination;
        LdnMacAddress bssid;
        s16 band; // These are a single u16 with [20.0.0+].
        s16 channel;
        u32 flags;
    } in = { destination, bssid, tmp_band, tmp_channel, flags};

    return _ldnObjectDispatchIn(&g_ldnSrv, 502, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { data, size } },
    );
}

Result ldnRecvActionFrame(void* data, size_t size, LdnMacAddress *addr0, LdnMacAddress *addr1, s16 *channel, u32 *out_size, s32 *link_level, u32 flags) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        LdnMacAddress addr0;
        LdnMacAddress addr1;
        s16 band; // These are a single u16 with [20.0.0+].
        s16 channel;
        u32 size;
        s32 link_level;
    } out;

    Result rc = _ldnObjectDispatchInOut(&g_ldnSrv, 503, flags, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { data, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (addr0) *addr0 = out.addr0;
        if (addr1) *addr1 = out.addr1;
        if (channel) {
            if (hosversionBefore(20,0,0))
                *channel = out.channel;
            else
                *channel = _ldnChannelBandToChannel(out.band);
        }
        if (out_size) *out_size = out.size;
        if (link_level) *link_level = out.link_level;
    }
    return rc;
}

Result ldnSetHomeChannel(s16 channel) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    s16 tmp_band = 0, tmp_channel = 0;
    if (hosversionBefore(20,0,0)) {
        tmp_channel = channel;
        tmp_band = _ldnChannelToOldBand(channel);

        const struct {
            s16 band;
            s16 channel;
        } in = { tmp_band, tmp_channel };

        return _ldnObjectDispatchIn(&g_ldnSrv, 505, in);
    }
    else
        tmp_band = _ldnChannelToChannelBand(channel);

    return _ldnCmdInU16NoOut(&g_ldnSrv, tmp_band, 505);
}

Result ldnSetTxPower(s16 power) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdInU16NoOut(&g_ldnSrv, power, 600);
}

Result ldnResetTxPower(void) {
    if (!serviceIsActive(&g_ldnSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdNoIO(&g_ldnSrv, 601);
}

