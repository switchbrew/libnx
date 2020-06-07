#include <string.h>
#include "libapplet_internal.h"
#include "applets/nfp_la.h"

static Result _nfpLaShow(const NfpLaStartParamForAmiiboSettings *param, NfpLaReturnValueForAmiiboSettings *reply) {
    Result rc=0;
    size_t out_reply_size=0;
    u32 ver=1;
    LibAppletArgs commonargs;

    libappletArgsCreate(&commonargs, ver);

    rc = libappletLaunch(AppletId_cabinet, &commonargs, param, sizeof(*param), reply, sizeof(*reply), &out_reply_size);
    if (R_SUCCEEDED(rc)) { // sdknso doesn't validate out_reply_size.
        if (!reply->flags) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    }
    else
        reply->flags = 0;

    return rc;
}

static Result _nfpLaStartSettings(NfpLaStartParamTypeForAmiiboSettings type, const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, const NfpRegisterInfo *in_reg_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle, bool *reg_info_flag, NfpRegisterInfo *out_reg_info) {
    Result rc=0;
    NfpLaStartParamForAmiiboSettings param={0};
    NfpLaReturnValueForAmiiboSettings reply={0};

    param.type = type;
    param.flags = 0x1;
    if (in_tag_info) param.flags |= BIT(1);
    if (in_reg_info) param.flags |= BIT(2);
    param.unk_x3 = in_param->unk_x28;
    memcpy(param.unk_x4, in_param->unk_x0, sizeof(in_param->unk_x0));

    if (in_tag_info) memcpy(&param.tag_info, in_tag_info, sizeof(NfpTagInfo));
    if (in_reg_info) memcpy(&param.register_info, in_reg_info, sizeof(NfpRegisterInfo));

    memcpy(param.unk_x164, in_param->unk_x8, sizeof(in_param->unk_x8));

    rc = _nfpLaShow(&param, &reply);

    if (R_SUCCEEDED(rc)) {
        if (out_tag_info) memcpy(out_tag_info, &reply.tag_info, sizeof(NfpTagInfo));
        *handle = reply.handle;
        if (reg_info_flag || out_reg_info) {
            bool tmpflag = (reply.flags & BIT(2)) != 0;
            if (reg_info_flag) *reg_info_flag = tmpflag;
            if (tmpflag && out_reg_info) memcpy(out_reg_info, &reply.register_info, sizeof(NfpRegisterInfo));
        }
    }

    return rc;
}

Result nfpLaStartNicknameAndOwnerSettings(const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, const NfpRegisterInfo *in_reg_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle, bool *reg_info_flag, NfpRegisterInfo *out_reg_info) {
    return _nfpLaStartSettings(NfpLaStartParamTypeForAmiiboSettings_NicknameAndOwnerSettings, in_param, in_tag_info, in_reg_info, out_tag_info, handle, reg_info_flag, out_reg_info);
}

Result nfpLaStartGameDataEraser(const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle) {
    return _nfpLaStartSettings(NfpLaStartParamTypeForAmiiboSettings_GameDataEraser, in_param, in_tag_info, NULL, out_tag_info, handle, NULL, NULL);
}

Result nfpLaStartRestorer(const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle) {
    return _nfpLaStartSettings(NfpLaStartParamTypeForAmiiboSettings_Restorer, in_param, in_tag_info, NULL, out_tag_info, handle, NULL, NULL);
}

Result nfpLaStartFormatter(const NfpLaAmiiboSettingsStartParam *in_param, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle) {
    return _nfpLaStartSettings(NfpLaStartParamTypeForAmiiboSettings_Formatter, in_param, NULL, NULL, out_tag_info, handle, NULL, NULL);
}

