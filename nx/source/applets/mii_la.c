#include <string.h>
#include "libapplet_internal.h"
#include "applets/mii_la.h"
#include "runtime/hosversion.h"

static s32 _miiLaGetVersion(void) {
    s32 version = 0x3;

    if (hosversionAtLeast(10,2,0)) version = 0x4;

    return version;
}

static Result _miiLaShow(const MiiLaAppletInput *in, void* out, size_t out_size) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage;

    rc = appletCreateLibraryApplet(&holder, AppletId_LibraryAppletMiiEdit, LibAppletMode_AllForeground);
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
        rc = appletStorageRead(&storage, 0, out, out_size);
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

static Result _miiLaGetResult(u32 res) {
    Result rc=0;

    if (res == 0)
        rc = 0;
    else if (res == 1)
        rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    else
        rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);

    return rc;
}

Result miiLaShowMiiEdit(MiiSpecialKeyCode special_key_code) {
    Result rc=0;
    MiiLaAppletInput in = {.version = _miiLaGetVersion(), .mode = MiiLaAppletMode_ShowMiiEdit, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    rc = _miiLaShow(&in, &out, sizeof(out));
    if (R_SUCCEEDED(rc)) {
        rc = _miiLaGetResult(out.res);
        rc = R_VALUE(rc) == MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit) ? 0 : rc;
    }
    return rc;
}

Result miiLaAppendMii(MiiSpecialKeyCode special_key_code, s32 *index) {
    Result rc=0;
    MiiLaAppletInput in = {.version = _miiLaGetVersion(), .mode = MiiLaAppletMode_AppendMii, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    rc = _miiLaShow(&in, &out, sizeof(out));
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(out.res);
    if (R_SUCCEEDED(rc)) *index = out.index;
    return rc;
}

Result miiLaAppendMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, s32 *index) {
    Result rc=0;
    MiiLaAppletInput in = {.version = _miiLaGetVersion(), .mode = MiiLaAppletMode_AppendMiiImage, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    _miiLaInitializeValidUuidArray(&in, valid_uuid_array, count);
    rc = _miiLaShow(&in, &out, sizeof(out));
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(out.res);
    if (R_SUCCEEDED(rc)) *index = out.index;
    return rc;
}

Result miiLaUpdateMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, Uuid used_uuid, s32 *index) {
    Result rc=0;
    MiiLaAppletInput in = {.version = _miiLaGetVersion(), .mode = MiiLaAppletMode_UpdateMiiImage, .special_key_code = special_key_code};
    MiiLaAppletOutput out={0};

    _miiLaInitializeValidUuidArray(&in, valid_uuid_array, count);
    in.used_uuid = used_uuid;
    rc = _miiLaShow(&in, &out, sizeof(out));
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(out.res);
    if (R_SUCCEEDED(rc)) *index = out.index;
    return rc;
}

Result miiLaCreateMii(MiiSpecialKeyCode special_key_code, MiiCharInfo *out_char) {
    Result rc=0;
    MiiLaAppletInput in = {.version = _miiLaGetVersion(), .mode = MiiLaAppletMode_CreateMii, .special_key_code = special_key_code};
    MiiLaAppletOutputForCharInfoEditing out={0};

    if (hosversionBefore(10,2,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _miiLaShow(&in, &out, sizeof(out));
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(out.res);
    if (R_SUCCEEDED(rc)) *out_char = out.char_info;
    return rc;
}

Result miiLaEditMii(MiiSpecialKeyCode special_key_code, const MiiCharInfo *in_char, MiiCharInfo *out_char) {
    Result rc=0;
    MiiLaAppletInput in = {.version = _miiLaGetVersion(), .mode = MiiLaAppletMode_EditMii, .special_key_code = special_key_code};
    MiiLaAppletOutputForCharInfoEditing out={0};

    if (hosversionBefore(10,2,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    // sdknso does various validation with in_char, we won't do so.

    in.char_info.char_info = *in_char;

    rc = _miiLaShow(&in, &out, sizeof(out));
    if (R_SUCCEEDED(rc)) rc = _miiLaGetResult(out.res);
    if (R_SUCCEEDED(rc)) *out_char = out.char_info;
    return rc;
}

