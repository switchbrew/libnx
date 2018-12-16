#include <string.h>

#include "types.h"
#include "arm/atomics.h"
#include "services/acc.h"
#include "services/sm.h"
#include "services/applet.h"

static Service g_accSrv;
static u64 g_refCnt;

Result accountInitialize(void)
{
    Result rc=0;

    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_accSrv))
        return 0;

    rc = smGetService(&g_accSrv, "acc:u1");
    if (R_FAILED(rc)) rc = smGetService(&g_accSrv, "acc:u0");

    return rc;
}

void accountExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
        serviceClose(&g_accSrv);
}

Service* accountGetService(void) {
    return &g_accSrv;
}

Result accountGetUserCount(s32* user_count)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_accSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 user_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *user_count = resp->user_count;
        }
    }

    return rc;
}

static Result _accountListAllUsers(u128* userIDs)
{
    IpcCommand c;
    ipcInitialize(&c);

    Result rc=0;

    size_t bufsize = ACC_USER_LIST_SIZE * sizeof(u128);

    ipcAddRecvStatic(&c, userIDs, bufsize, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    rc = serviceIpcDispatch(&g_accSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result accountListAllUsers(u128* userIDs, size_t max_userIDs, size_t *actual_total)
{
    Result rc=0;
    u128 temp_userIDs[ACC_USER_LIST_SIZE];
    memset(temp_userIDs, 0, sizeof(temp_userIDs));

    rc = _accountListAllUsers(temp_userIDs);

    if (R_SUCCEEDED(rc)) {
        size_t total_userIDs;
        for (total_userIDs = 0; total_userIDs < ACC_USER_LIST_SIZE; total_userIDs++) {
            if (!temp_userIDs[total_userIDs])
                break;
        }

        if (max_userIDs > total_userIDs) {
            max_userIDs = total_userIDs;
        }

        memcpy(userIDs, temp_userIDs, sizeof(u128) * max_userIDs);
        *actual_total = max_userIDs;
    }

    return rc;
}

Result accountGetActiveUser(u128 *userID, bool *account_selected)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_accSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u128 userID;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && userID) {
            *userID = resp->userID;
            if (account_selected) {
                *account_selected = 0;
                if (*userID != 0) *account_selected = 1;
            }
        }
    }

    return rc;
}

Result accountGetProfile(AccountProfile* out, u128 userID) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u128 userID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->userID = userID;

    Result rc = serviceIpcDispatch(&g_accSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }

    return rc;
}

//IProfile implementation
Result accountProfileGet(AccountProfile* profile, AccountUserData* userdata, AccountProfileBase* profilebase) {
    IpcCommand c;
    ipcInitialize(&c);
    if (userdata) ipcAddRecvStatic(&c, userdata, sizeof(AccountUserData), 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = userdata==NULL ? 1 : 0;

    Result rc = serviceIpcDispatch(&profile->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            AccountProfileBase profilebase;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && profilebase) memcpy(profilebase, &resp->profilebase, sizeof(AccountProfileBase));
    }

    return rc;
}

Result accountProfileGetImageSize(AccountProfile* profile, size_t* image_size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&profile->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 image_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *image_size = resp->image_size;
        }
    }

    return rc;
}

Result accountProfileLoadImage(AccountProfile* profile, void* buf, size_t len, size_t* image_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;

    Result rc = serviceIpcDispatch(&profile->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 image_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *image_size = resp->image_size;
        }
    }

    return rc;
}

void accountProfileClose(AccountProfile* profile) {
    serviceClose(&profile->s);
}

Result accountGetPreselectedUser(u128 *userID) {
    Result rc=0;
    AppletStorage storage;
    s64 tmpsize=0;

    struct {
        u32  magicnum;//These two fields must match fixed values.
        u8   unk_x4;
        u8   pad[3];
        u128 userID;
        u8   unk_x18[0x70];//unused
    } PACKED storagedata;

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

        if (R_SUCCEEDED(rc) && userID) *userID = storagedata.userID;
    }

    return rc;
}

