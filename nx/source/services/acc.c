#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/acc.h"
#include "services/applet.h"
#include "runtime/env.h"
#include "runtime/hosversion.h"

static AccountServiceType g_accServiceType;
static Service g_accSrv;
static AccountUid g_accPreselectedUserID;
static bool g_accPreselectedUserInitialized;

static Result _accountInitializeApplicationInfo(void);

static Result _accountGetPreselectedUser(AccountUid *uid);

NX_GENERATE_SERVICE_GUARD_PARAMS(account, (AccountServiceType service_type), (service_type));

Result _accountInitialize(AccountServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    Result rc2=0;
    AccountUid *userIdEnv = envGetUserIdStorage();

    g_accServiceType = service_type;
    switch (g_accServiceType) {
        case AccountServiceType_Application:
            rc = smGetService(&g_accSrv, "acc:u0");
            if (R_SUCCEEDED(rc)) rc = _accountInitializeApplicationInfo();
            break;
        case AccountServiceType_System:
            rc = smGetService(&g_accSrv, "acc:u1");
            break;
        case AccountServiceType_Administrator:
            rc = smGetService(&g_accSrv, "acc:su");
            break;
    }

    if (R_SUCCEEDED(rc)) {
        rc2 = _accountGetPreselectedUser(&g_accPreselectedUserID);
        if (R_SUCCEEDED(rc2)) {
            g_accPreselectedUserInitialized = true;
            if (userIdEnv) *userIdEnv = g_accPreselectedUserID;
        }
        else if (userIdEnv) {
            g_accPreselectedUserID = *userIdEnv;
            if (accountUidIsValid(&g_accPreselectedUserID)) g_accPreselectedUserInitialized = true;
        }
    }

    return rc;
}

void _accountCleanup(void) {
    serviceClose(&g_accSrv);
}

Service* accountGetServiceSession(void) {
    return &g_accSrv;
}

static Result _accountCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _accountInitializeApplicationInfo(void) {
    u64 pid_placeholder=0;
    return serviceDispatchIn(&g_accSrv, hosversionBefore(6,0,0) ? 100 : 140, pid_placeholder,
        .in_send_pid = true,
    );
}

Result accountGetUserCount(s32* user_count) {
    return _accountCmdNoInOutU32(&g_accSrv, (u32*)user_count, 0);
}

static Result _accountListAllUsers(AccountUid* uids) {
    return serviceDispatch(&g_accSrv, 2,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { uids, sizeof(AccountUid)*ACC_USER_LIST_SIZE } },
    );
}

Result accountListAllUsers(AccountUid* uids, s32 max_uids, s32 *actual_total) {
    Result rc=0;
    AccountUid temp_uids[ACC_USER_LIST_SIZE];
    memset(temp_uids, 0, sizeof(temp_uids));

    rc = _accountListAllUsers(temp_uids);

    if (R_SUCCEEDED(rc)) {
        s32 total_uids;
        for (total_uids=0; total_uids<ACC_USER_LIST_SIZE; total_uids++) {
            if (!accountUidIsValid(&temp_uids[total_uids])) break;
        }

        if (max_uids > total_uids) {
            max_uids = total_uids;
        }

        memcpy(uids, temp_uids, sizeof(AccountUid)*max_uids);
        *actual_total = max_uids;
    }

    return rc;
}

Result accountGetLastOpenedUser(AccountUid *uid) {
    return serviceDispatchOut(&g_accSrv, 4, *uid);
}

Result accountGetProfile(AccountProfile* out, AccountUid uid) {
    return serviceDispatchIn(&g_accSrv, 5, uid,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result accountIsUserRegistrationRequestPermitted(bool *out) {
    u64 pid_placeholder=0;
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_accSrv, 50, pid_placeholder, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result accountTrySelectUserWithoutInteraction(AccountUid *uid, bool is_network_service_account_required) {
    u8 tmp=is_network_service_account_required!=0;
    return serviceDispatchInOut(&g_accSrv, 51, tmp, *uid);
}

// IProfile

void accountProfileClose(AccountProfile* profile) {
    serviceClose(&profile->s);
}

static Result _accountProfileGet(AccountProfile* profile, AccountUserData* userdata, AccountProfileBase* profilebase) {
    return serviceDispatchOut(&profile->s, 0, *profilebase,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { userdata, sizeof(AccountUserData) } },
    );
}

static Result _accountProfileGetBase(AccountProfile* profile, AccountProfileBase* profilebase) {
    return serviceDispatchOut(&profile->s, 1, *profilebase);
}

Result accountProfileGet(AccountProfile* profile, AccountUserData* userdata, AccountProfileBase* profilebase) {
    Result rc=0;

    if (!serviceIsActive(&profile->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (userdata)
        rc = _accountProfileGet(profile, userdata, profilebase);
    else
        rc = _accountProfileGetBase(profile, profilebase);

    return rc;
}

Result accountProfileGetImageSize(AccountProfile* profile, u32* image_size) {
    if (!serviceIsActive(&profile->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _accountCmdNoInOutU32(&profile->s, image_size, 10);
}

Result accountProfileLoadImage(AccountProfile* profile, void* buf, size_t len, u32* image_size) {
    if (!serviceIsActive(&profile->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchOut(&profile->s, 11, *image_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, len } },
    );
}

static Result _accountGetPreselectedUser(AccountUid *uid) {
    Result rc=0;
    AppletStorage storage;
    s64 tmpsize=0;

    struct {
        u32  magicnum;//These two fields must match fixed values.
        u8   unk_x4;
        u8   pad[3];
        AccountUid uid;
        u8   unk_x18[0x70];//unused
    } storagedata;

    memset(&storagedata, 0, sizeof(storagedata));

    rc = appletPopLaunchParameter(&storage, AppletLaunchParameterKind_PreselectedUser);

    if (R_SUCCEEDED(rc)) {
        rc = appletStorageGetSize(&storage, &tmpsize);
        if (R_SUCCEEDED(rc) && tmpsize < sizeof(storagedata))
            rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

        if (R_SUCCEEDED(rc)) rc = appletStorageRead(&storage, 0, &storagedata, sizeof(storagedata));

        appletStorageClose(&storage);

        if (R_SUCCEEDED(rc) && (storagedata.magicnum!=0xc79497ca || storagedata.unk_x4!=1))
            rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

        if (R_SUCCEEDED(rc) && uid) *uid = storagedata.uid;
    }

    return rc;
}

Result accountGetPreselectedUser(AccountUid *uid) {
    if (!g_accPreselectedUserInitialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    *uid = g_accPreselectedUserID;

    return 0;
}

