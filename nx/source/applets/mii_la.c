#include <string.h>
#include "libapplet_internal.h"
#include "applets/mii_la.h"

static Result _miiLaShow(const MiiLaAppletInput *in, MiiLaAppletOutput *out) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage;

    rc = appletCreateLibraryApplet(&holder, AppletId_miiEdit, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(&holder, in, sizeof(*in));

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);

    appletHolderJoin(&holder);

    LibAppletExitReason reason = appletHolderGetExitReason(&holder);

    if (reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
        rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    }

    if (R_SUCCEEDED(rc)) rc = appletHolderPopOutData(&holder, &storage);

    if (R_SUCCEEDED(rc)) {
        rc = appletStorageRead(&storage, 0, out, sizeof(*out));
        appletStorageClose(&storage);
    }

    appletHolderClose(&holder);

    return rc;
}

static void _miiLaInitializeValidUuidArray(MiiLaAppletInput *in, const Uuid *in_array, s32 count) {
    memset(in->valid_uuid_array, 0, sizeof(in->valid_uuid_array));

    if (count > 8) count = 8;
    for (s32 i=0; i<count; i++) in->valid_uuid_array[i] = in_array[i];
}

static Result _miiLaGetResult(MiiLaAppletOutput *out) {
    Result rc=0;

    if (out->res == 0)
        rc = 0;
    else if (out->res == 1)
        rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    else
        rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);

    return rc;
}

Result miiLaShowMiiEdit(MiiSpecialKeyCode special_key_code) {
    Result rc=0;
    MiiLaAppletInput in = {.unk_x0 = 0x3, .mode = NfpLaMiiLaAppletMode_ShowMiiEdit, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    rc = _miiLaShow(&in, &out);
    if (R_SUCCEEDED(rc)) {
        rc = _miiLaGetResult(&out);
        rc = R_VALUE(rc) == MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit) ? 0 : rc;
    }
    return rc;
}

Result miiLaAppendMii(MiiSpecialKeyCode special_key_code, s32 *index) {
    Result rc=0;
    MiiLaAppletInput in = {.unk_x0 = 0x3, .mode = NfpLaMiiLaAppletMode_AppendMii, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    rc = _miiLaShow(&in, &out);
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(&out);
    if (R_SUCCEEDED(rc)) *index = out.index;
    return rc;
}

Result miiLaAppendMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, s32 *index) {
    Result rc=0;
    MiiLaAppletInput in = {.unk_x0 = 0x3, .mode = NfpLaMiiLaAppletMode_AppendMiiImage, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    _miiLaInitializeValidUuidArray(&in, valid_uuid_array, count);
    rc = _miiLaShow(&in, &out);
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(&out);
    if (R_SUCCEEDED(rc)) *index = out.index;
    return rc;
}

Result miiLaUpdateMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, Uuid used_uuid, s32 *index) {
    Result rc=0;
    MiiLaAppletInput in = {.unk_x0 = 0x3, .mode = NfpLaMiiLaAppletMode_UpdateMiiImage, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    _miiLaInitializeValidUuidArray(&in, valid_uuid_array, count);
    in.used_uuid = used_uuid;
    rc = _miiLaShow(&in, &out);
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(&out);
    if (R_SUCCEEDED(rc)) *index = out.index;
    return rc;
}

