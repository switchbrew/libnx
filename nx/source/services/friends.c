#include "service_guard.h"
#include "sf/sessionmgr.h"
#include "services/friends.h"

static FriendsServiceType g_friendsServiceType;

static Service g_friendsSrv;
static Service g_friendsIFriendService;

static SessionMgr g_friendsSessionMgr;

NX_GENERATE_SERVICE_GUARD_PARAMS(friends, (FriendsServiceType service_type), (service_type));

NX_INLINE bool _friendsObjectIsChild(Service* s) {
    return s->session == g_friendsSrv.session;
}

static void _friendsObjectClose(Service* s) {
    if (!_friendsObjectIsChild(s)) {
        serviceClose(s);
    } else {
        int slot = sessionmgrAttachClient(&g_friendsSessionMgr);
        uint32_t object_id = serviceGetObjectId(s);

        serviceAssumeDomain(s);
        cmifMakeCloseRequest(armGetTls(), object_id);
        svcSendSyncRequest(sessionmgrGetClientSession(&g_friendsSessionMgr, slot));
        sessionmgrDetachClient(&g_friendsSessionMgr, slot);
    }
}

NX_INLINE Result _friendsObjectDispatchImpl(Service* s, u32 request_id,
                                            const void * in_data, u32 in_data_size,
                                            void * out_data, u32 out_data_size,
                                            SfDispatchParams disp)
{
    int slot = -1;
    if (_friendsObjectIsChild(s)) {
        slot = sessionmgrAttachClient(&g_friendsSessionMgr);
        if (slot < 0)
            __builtin_unreachable();
        disp.target_session = sessionmgrGetClientSession(&g_friendsSessionMgr, slot);
        serviceAssumeDomain(s);
    }

    Result rc = serviceDispatchImpl(s, request_id, in_data, in_data_size, out_data, out_data_size, disp);

    if (slot >= 0)
        sessionmgrDetachClient(&g_friendsSessionMgr, slot);

    return rc;
}

#define _friendsObjectDispatch(_s,_rid,...) \
    _friendsObjectDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _friendsObjectDispatchIn(_s,_rid,_in,...) \
    _friendsObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _friendsObjectDispatchOut(_s,_rid,_out,...) \
    _friendsObjectDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _friendsObjectDispatchInOut(_s,_rid,_in,_out,...) \
    _friendsObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

Result _friendsInitialize(FriendsServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    g_friendsServiceType = service_type;

    switch (g_friendsServiceType) {
        case FriendsServiceType_User:
            rc = smGetService(&g_friendsSrv, "friend:u");
            break;
        case FriendsServiceType_Administrator:
            rc = smGetService(&g_friendsSrv, "friend:a");
            break;
        case FriendsServiceType_Manager:
            rc = smGetService(&g_friendsSrv, "friend:m");
            break;
        case FriendsServiceType_Viewer:
            rc = smGetService(&g_friendsSrv, "friend:v");
            break;
        case FriendsServiceType_System:
            rc = smGetService(&g_friendsSrv, "friend:s");
            break;
    }

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_friendsSrv);

    if (R_SUCCEEDED(rc))
        rc = sessionmgrCreate(&g_friendsSessionMgr, g_friendsSrv.session, 0x5);

    if (R_SUCCEEDED(rc)) {
        rc = _friendsObjectDispatch(&g_friendsSrv, 0,
            .out_num_objects = 1,
            .out_objects = &g_friendsIFriendService
        );
    }

    return rc;
}

void _friendsCleanup(void) {
    sessionmgrClose(&g_friendsSessionMgr);
    _friendsObjectClose(&g_friendsIFriendService);
    serviceClose(&g_friendsSrv);
}

Service* friendsGetServiceSession(void) {
    return &g_friendsSrv;
}

Service* friendsGetServiceSession_IFriendsService(void) {
    return &g_friendsIFriendService;
}

Result friendsGetUserSetting(AccountUid uid, FriendsUserSetting *user_setting) {
    return _friendsObjectDispatchIn(&g_friendsIFriendService, 20800, uid,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out | SfBufferAttr_FixedSize },
        .buffers = { { user_setting, sizeof(FriendsUserSetting) } }
    );
}

Result friendsTryPopFriendInvitationNotificationInfo(AccountUid *uid, void* buffer, u64 size, u64 *out_size) {
    Result rc=0;
    AppletStorage storage;
    s64 storage_size=0;
    u64 data_size = size;
    AccountUid tmpuid={0};

    rc = appletTryPopFromFriendInvitationStorageChannel(&storage);
    if (R_SUCCEEDED(rc)) rc = appletStorageGetSize(&storage, &storage_size);
    if (R_SUCCEEDED(rc) && storage_size < sizeof(AccountUid)) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (R_SUCCEEDED(rc)) {
        storage_size-=sizeof(AccountUid);
        rc = appletStorageRead(&storage, 0, &tmpuid, sizeof(AccountUid));
        if (R_SUCCEEDED(rc)) {
            if (data_size > storage_size) data_size = storage_size;
            if (data_size) rc = appletStorageRead(&storage, sizeof(AccountUid), buffer, data_size);
            if (R_SUCCEEDED(rc)) {
                if (out_size) *out_size = data_size;
                if (uid) *uid = tmpuid;
            }
        }
    }

    appletStorageClose(&storage);
    return rc;
}
