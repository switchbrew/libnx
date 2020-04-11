#include <string.h>
#include "service_guard.h"
#include "services/nifm.h"
#include "runtime/hosversion.h"

static NifmServiceType g_nifmServiceType;

static Service g_nifmSrv;
static Service g_nifmIGS;

static Result _nifmCreateGeneralService(Service* srv_out);
static Result _nifmCreateGeneralServiceOld(Service* srv_out);

NX_GENERATE_SERVICE_GUARD_PARAMS(nifm, (NifmServiceType service_type), (service_type));

Result _nifmInitialize(NifmServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    g_nifmServiceType = service_type;
    switch (g_nifmServiceType) {
        case NifmServiceType_User:
            rc = smGetService(&g_nifmSrv, "nifm:u");
            break;
        case NifmServiceType_System:
            rc = smGetService(&g_nifmSrv, "nifm:s");
            break;
        case NifmServiceType_Admin:
            rc = smGetService(&g_nifmSrv, "nifm:a");
            break;
    }
    
    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(3,0,0))
            rc = _nifmCreateGeneralService(&g_nifmIGS);
        else
            rc = _nifmCreateGeneralServiceOld(&g_nifmIGS);
    }

    return rc;
}

void _nifmCleanup(void) {
    serviceClose(&g_nifmIGS);
    serviceClose(&g_nifmSrv);
}

Service* nifmGetServiceSession_StaticService(void) {
    return &g_nifmSrv;
}

Service* nifmGetServiceSession_GeneralService(void) {
    return &g_nifmIGS;
}

static Result _nifmCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _nifmCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

/*static Result _nifmCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}*/

static Result _nifmCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nifmCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _nifmCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _nifmCmdInU8NoOut(Service* srv, u8 inval, u64 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _nifmCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    return _nifmCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _nifmCreateGeneralServiceOld(Service* srv_out) {
    return _nifmCmdGetSession(&g_nifmSrv, srv_out, 4);
}

static Result _nifmCreateGeneralService(Service* srv_out) {
    u64 reserved=0;
    serviceAssumeDomain(&g_nifmSrv);
    return serviceDispatchIn(&g_nifmSrv, 5, reserved,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static void _nifmConvertSfToNetworkProfileData(const NifmSfNetworkProfileData *in, NifmNetworkProfileData *out) {
    memset(out, 0, sizeof(*out));

    out->uuid = in->uuid;
    strncpy(out->network_name, in->network_name, sizeof(in->network_name));
    out->network_name[sizeof(out->network_name)-1] = 0;

    out->unk_x50 = in->unk_x112;
    out->unk_x54 = in->unk_x113;
    out->unk_x58 = in->unk_x114;
    out->unk_x59 = in->unk_x115;

    out->wireless_setting_data.ssid_len = in->wireless_setting_data.ssid_len;
    if (out->wireless_setting_data.ssid_len > sizeof(out->wireless_setting_data.ssid)-1) out->wireless_setting_data.ssid_len = sizeof(out->wireless_setting_data.ssid)-1;
    if (out->wireless_setting_data.ssid_len) memcpy(out->wireless_setting_data.ssid, in->wireless_setting_data.ssid, out->wireless_setting_data.ssid_len);
    out->wireless_setting_data.unk_x22 = in->wireless_setting_data.unk_x21;
    out->wireless_setting_data.unk_x24 = in->wireless_setting_data.unk_x22;
    out->wireless_setting_data.unk_x28 = in->wireless_setting_data.unk_x23;
    memcpy(out->wireless_setting_data.passphrase, in->wireless_setting_data.passphrase, sizeof(out->wireless_setting_data.passphrase));

    memcpy(&out->ip_setting_data, &in->ip_setting_data, sizeof(out->ip_setting_data));
}

static void _nifmConvertSfFromNetworkProfileData(const NifmNetworkProfileData *in, NifmSfNetworkProfileData *out) {
    memset(out, 0, sizeof(*out));

    out->uuid = in->uuid;
    strncpy(out->network_name, in->network_name, sizeof(out->network_name));
    out->network_name[sizeof(out->network_name)-1] = 0;

    out->unk_x112 = in->unk_x50;
    out->unk_x113 = in->unk_x54;
    out->unk_x114 = in->unk_x58;
    out->unk_x115 = in->unk_x59;

    out->wireless_setting_data.ssid_len = in->wireless_setting_data.ssid_len;
    memcpy(out->wireless_setting_data.ssid, in->wireless_setting_data.ssid, sizeof(out->wireless_setting_data.ssid)-1);
    out->wireless_setting_data.unk_x21 = in->wireless_setting_data.unk_x22;
    out->wireless_setting_data.unk_x22 = in->wireless_setting_data.unk_x24;
    out->wireless_setting_data.unk_x23 = in->wireless_setting_data.unk_x28;
    memcpy(out->wireless_setting_data.passphrase, in->wireless_setting_data.passphrase, sizeof(out->wireless_setting_data.passphrase));

    memcpy(&out->ip_setting_data, &in->ip_setting_data, sizeof(out->ip_setting_data));
}

NifmClientId nifmGetClientId(void) {
    NifmClientId id={0};
    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatch(&g_nifmIGS, 1,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { &id, sizeof(id) } },
    );
    if (R_FAILED(rc)) id.id = 0;
    return id;
}

Result nifmGetCurrentNetworkProfile(NifmNetworkProfileData *profile) {
    NifmSfNetworkProfileData tmp={0};
    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatch(&g_nifmIGS, 5,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { &tmp, sizeof(tmp) } },
    );
    if (R_SUCCEEDED(rc)) _nifmConvertSfToNetworkProfileData(&tmp, profile);
    return rc;
}

Result nifmGetNetworkProfile(Uuid uuid, NifmNetworkProfileData *profile) {
    NifmSfNetworkProfileData tmp={0};
    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchIn(&g_nifmIGS, 8, uuid,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { &tmp, sizeof(tmp) } },
    );
    if (R_SUCCEEDED(rc)) _nifmConvertSfToNetworkProfileData(&tmp, profile);
    return rc;
}

Result nifmSetNetworkProfile(const NifmNetworkProfileData *profile, Uuid *uuid) {
    NifmSfNetworkProfileData tmp={0};
    _nifmConvertSfFromNetworkProfileData(profile, &tmp);
    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchOut(&g_nifmIGS, 9, *uuid,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { &tmp, sizeof(tmp) } },
    );
    return rc;
}

Result nifmGetCurrentIpAddress(u32* out) {
    NifmIpV4Address tmp={0};
    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchOut(&g_nifmIGS, 12, tmp);
    if (R_SUCCEEDED(rc) && out) *out = *((u32*)tmp.addr);
    return rc;
}

Result nifmGetCurrentIpConfigInfo(u32 *current_addr, u32 *subnet_mask, u32 *gateway, u32 *primary_dns_server, u32 *secondary_dns_server) {
    struct {
        NifmIpAddressSetting ip_setting;
        NifmDnsSetting dns_setting;
    } out;

    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchOut(&g_nifmIGS, 15, out);
    if (R_SUCCEEDED(rc)) {
        if (current_addr) *current_addr = *((u32*)out.ip_setting.current_addr.addr);
        if (subnet_mask) *subnet_mask = *((u32*)out.ip_setting.subnet_mask.addr);
        if (gateway) *gateway = *((u32*)out.ip_setting.gateway.addr);
        if (primary_dns_server) *primary_dns_server = *((u32*)out.dns_setting.primary_dns_server.addr);
        if (secondary_dns_server) *secondary_dns_server = *((u32*)out.dns_setting.secondary_dns_server.addr);
    }
    return rc;
}

Result nifmSetWirelessCommunicationEnabled(bool enable) {
    if (g_nifmServiceType < NifmServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nifmCmdInBoolNoOut(&g_nifmIGS, enable, 16);
}

Result nifmIsWirelessCommunicationEnabled(bool* out) {
    return _nifmCmdNoInOutBool(&g_nifmIGS, out, 17);
}

Result nifmGetInternetConnectionStatus(NifmInternetConnectionType* connectionType, u32* wifiStrength, NifmInternetConnectionStatus* connectionStatus) {
    struct {
        u8 out1;
        u8 out2;
        u8 out3;
    } out;

    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchOut(&g_nifmIGS, 18, out);
    if (R_SUCCEEDED(rc)) {
        if (connectionType) *connectionType = out.out1;
        if (wifiStrength) *wifiStrength = out.out2;
        if (connectionStatus) *connectionStatus = out.out3;
    }
    return rc;
}

Result nifmIsEthernetCommunicationEnabled(bool* out) {
    return _nifmCmdNoInOutBool(&g_nifmIGS, out, 20);
}

bool nifmIsAnyInternetRequestAccepted(NifmClientId id) {
    u8 tmp=0;
    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchOut(&g_nifmIGS, 21, tmp,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { &id, sizeof(id) } },
    );
    return R_SUCCEEDED(rc) ? tmp & 1 : 0;
}

Result nifmIsAnyForegroundRequestAccepted(bool* out) {
    return _nifmCmdNoInOutBool(&g_nifmIGS, out, 22);
}

Result nifmPutToSleep(void) {
    return _nifmCmdNoIO(&g_nifmIGS, 23);
}

Result nifmWakeUp(void) {
    return _nifmCmdNoIO(&g_nifmIGS, 24);
}
