#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/ldn.h"
#include "runtime/hosversion.h"

static LdnServiceType g_ldnServiceType;

static Service g_ldnSrv;
static Service g_ldnmSrv;

static Result _ldnGetSession(Service* srv, Service* srv_out, u32 cmd_id);

static Result _ldnCmdNoIO(Service* srv, u32 cmd_id);

static Result _ldnCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id);

static Result _ldnCmdInitialize(void);
static Result _ldnCmdInitialize2(u32 inval);

static Result _ldnGetNetworkInfo(Service* srv, LdnNetworkInfo *out);

static Result _ldnGetIpv4Address(Service* srv, LdnIpv4Address *addr, LdnSubnetMask *mask);

static Result _ldnGetSecurityParameter(Service* srv, LdnSecurityParameter *out);
static Result _ldnGetNetworkConfig(Service* srv, LdnNetworkConfig *out);

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

Result ldnmGetStateForMonitor(LdnState *out) {
    u32 tmp=0;
    Result rc = _ldnCmdNoInOutU32(&g_ldnmSrv, &tmp, 0);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result ldnmGetNetworkInfoForMonitor(LdnNetworkInfo *out) {
    return _ldnGetNetworkInfo(&g_ldnmSrv, out);
}

Result ldnmGetIpv4AddressForMonitor(LdnIpv4Address *addr, LdnSubnetMask *mask) {
    return _ldnGetIpv4Address(&g_ldnmSrv, addr, mask);
}

Result ldnmGetSecurityParameterForMonitor(LdnSecurityParameter *out) {
    return _ldnGetSecurityParameter(&g_ldnmSrv, out);
}

Result ldnmGetNetworkConfigForMonitor(LdnNetworkConfig *out) {
    return _ldnGetNetworkConfig(&g_ldnmSrv, out);
}

// ldn

NX_GENERATE_SERVICE_GUARD_PARAMS(ldn, (LdnServiceType service_type), (service_type));

Result _ldnInitialize(LdnServiceType service_type) {
    Service srv_creator={0};
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    g_ldnServiceType = service_type;
    switch (g_ldnServiceType) {
        case LdnServiceType_User:
            rc = smGetService(&srv_creator, "ldn:u");
            break;
        case LdnServiceType_System:
            rc = smGetService(&srv_creator, "ldn:s");
            break;
    }

    if (R_SUCCEEDED(rc)) rc = _ldnGetSession(&srv_creator, &g_ldnSrv, 0); // CreateSystemLocalCommunicationService/CreateUserLocalCommunicationService
    serviceClose(&srv_creator);

    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(7,0,0))
            rc = _ldnCmdInitialize2(0x1);
        else
            rc = _ldnCmdInitialize();
    }

    return rc;
}

void _ldnCleanup(void) {
    if (serviceIsActive(&g_ldnSrv)) _ldnCmdNoIO(&g_ldnSrv, 401); // Finalize(System)
    serviceClose(&g_ldnSrv);
}

Service* ldnGetServiceSession_LocalCommunicationService(void) {
    return &g_ldnSrv;
}

static Result _ldnGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _ldnCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _ldnCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _ldnCmdInU32NoOut(Service* srv, u32 in, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _ldnCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _ldnCmdInitialize(void) {
    u64 reserved=0;
    return serviceDispatchIn(&g_ldnSrv, 400, reserved,
        .in_send_pid = true,
    );
}

static Result _ldnCmdInitialize2(u32 inval) {
    const struct {
        u32 inval;
        u32 pad;
        u64 reserved;
    } in = { inval, 0, 0};

    u32 cmd_id = g_ldnServiceType == LdnServiceType_User ? 402 : 403;
    return serviceDispatchIn(&g_ldnSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _ldnGetNetworkInfo(Service* srv, LdnNetworkInfo *out) {
    return serviceDispatch(srv, 1,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

static Result _ldnGetIpv4Address(Service* srv, LdnIpv4Address *addr, LdnSubnetMask *mask) {
    struct {
        LdnIpv4Address addr;
        LdnSubnetMask mask;
    } out;

    Result rc = serviceDispatchOut(srv, 2, out);
    if (R_SUCCEEDED(rc)) {
        if (addr) *addr = out.addr;
        if (mask) *mask = out.mask;
    }
    return rc;
}

static Result _ldnGetDisconnectReason(Service* srv, LdnDisconnectReason *out) {
    s16 tmp=0;
    Result rc = serviceDispatchOut(srv, 3, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

static Result _ldnGetSecurityParameter(Service* srv, LdnSecurityParameter *out) {
    return serviceDispatchOut(srv, 4, *out);
}

static Result _ldnGetNetworkConfig(Service* srv, LdnNetworkConfig *out) {
    return serviceDispatchOut(srv, 5, *out);
}

static Result _ldnScan(s32 channel, const LdnScanFilter *filter, LdnNetworkInfo *network_info, s32 count, s32 *total_out, u32 cmd_id) {
    const struct {
        s16 channel;
        u8 pad[6];
        LdnScanFilter filter;
    } in = { channel, {0}, *filter};

    s16 out=0;
    Result rc = serviceDispatchInOut(&g_ldnSrv, cmd_id, in, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { network_info, count*sizeof(LdnNetworkInfo) } },
    );
    if (R_SUCCEEDED(rc) && total_out) *total_out = out;
    return rc;
}

static void _ldnCopyNetworkConfig(const LdnNetworkConfig *in, LdnNetworkConfig *out) {
    memset(out, 0, sizeof(*out));

    out->local_communication_id = in->local_communication_id;
    out->userdata_filter = in->userdata_filter;
    out->network_channel = in->network_channel;
    out->participant_max = in->participant_max;
    out->local_communication_version = in->local_communication_version;
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

Result ldnAttachStateChangeEvent(Event* out_event) {
    return _ldnCmdGetEvent(&g_ldnSrv, out_event, true, 100);
}

Result ldnGetNetworkInfoLatestUpdate(LdnNetworkInfo *network_info, LdnNodeLatestUpdate *nodes, s32 count) {
    return serviceDispatch(&g_ldnSrv, 101,
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
    memcpy(tmp_user.nickname, user_config->nickname, sizeof(tmp_user.nickname));
    _ldnCopyNetworkConfig(network_config, &tmp_network_config);

    const struct {
        LdnSecurityConfig sec_config;
        LdnUserConfig user_config;
        u32 pad;
        LdnNetworkConfig network_config;
    } in = { *sec_config, tmp_user, 0, tmp_network_config };

    return serviceDispatchIn(&g_ldnSrv, 202, in);
}

Result ldnCreateNetworkPrivate(const LdnSecurityConfig *sec_config, const LdnSecurityParameter *sec_param, const LdnUserConfig *user_config, const LdnNetworkConfig *network_config, const LdnAddressEntry *addrs, s32 count) {
    LdnNetworkConfig tmp_network_config;
    LdnUserConfig tmp_user={0};
    memcpy(tmp_user.nickname, user_config->nickname, sizeof(tmp_user.nickname));
    _ldnCopyNetworkConfig(network_config, &tmp_network_config);

    const struct {
        LdnSecurityConfig sec_config;
        LdnSecurityParameter sec_param;
        LdnUserConfig user_config;
        u32 pad;
        LdnNetworkConfig network_config;
    } in = { *sec_config, *sec_param, tmp_user, 0, tmp_network_config };

    return serviceDispatchIn(&g_ldnSrv, 203, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { addrs, count*sizeof(LdnAddressEntry) } },
    );
}

Result ldnDestroyNetwork(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 204);
}

Result ldnReject(LdnIpv4Address addr) {
    return serviceDispatchIn(&g_ldnSrv, 205, addr);
}

Result ldnSetAdvertiseData(const void* buffer, size_t size) {
    return serviceDispatch(&g_ldnSrv, 206,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result ldnSetStationAcceptPolicy(LdnAcceptPolicy policy) {
    u8 tmp=policy;
    return serviceDispatchIn(&g_ldnSrv, 207, tmp);
}

Result ldnAddAcceptFilterEntry(LdnMacAddress addr) {
    return serviceDispatchIn(&g_ldnSrv, 208, addr);
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
    memcpy(tmp_user.nickname, user_config->nickname, sizeof(tmp_user.nickname));

    const struct {
        LdnSecurityConfig sec_config;
        LdnUserConfig user_config;
        s32 version;
        u32 option;
    } in = { *sec_config, tmp_user, version, option };

    return serviceDispatchIn(&g_ldnSrv, 302, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { network_info, sizeof(*network_info) } },
    );
}

Result ldnConnectPrivate(const LdnSecurityConfig *sec_config, const LdnSecurityParameter *sec_param, const LdnUserConfig *user_config, s32 version, u32 option, const LdnNetworkConfig *network_config) {
    LdnNetworkConfig tmp_network_config;
    LdnUserConfig tmp_user={0};
    memcpy(tmp_user.nickname, user_config->nickname, sizeof(tmp_user.nickname));
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

    return serviceDispatchIn(&g_ldnSrv, 303, in);
}

Result ldnDisconnect(void) {
    return _ldnCmdNoIO(&g_ldnSrv, 304);
}

Result ldnSetOperationMode(LdnOperationMode mode) {
    if (!serviceIsActive(&g_ldnSrv) || g_ldnServiceType != LdnServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _ldnCmdInU32NoOut(&g_ldnSrv, mode, 402);
}

