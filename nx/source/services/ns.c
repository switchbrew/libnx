#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ns.h"
#include "services/async.h"
#include "services/nifm.h"

static Service g_nsAppManSrv, g_nsGetterSrv;
static Service g_nsvmSrv;
static Service g_nsdevSrv;
static Service g_nssuSrv;

static Result _nsGetSession(Service* srv, Service* srv_out, u32 cmd_id);

// ns

NX_GENERATE_SERVICE_GUARD(ns);

Result _nsInitialize(void) {
    Result rc=0;
    static const char* const servarray[6] = {"ns:ec", "ns:web", "ns:rid", "ns:rt", "ns:ro", "ns:am2"}; // This is the order used used by official sw, however the below loop uses this in reverse since ns:am2 is last in the list.

    if(hosversionBefore(3,0,0))
        return smGetService(&g_nsAppManSrv, "ns:am");

    for (s32 i=5; i>=0; i--) {
        if (i==4 && hosversionBefore(11,0,0)) continue;
        rc = smGetService(&g_nsGetterSrv, servarray[i]);
        if (R_SUCCEEDED(rc)) break;
    }

    return rc;
}

void _nsCleanup(void) {
    if(hosversionBefore(3,0,0))
        serviceClose(&g_nsAppManSrv);
    else
        serviceClose(&g_nsGetterSrv);
}

Service* nsGetServiceSession_GetterInterface(void) {
    return &g_nsGetterSrv;
}

Service* nsGetServiceSession_ApplicationManagerInterface(void) {
    return &g_nsAppManSrv;
}

Result nsGetDynamicRightsInterface(Service* srv_out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7988);
}

Result nsGetReadOnlyApplicationControlDataInterface(Service* srv_out) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7989);
}

Result nsGetReadOnlyApplicationRecordInterface(Service* srv_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7991);
}

Result nsGetECommerceInterface(Service* srv_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7992);
}

Result nsGetApplicationVersionInterface(Service* srv_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7993);
}

Result nsGetFactoryResetInterface(Service* srv_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7994);
}

Result nsGetAccountProxyInterface(Service* srv_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7995);
}

Result nsGetApplicationManagerInterface(Service* srv_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7996);
}

Result nsGetDownloadTaskInterface(Service* srv_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7997);
}

Result nsGetContentManagementInterface(Service* srv_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7998);
}

Result nsGetDocumentInterface(Service* srv_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsGetSession(&g_nsGetterSrv, srv_out, 7999);
}

static Result _nsGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _nsCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _nsManCmdGetEvent(Event* out_event, bool autoclear, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdGetEvent(srv_ptr, out_event, autoclear, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInHandle64NoOut(Service* srv, Handle handle, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval,
        .in_num_handles = 1,
        .in_handles = { handle },
    );
}

static Result _nsCmdInTmemNoOut(Service* srv, TransferMemory *tmem, u32 cmd_id) {
    return _nsCmdInHandle64NoOut(srv, tmem->handle, tmem->size, cmd_id);
}

static Result _nsCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _nsManCmdNoIO(u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInBool(Service* srv, bool inval, u32 cmd_id) {
    u8 in = inval!=0;

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _nsCmdInU64(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _nsManCmdInU64(u64 inval, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU64(srv_ptr, inval, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInU64OutU64(Service* srv, u64 inval, u64 *out, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, inval, *out);
}

static Result _nsCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nsCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmpout=0;
    Result rc = _nsCmdNoInOutU8(srv, &tmpout, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmpout & 1;
    return rc;
}

static Result _nsManCmdNoInOutBool(bool *out, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoInOutBool(srv_ptr, out, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nsCmdInU8U64NoOut(Service* srv, u8 in8, u64 in64, u32 cmd_id) {
    const struct {
        u8 in8;
        u8 pad[7];
        u64 in64;
    } in = { in8, {0}, in64 };

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _nsManCmdInU8U64NoOut(u8 in8, u64 in64, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU8U64NoOut(srv_ptr, in8, in64, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInU64OutStorageIdS64(Service* srv, u64 inval, NcmStorageId *storage_id, s64 *outval, u32 cmd_id) {
    struct {
        u8 storage_id;
        u8 pad[7];
        s64 outval;
    } out;

    Result rc = serviceDispatchInOut(srv, cmd_id, inval, out);
    if (R_SUCCEEDED(rc)) {
        if (storage_id) *storage_id = out.storage_id;
        if (outval) *outval = out.outval;
    }
    return rc;
}

static Result _nsManCmdInU64OutStorageIdS64(u64 inval, NcmStorageId *storage_id, s64 *outval, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU64OutStorageIdS64(srv_ptr, inval, storage_id, outval, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInUidNoOut(Service* srv, AccountUid uid, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, uid);
}

static Result _nsManCmdInUidNoOut(AccountUid uid, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInUidNoOut(srv_ptr, uid, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdNoInOutSystemUpdateProgress(Service* srv, NsSystemUpdateProgress *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nsCmdRequestSendReceiveSystemUpdate(Service* srv, AsyncResult *a, u32 addr, u16 port, NsSystemDeliveryInfo *info, u32 cmd_id) {
    const struct {
        u16 port;
        u32 addr;
    } in = { port, addr };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(srv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { info, sizeof(*info) } },
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

static Result _nsCmdNoInOutAsyncValue(Service* srv, AsyncValue *a, u32 cmd_id) {
    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

static Result _nsCmdNoInOutAsyncResult(Service* srv, AsyncResult *a, u32 cmd_id) {
    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

static Result _nsCmdInU64OutAsyncValue(Service* srv, AsyncValue *a, u64 inval, u32 cmd_id) {
    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(srv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

static Result _nsManCmdInU64OutAsyncValue(AsyncValue *a, u64 inval, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU64OutAsyncValue(srv_ptr, a, inval, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInU64OutAsyncResult(Service* srv, AsyncResult *a, u64 inval, u32 cmd_id) {
    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(srv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

static Result _nsManCmdInU64OutAsyncResult(AsyncResult *a, u64 inval, u32 cmd_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU64OutAsyncResult(srv_ptr, a, inval, cmd_id);

    serviceClose(&srv);
    return rc;
}

static Result _nsCmdInUidOutAsyncResult(Service* srv, AsyncResult *a, AccountUid uid, u32 cmd_id) {
    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(srv, cmd_id, uid,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

static Result _nsCheckNifm(void) {
    return nifmIsAnyInternetRequestAccepted(nifmGetClientId()) ? 0 : MAKERESULT(16, 340);
}

// IReadOnlyApplicationControlDataInterface

Result nsGetApplicationControlData(NsApplicationControlSource source, u64 application_id, NsApplicationControlData* buffer, size_t size, u64* actual_size) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    u32 cmd_id = 400;
    if (hosversionAtLeast(5,1,0)) {
        rc = nsGetReadOnlyApplicationControlDataInterface(&srv);
        cmd_id = 0;
    } else if (hosversionAtLeast(3,0,0)) {
        rc = nsGetApplicationManagerInterface(&srv);
    }
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        u8 source;
        u8 pad[7];
        u64 application_id;
    } in = { source, {0}, application_id };

    u32 tmp=0;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc) && actual_size) *actual_size = tmp;

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationControlData2(NsApplicationControlSource source, u64 application_id, NsApplicationControlData* buffer, size_t size, u8 flag1, u8 flag2, u64* actual_size, u32* unk) {
    if (hosversionBefore(19,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    u32 cmd_id = 6;
    rc = nsGetReadOnlyApplicationControlDataInterface(&srv);

    const struct {
        u8 source;
        u8 flags[2];
        u8 pad[5];
        u64 application_id;
    } in = { source, {flag1, flag2}, {0}, application_id };

    u64 tmp=0;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (actual_size) *actual_size = tmp >> 32;
        if (unk) *unk = (u32)tmp;
    }

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationDesiredLanguage(NacpStruct *nacp, NacpLanguageEntry **langentry) {
    if (nacp==NULL || langentry==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    *langentry = NULL;

    NacpLanguageEntry *entry = NULL;
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    u32 cmd_id = 55;
    if (hosversionAtLeast(5,1,0)) {
        rc = nsGetReadOnlyApplicationControlDataInterface(&srv);
        cmd_id = 1;
    } else if (hosversionAtLeast(3,0,0)) {
        rc = nsGetApplicationManagerInterface(&srv);
    }
    else
        srv_ptr = &g_nsAppManSrv;

    u8 lang_bitmask=0, out=0;

    if (R_SUCCEEDED(rc)) {
        for (u32 i=0; i<16; i++) {
            entry = &nacp->lang[i];
            if (entry->name[0] || entry->author[0]) lang_bitmask |= BIT(i);
        }
        if (!lang_bitmask) {
            *langentry = &nacp->lang[0];
            return 0;
        }
    }

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, cmd_id, lang_bitmask, out);
    if (R_SUCCEEDED(rc)) {
        if (out > 16) out = 0;
        if (lang_bitmask & BIT(out))
            *langentry = &nacp->lang[out];
        else
            rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    }

    serviceClose(&srv);
    return rc;
}

// IECommerceInterface

Result nsRequestLinkDevice(AsyncResult *a, AccountUid uid) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    Service srv={0};
    rc = nsGetECommerceInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdInUidOutAsyncResult(&srv, a, uid, 0);

    serviceClose(&srv);
    return rc;
}

Result nsRequestSyncRights(AsyncResult *a) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetECommerceInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoInOutAsyncResult(&srv, a, 3);

    serviceClose(&srv);
    return rc;
}

Result nsRequestUnlinkDevice(AsyncResult *a, AccountUid uid) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    Service srv={0};
    rc = nsGetECommerceInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdInUidOutAsyncResult(&srv, a, uid, 4);

    serviceClose(&srv);
    return rc;
}

// IFactoryResetInterface

Result nsResetToFactorySettings(void) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetFactoryResetInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, 100);

    serviceClose(&srv);
    return rc;
}

Result nsResetToFactorySettingsWithoutUserSaveData(void) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetFactoryResetInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, 101);

    serviceClose(&srv);
    return rc;
}

Result nsResetToFactorySettingsForRefurbishment(void) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetFactoryResetInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, 102);

    serviceClose(&srv);
    return rc;
}

Result nsResetToFactorySettingsWithPlatformRegion(void) {
    if (hosversionBefore(9,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetFactoryResetInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&srv, 103);

    serviceClose(&srv);
    return rc;
}

Result nsResetToFactorySettingsWithPlatformRegionAuthentication(void) {
    if (hosversionBefore(9,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetFactoryResetInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&srv, 104);

    serviceClose(&srv);
    return rc;
}

// IApplicationManagerInterface

Result nsListApplicationRecord(NsApplicationRecord* records, s32 count, s32 entry_offset, s32* out_entrycount) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 0, entry_offset, *out_entrycount,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { records, count*sizeof(NsApplicationRecord) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationRecordUpdateSystemEvent(Event* out_event) {
    return _nsManCmdGetEvent(out_event, true, 2);
}

Result nsGetApplicationViewDeprecated(NsApplicationViewDeprecated *views, const u64 *application_ids, s32 count) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = serviceDispatch(srv_ptr, 3,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { views, count*sizeof(NsApplicationViewDeprecated) },
            { application_ids, count*sizeof(u64) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsDeleteApplicationEntity(u64 application_id) {
    return _nsManCmdInU64(application_id, 4);
}

Result nsDeleteApplicationCompletely(u64 application_id) {
    return _nsManCmdInU64(application_id, 5);
}

Result nsDeleteRedundantApplicationEntity(void) {
    return _nsManCmdNoIO(7);
}

Result nsIsApplicationEntityMovable(u64 application_id, NcmStorageId storage_id, bool *out) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        u8 storage_id;
        u8 pad[7];
        u64 application_id;
    } in = { storage_id, {0}, application_id };

    u8 tmp=0;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 8, in, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;

    serviceClose(&srv);
    return rc;
}

Result nsMoveApplicationEntity(u64 application_id, NcmStorageId storage_id) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU8U64NoOut(storage_id, application_id, 9);
}

Result nsRequestApplicationUpdateInfo(AsyncValue *a, u64 application_id) {
    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncValue(a, application_id, 30);
}

Result nsCancelApplicationDownload(u64 application_id) {
    return _nsManCmdInU64(application_id, 32);
}

Result nsResumeApplicationDownload(u64 application_id) {
    return _nsManCmdInU64(application_id, 33);
}

Result nsCheckApplicationLaunchVersion(u64 application_id) {
    return _nsManCmdInU64(application_id, 38);
}

Result nsCalculateApplicationDownloadRequiredSize(u64 application_id, NcmStorageId *storage_id, s64 *size) {
    return _nsManCmdInU64OutStorageIdS64(application_id, storage_id, size, 41);
}

Result nsCleanupSdCard(void) {
    return _nsManCmdNoIO(42);
}

Result nsGetSdCardMountStatusChangedEvent(Event* out_event) {
    return _nsManCmdGetEvent(out_event, false, 44);
}

Result nsGetGameCardUpdateDetectionEvent(Event* out_event) {
    return _nsManCmdGetEvent(out_event, false, 52);
}

Result nsDisableApplicationAutoDelete(u64 application_id) {
    return _nsManCmdInU64(application_id, 53);
}

Result nsEnableApplicationAutoDelete(u64 application_id) {
    return _nsManCmdInU64(application_id, 54);
}

Result nsSetApplicationTerminateResult(u64 application_id, Result res) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        Result res;
        u32 pad;
        u64 application_id;
    } in = { res, 0, application_id };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(srv_ptr, 56, in);

    serviceClose(&srv);
    return rc;
}

Result nsClearApplicationTerminateResult(u64 application_id) {
    return _nsManCmdInU64(application_id, 57);
}

Result nsGetLastSdCardMountUnexpectedResult(void) {
    return _nsManCmdNoIO(58);
}

Result nsGetRequestServerStopper(NsRequestServerStopper *r) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsGetSession(srv_ptr, &r->s, 65);

    serviceClose(&srv);
    return rc;
}

Result nsCancelApplicationApplyDelta(u64 application_id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU64(application_id, 67);
}

Result nsResumeApplicationApplyDelta(u64 application_id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU64(application_id, 68);
}

Result nsCalculateApplicationApplyDeltaRequiredSize(u64 application_id, NcmStorageId *storage_id, s64 *size) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU64OutStorageIdS64(application_id, storage_id, size, 69);
}

Result nsResumeAll(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoIO(70);
}

Result nsGetStorageSize(NcmStorageId storage_id, s64 *total_space_size, s64 *free_space_size) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    struct {
        s64 total_space_size;
        s64 free_space_size;
    } out;

    u8 tmp = storage_id;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 71, tmp, out);
    if (R_SUCCEEDED(rc)) {
        if (total_space_size) *total_space_size = out.total_space_size;
        if (free_space_size) *free_space_size = out.free_space_size;
    }

    serviceClose(&srv);
    return rc;
}

Result nsRequestUpdateApplication2(AsyncResult *a, u64 application_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncResult(a, application_id, 85);
}

Result nsDeleteUserSaveDataAll(NsProgressMonitorForDeleteUserSaveDataAll *p, AccountUid uid) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(srv_ptr, 201, uid,
        .out_num_objects = 1,
        .out_objects = &p->s,
    );

    serviceClose(&srv);
    return rc;
}

Result nsDeleteUserSystemSaveData(AccountUid uid, u64 system_save_data_id) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        AccountUid uid;
        u64 system_save_data_id;
    } in = { uid, system_save_data_id };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(srv_ptr, 210, in);

    serviceClose(&srv);
    return rc;
}

Result nsDeleteSaveData(FsSaveDataSpaceId save_data_space_id, u64 save_data_id) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU8U64NoOut(save_data_space_id, save_data_id, 211);
}

Result nsUnregisterNetworkServiceAccount(AccountUid uid) {
    return _nsManCmdInUidNoOut(uid, 220);
}

Result nsUnregisterNetworkServiceAccountWithUserSaveDataDeletion(AccountUid uid) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInUidNoOut(uid, 221);
}

Result nsRequestDownloadApplicationControlData(AsyncResult *a, u64 application_id) {
    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncResult(a, application_id, 402);
}

static Result _nsListApplicationTitleIcon(AsyncValue *a, NsApplicationControlSource source, const u64 *application_ids, s32 count, TransferMemory *tmem, u32 cmd_id) { // [8.0.0+]
    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u8 source;
        u8 pad[7];
        u64 size;
    } in = { source, {0}, tmem->size };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { application_ids, count*sizeof(u64) } },
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsListApplicationTitle(AsyncValue *a, NsApplicationControlSource source, const u64 *application_ids, s32 count, void* buffer, size_t size) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    TransferMemory tmem={0};

    rc = tmemCreateFromMemory(&tmem, buffer, size, Perm_R);
    if (R_SUCCEEDED(rc)) rc = _nsListApplicationTitleIcon(a, source, application_ids, count, &tmem, 407);
    tmemClose(&tmem);

    return rc;
}

Result nsListApplicationIcon(AsyncValue *a, NsApplicationControlSource source, const u64 *application_ids, s32 count, void* buffer, size_t size) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    TransferMemory tmem={0};

    rc = tmemCreateFromMemory(&tmem, buffer, size, Perm_R);
    if (R_SUCCEEDED(rc)) rc = _nsListApplicationTitleIcon(a, source, application_ids, count, &tmem, 408);
    tmemClose(&tmem);

    return rc;
}

Result nsRequestCheckGameCardRegistration(AsyncResult *a, u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncResult(a, application_id, 502);
}

Result nsRequestGameCardRegistrationGoldPoint(AsyncValue *a, AccountUid uid, u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    Service srv={0}, *srv_ptr = &srv;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        AccountUid uid;
        u64 application_id;
    } in = { uid, application_id };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(srv_ptr, 503, in,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsRequestRegisterGameCard(AsyncResult *a, AccountUid uid, u64 application_id, s32 inval) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    Service srv={0}, *srv_ptr = &srv;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        s32 inval;
        u32 pad;
        AccountUid uid;
        u64 application_id;
    } in = { inval, 0, uid, application_id };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(srv_ptr, 504, in,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsGetGameCardMountFailureEvent(Event* out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdGetEvent(out_event, false, 505);
}

Result nsIsGameCardInserted(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoInOutBool(out, 506);
}

Result nsEnsureGameCardAccess(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoIO(507);
}

Result nsGetLastGameCardMountFailureResult(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoIO(508);
}

Result nsListApplicationIdOnGameCard(u64 *application_ids, s32 count, s32 *total_out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 509, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { application_ids, count*sizeof(u64) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsTouchApplication(u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU64(application_id, 904);
}

Result nsIsApplicationUpdateRequested(u64 application_id, bool *flag, u32 *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    struct {
        u8 flag;
        u8 pad[3];
        u32 out;
    } tmpout;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 906, application_id, tmpout);
    if (R_SUCCEEDED(rc)) {
        if (flag) *flag = tmpout.flag & 1;
        if (out) *out = tmpout.out;
    }

    serviceClose(&srv);
    return rc;
}

Result nsWithdrawApplicationUpdateRequest(u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU64(application_id, 907);
}

static Result _nsRequestVerifyApplicationDeprecated(NsProgressAsyncResult *a, u64 application_id, TransferMemory *tmem) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        u64 application_id;
        u64 size;
    } in = { application_id, tmem->size };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(srv_ptr, 1000, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

static Result _nsRequestVerifyApplication(NsProgressAsyncResult *a, u64 application_id, u32 unk, TransferMemory *tmem) { // [5.0.0+]
    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u32 unk;
        u32 pad;
        u64 application_id;
        u64 size;
    } in = { unk, 0, application_id, tmem->size };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, 1003, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsRequestVerifyAddOnContentsRights(NsProgressAsyncResult *a, u64 application_id) {
    if (!hosversionBetween(3,10))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, 1002, application_id,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsRequestVerifyApplication(NsProgressAsyncResult *a, u64 application_id, u32 unk, void* buffer, size_t size) {
    Result rc=0;
    TransferMemory tmem={0};

    rc = tmemCreateFromMemory(&tmem, buffer, size, Perm_None);
    if (R_SUCCEEDED(rc)) {
        if (hosversionBefore(5,0,0))
            rc = _nsRequestVerifyApplicationDeprecated(a, application_id, &tmem);
        else
            rc = _nsRequestVerifyApplication(a, application_id, unk, &tmem);
    }
    tmemClose(&tmem);

    return rc;
}

Result nsIsAnyApplicationEntityInstalled(u64 application_id, bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetApplicationManagerInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    u8 tmp=0;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 1300, application_id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;

    serviceClose(&srv);
    return rc;
}

Result nsCleanupUnavailableAddOnContents(u64 application_id, AccountUid uid) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u64 application_id;
        AccountUid uid;
    } in = { application_id, uid };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, 1309, in);

    serviceClose(&srv);
    return rc;
}

Result nsEstimateSizeToMove(u8 *storage_ids, s32 count, NcmStorageId storage_id, u32 flags, u64 application_id, s64 *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u8 storage_id;
        u8 pad[3];
        u32 flags;
        u64 application_id;
    } in = { storage_id, {0}, flags, application_id };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 1311, in, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { storage_ids, count*sizeof(u8) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsFormatSdCard(void) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoIO(1500);
}

Result nsNeedsSystemUpdateToFormatSdCard(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoInOutBool(out, 1501);
}

Result nsGetLastSdCardFormatUnexpectedResult(void) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdNoIO(1502);
}

Result nsGetApplicationView(NsApplicationView *views, const u64 *application_ids, s32 count) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatch(&srv, 1701,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { views, count*sizeof(NsApplicationView) },
            { application_ids, count*sizeof(u64) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationViewDownloadErrorContext(u64 application_id, ErrorContext *context) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, 1703, application_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { context, sizeof(*context) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationViewWithPromotionInfo(NsApplicationViewWithPromotionInfo *out, const u64 *application_ids, s32 count) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatch(&srv, 1704,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { out, count*sizeof(NsApplicationViewWithPromotionInfo) },
            { application_ids, count*sizeof(u64) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsRequestDownloadApplicationPrepurchasedRights(AsyncResult *a, u64 application_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncResult(a, application_id, 1901);
}

Result nsGetSystemDeliveryInfo(NsSystemDeliveryInfo *info) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatch(&srv, 2000,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { info, sizeof(*info) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsSelectLatestSystemDeliveryInfo(const NsSystemDeliveryInfo *sys_list, s32 sys_count, const NsSystemDeliveryInfo *base_info, const NsApplicationDeliveryInfo *app_list, s32 app_count, s32 *index) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2001, *index,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { base_info, sizeof(*base_info) },
            { sys_list, sys_count*sizeof(NsSystemDeliveryInfo) },
            { app_list, app_count*sizeof(NsApplicationDeliveryInfo) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsVerifyDeliveryProtocolVersion(const NsSystemDeliveryInfo *info) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatch(&srv, 2002,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { info, sizeof(*info) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationDeliveryInfo(NsApplicationDeliveryInfo *info, s32 count, u64 application_id, u32 attr, s32 *total_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u32 attr;
        u32 pad;
        u64 application_id;
    } in = { attr, 0, application_id };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2003, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { info, count*sizeof(NsApplicationDeliveryInfo) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsHasAllContentsToDeliver(const NsApplicationDeliveryInfo *info, s32 count, bool *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    u8 tmp=0;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2004, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { info, count*sizeof(NsApplicationDeliveryInfo) } },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;

    serviceClose(&srv);
    return rc;
}

Result nsCompareApplicationDeliveryInfo(const NsApplicationDeliveryInfo *info0, s32 count0, const NsApplicationDeliveryInfo *info1, s32 count1, s32 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2005, *out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { info0, count0*sizeof(NsApplicationDeliveryInfo) },
            { info1, count1*sizeof(NsApplicationDeliveryInfo) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsCanDeliverApplication(const NsApplicationDeliveryInfo *info0, s32 count0, const NsApplicationDeliveryInfo *info1, s32 count1, bool *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    u8 tmp=0;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2006, tmp,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { info0, count0*sizeof(NsApplicationDeliveryInfo) },
            { info1, count1*sizeof(NsApplicationDeliveryInfo) },
        },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;

    serviceClose(&srv);
    return rc;
}

Result nsListContentMetaKeyToDeliverApplication(NcmContentMetaKey *meta, s32 meta_count, s32 meta_index, const NsApplicationDeliveryInfo *info, s32 info_count, s32 *total_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2007, meta_index, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { meta, meta_count*sizeof(NcmContentMetaKey) },
            { info, info_count*sizeof(NsApplicationDeliveryInfo) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsNeedsSystemUpdateToDeliverApplication(const NsApplicationDeliveryInfo *info, s32 count, const NsSystemDeliveryInfo *sys_info, bool *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    u8 tmp=0;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2008, tmp,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { sys_info, sizeof(*sys_info) },
            { info, count*sizeof(NsApplicationDeliveryInfo) },
        },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;

    serviceClose(&srv);
    return rc;
}

Result nsEstimateRequiredSize(const NcmContentMetaKey *meta, s32 count, s64 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2009, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { meta, count*sizeof(NcmContentMetaKey) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsRequestReceiveApplication(AsyncResult *a, u32 addr, u16 port, u64 application_id, const NcmContentMetaKey *meta, s32 count, NcmStorageId storage_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u8 storage_id;
        u8 pad;
        u16 port;
        u16 pad2;
        u32 addr;
        u64 application_id;
    } in = { storage_id, 0, port, 0, addr, application_id };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, 2010, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { meta, count*sizeof(NcmContentMetaKey) } },
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsCommitReceiveApplication(u64 application_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsManCmdInU64(application_id, 2011);
}

Result nsGetReceiveApplicationProgress(u64 application_id, NsReceiveApplicationProgress *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2012, application_id, *out);

    serviceClose(&srv);
    return rc;
}

Result nsRequestSendApplication(AsyncResult *a, u32 addr, u16 port, u64 application_id, const NcmContentMetaKey *meta, s32 count) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u16 port;
        u16 pad2;
        u32 addr;
        u64 application_id;
    } in = { port, 0, addr, application_id };

    memset(a, 0, sizeof(*a));
    Handle event = INVALID_HANDLE;
    if (R_SUCCEEDED(rc)) rc = serviceDispatchIn(&srv, 2013, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { meta, count*sizeof(NcmContentMetaKey) } },
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    serviceClose(&srv);
    return rc;
}

Result nsGetSendApplicationProgress(u64 application_id, NsSendApplicationProgress *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2014, application_id, *out);

    serviceClose(&srv);
    return rc;
}

Result nsCompareSystemDeliveryInfo(const NsSystemDeliveryInfo *info0, const NsSystemDeliveryInfo *info1, s32 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2015, *out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { info0, sizeof(*info0) },
            { info1, sizeof(*info1) },
        },
    );

    serviceClose(&srv);
    return rc;
}

Result nsListNotCommittedContentMeta(NcmContentMetaKey *meta, s32 count, u64 application_id, s32 unk, s32 *total_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        s32 unk;
        u32 pad;
        u64 application_id;
    } in = { unk, 0, application_id };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2016, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { meta, count*sizeof(NcmContentMetaKey) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationDeliveryInfoHash(const NsApplicationDeliveryInfo *info, s32 count, u8 *out_hash) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    u8 tmp[0x20];
    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(&srv, 2018, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { info, count*sizeof(NsApplicationDeliveryInfo) } },
    );
    if (R_SUCCEEDED(rc) && out_hash) memcpy(out_hash, tmp, sizeof(tmp));

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationTerminateResult(u64 application_id, Result *res) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2100, application_id, *res);

    serviceClose(&srv);
    return rc;
}

Result nsGetApplicationRightsOnClient(NsApplicationRightsOnClient *rights, s32 count, u64 application_id, AccountUid uid, u32 flags, s32 *total_out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    const struct {
        u32 flags;
        u32 pad;
        u64 application_id;
        AccountUid uid;
    } in = { flags, 0, application_id, uid };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(&srv, 2050, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { rights, count*sizeof(NsApplicationRightsOnClient) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsRequestNoDownloadRightsErrorResolution(AsyncValue *a, u64 application_id) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncValue(a, application_id, 2351);
}

Result nsRequestResolveNoDownloadRightsError(AsyncValue *a, u64 application_id) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc = _nsCheckNifm();
    if (R_FAILED(rc)) return rc;

    return _nsManCmdInU64OutAsyncValue(a, application_id, 2352);
}

Result nsGetPromotionInfo(NsPromotionInfo *promotion, u64 application_id, AccountUid uid) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetApplicationManagerInterface(&srv);

    // These are arrays, but official sw uses hard-coded value 1 for array-count.

    if (R_SUCCEEDED(rc)) rc = serviceDispatch(&srv, 2400,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { promotion, sizeof(NsPromotionInfo) },
            { &application_id, sizeof(u64) },
            { &uid, sizeof(AccountUid) },
        },
    );

    serviceClose(&srv);
    return rc;
}

// IDownloadTaskInterface

Result nsClearTaskStatusList(void) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetDownloadTaskInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, 701);

    serviceClose(&srv);
    return rc;
}

Result nsRequestDownloadTaskList(void) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetDownloadTaskInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, 702);

    serviceClose(&srv);
    return rc;
}

Result nsRequestEnsureDownloadTask(AsyncResult *a) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetDownloadTaskInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoInOutAsyncResult(srv_ptr, a, 703);

    serviceClose(&srv);
    return rc;
}

Result nsListDownloadTaskStatus(NsDownloadTaskStatus* tasks, s32 count, s32 *total_out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetDownloadTaskInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchOut(srv_ptr, 704, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { tasks, count*sizeof(NsDownloadTaskStatus) } },
    );


    serviceClose(&srv);
    return rc;
}

Result nsRequestDownloadTaskListData(AsyncValue *a) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetDownloadTaskInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoInOutAsyncValue(srv_ptr, a, 705);

    serviceClose(&srv);
    return rc;
}

Result nsTryCommitCurrentApplicationDownloadTask(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetDownloadTaskInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&srv, 706);

    serviceClose(&srv);
    return rc;
}

Result nsEnableAutoCommit(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetDownloadTaskInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&srv, 707);

    serviceClose(&srv);
    return rc;
}

Result nsDisableAutoCommit(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetDownloadTaskInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&srv, 708);

    serviceClose(&srv);
    return rc;
}

Result nsTriggerDynamicCommitEvent(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetDownloadTaskInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&srv, 709);

    serviceClose(&srv);
    return rc;
}

// IContentManagementInterface

Result nsCalculateApplicationOccupiedSize(u64 application_id, NsApplicationOccupiedSize *out) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetContentManagementInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 11, application_id, *out);

    serviceClose(&srv);
    return rc;
}

Result nsCheckSdCardMountStatus(void) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetContentManagementInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(srv_ptr, 43);

    serviceClose(&srv);
    return rc;
}

Result nsGetTotalSpaceSize(NcmStorageId storage_id, s64 *size) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetContentManagementInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU64OutU64(srv_ptr, storage_id, (u64*)size, 47);

    serviceClose(&srv);
    return rc;
}

Result nsGetFreeSpaceSize(NcmStorageId storage_id, s64 *size) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetContentManagementInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = _nsCmdInU64OutU64(srv_ptr, storage_id, (u64*)size, 48);

    serviceClose(&srv);
    return rc;
}

Result nsCountApplicationContentMeta(u64 application_id, s32 *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetContentManagementInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 600, application_id, *out);

    serviceClose(&srv);
    return rc;
}

Result nsListApplicationContentMetaStatus(u64 application_id, s32 index, NsApplicationContentMetaStatus* list, s32 count, s32* out_entrycount) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    if (hosversionAtLeast(3,0,0))
        rc = nsGetContentManagementInterface(&srv);
    else
        srv_ptr = &g_nsAppManSrv;

    const struct {
        s32 index;
        u32 pad;
        u64 application_id;
    } in = { index, 0, application_id };

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, 601, in, *out_entrycount,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { list, count*sizeof(NsApplicationContentMetaStatus) } },
    );

    serviceClose(&srv);
    return rc;
}

Result nsIsAnyApplicationRunning(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Service srv={0};
    Result rc = nsGetContentManagementInterface(&srv);

    if (R_SUCCEEDED(rc)) rc = _nsCmdNoInOutBool(&srv, out, 607);

    serviceClose(&srv);
    return rc;
}

// IRequestServerStopper

void nsRequestServerStopperClose(NsRequestServerStopper *r) {
    serviceClose(&r->s);
}

// IProgressMonitorForDeleteUserSaveDataAll

Result nsProgressMonitorForDeleteUserSaveDataAllClose(NsProgressMonitorForDeleteUserSaveDataAll *p) {
    Result rc=0;

    if (serviceIsActive(&p->s)) {
        bool finished=0;
        rc = nsProgressMonitorForDeleteUserSaveDataAllIsFinished(p, &finished);
        if (R_SUCCEEDED(rc) && !finished) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    }

    if (R_SUCCEEDED(rc)) serviceClose(&p->s);
    return rc;
}

Result nsProgressMonitorForDeleteUserSaveDataAllGetSystemEvent(NsProgressMonitorForDeleteUserSaveDataAll *p, Event* out_event) {
    if (!serviceIsActive(&p->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdGetEvent(&p->s, out_event, false, 0);
}

Result nsProgressMonitorForDeleteUserSaveDataAllIsFinished(NsProgressMonitorForDeleteUserSaveDataAll *p, bool *out) {
    if (!serviceIsActive(&p->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutBool(&p->s, out, 1);
}

Result nsProgressMonitorForDeleteUserSaveDataAllGetResult(NsProgressMonitorForDeleteUserSaveDataAll *p) {
    if (!serviceIsActive(&p->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoIO(&p->s, 2);
}

Result nsProgressMonitorForDeleteUserSaveDataAllGetProgress(NsProgressMonitorForDeleteUserSaveDataAll *p, NsProgressForDeleteUserSaveDataAll *progress) {
    if (!serviceIsActive(&p->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchOut(&p->s, 10, *progress);
}

// IProgressAsyncResult

void nsProgressAsyncResultClose(NsProgressAsyncResult *a) {
    if (serviceIsActive(&a->s)) {
        nsProgressAsyncResultCancel(a); // Official sw ignores rc from this prior to waiting on the event.
        nsProgressAsyncResultWait(a, UINT64_MAX);
    }

    serviceClose(&a->s);
    eventClose(&a->event);
}

Result nsProgressAsyncResultWait(NsProgressAsyncResult *a, u64 timeout) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return eventWait(&a->event, timeout);
}

Result nsProgressAsyncResultGet(NsProgressAsyncResult *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc = nsProgressAsyncResultWait(a, UINT64_MAX);
    if (R_SUCCEEDED(rc)) rc = _nsCmdNoIO(&a->s, 0);
    return rc;
}

Result nsProgressAsyncResultCancel(NsProgressAsyncResult *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoIO(&a->s, 1);
}

Result nsProgressAsyncResultGetProgress(NsProgressAsyncResult *a, void* buffer, size_t size) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatch(&a->s, 2,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result nsProgressAsyncResultGetDetailResult(NsProgressAsyncResult *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoIO(&a->s, 3);
}

Result nsProgressAsyncResultGetErrorContext(NsProgressAsyncResult *a, ErrorContext *context) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&a->s, 4,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { context, sizeof(*context) } },
    );
}

// ns:vm

NX_GENERATE_SERVICE_GUARD(nsvm);

Result _nsvmInitialize(void) {
    if (hosversionBefore(3,0,0))
        return 0;

    return smGetService(&g_nsvmSrv, "ns:vm");
}

void _nsvmCleanup(void) {
    if (hosversionBefore(3,0,0))
        return;

    serviceClose(&g_nsvmSrv);
}

Service* nsvmGetServiceSession(void) {
    return &g_nsvmSrv;
}

Result nsvmNeedsUpdateVulnerability(bool *out) {
    Service *srv = &g_nsAppManSrv;
    if (hosversionAtLeast(3,0,0)) srv = &g_nsvmSrv;

    return _nsCmdNoInOutBool(srv, out, 1200);
}

Result nsvmGetSafeSystemVersion(NcmContentMetaKey *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_nsvmSrv, 1202, *out);
}

// ns:dev

NX_GENERATE_SERVICE_GUARD(nsdev);

Result _nsdevInitialize(void) {
    return smGetService(&g_nsdevSrv, "ns:dev");
}

void _nsdevCleanup(void) {
    serviceClose(&g_nsdevSrv);
}

Service* nsdevGetServiceSession(void) {
    return &g_nsdevSrv;
}

Result nsdevLaunchProgram(u64* out_pid, const NsLaunchProperties* properties, u32 flags) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 flags;
        u32 pad;
        NsLaunchProperties properties;
    } in = { flags, 0, *properties};

    return serviceDispatchInOut(&g_nsdevSrv, 0, in, *out_pid);
}

Result nsdevTerminateProcess(u64 pid) {
    return _nsCmdInU64(&g_nsdevSrv, pid, 1);
}

Result nsdevTerminateProgram(u64 tid) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdInU64(&g_nsdevSrv, tid, 2);
}

Result nsdevGetShellEvent(Event* out_event) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdGetEvent(&g_nsdevSrv, out_event, true, 4);
}

Result nsdevGetShellEventInfo(NsShellEventInfo* out) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 event;
        u64 process_id;
    } tmpout;

    Result rc = serviceDispatchOut(&g_nsdevSrv, 5, tmpout);
    if (R_SUCCEEDED(rc) && out) {
        out->event = (NsShellEvent)tmpout.event;
        out->process_id = tmpout.process_id;
    }
    return rc;
}

Result nsdevTerminateApplication(void) {
    return _nsCmdNoIO(&g_nsdevSrv, 6);
}

Result nsdevPrepareLaunchProgramFromHost(NsLaunchProperties* out, const char* path, size_t path_len) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_nsdevSrv, 7, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { path, path_len } },
    );
}

Result nsdevLaunchApplicationForDevelop(u64* out_pid, u64 application_id, u32 flags) {
    if (hosversionAtLeast(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 flags;
        u64 application_id;
    } in = { .flags = flags, .application_id = application_id};

    return serviceDispatchInOut(&g_nsdevSrv, 8, in, *out_pid);
}

Result nsdevLaunchApplicationFromHost(u64* out_pid, const char* path, size_t path_len, u32 flags) {
    if (hosversionBefore(10,0,0) || hosversionAtLeast(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_nsdevSrv, 8, flags, *out_pid,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { path, path_len } },
    );
}

Result nsdevLaunchApplicationWithStorageIdForDevelop(u64* out_pid, u64 application_id, u32 flags, u8 app_storage_id, u8 patch_storage_id) {
    if (hosversionAtLeast(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 app_storage_id;
        u8 patch_storage_id;
        u32 flags;
        u64 application_id;
    } in = { .app_storage_id = app_storage_id, .patch_storage_id = patch_storage_id, .flags = flags, .application_id = application_id};

    return serviceDispatchInOut(&g_nsdevSrv, 9, in, *out_pid);
}

Result nsdevIsSystemMemoryResourceLimitBoosted(bool* out) {
    if (hosversionBefore(6,0,0) || hosversionAtLeast(9,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutBool(&g_nsdevSrv, out, 10);
}

Result nsdevGetRunningApplicationProcessIdForDevelop(u64* out_pid) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutU64(&g_nsdevSrv, out_pid, 11);
}

Result nsdevSetCurrentApplicationRightsEnvironmentCanBeActiveForDevelop(bool can_be_active) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdInBool(&g_nsdevSrv, can_be_active, 12);
}

// ns:su

NX_GENERATE_SERVICE_GUARD(nssu);

Result _nssuInitialize(void) {
    return smGetService(&g_nssuSrv, "ns:su");
}

void _nssuCleanup(void) {
    serviceClose(&g_nssuSrv);
}

Service* nssuGetServiceSession(void) {
    return &g_nssuSrv;
}

Result nssuGetBackgroundNetworkUpdateState(NsBackgroundNetworkUpdateState *out) {
    u8 tmpout=0;
    Result rc = _nsCmdNoInOutU8(&g_nssuSrv, &tmpout, 0);
    if (R_SUCCEEDED(rc)) *out = tmpout;
    return rc;
}

Result nssuOpenSystemUpdateControl(NsSystemUpdateControl *c) {
    return _nsGetSession(&g_nssuSrv, &c->s, 1);
}

Result nssuNotifyExFatDriverRequired(void) {
    return _nsCmdNoIO(&g_nssuSrv, 2);
}

Result nssuClearExFatDriverStatusForDebug(void) {
    return _nsCmdNoIO(&g_nssuSrv, 3);
}

Result nssuRequestBackgroundNetworkUpdate(void) {
    return _nsCmdNoIO(&g_nssuSrv, 4);
}

Result nssuNotifyBackgroundNetworkUpdate(const NcmContentMetaKey *key) {
    return serviceDispatchIn(&g_nssuSrv, 5, *key);
}

Result nssuNotifyExFatDriverDownloadedForDebug(void) {
    return _nsCmdNoIO(&g_nssuSrv, 6);
}

Result nssuGetSystemUpdateNotificationEventForContentDelivery(Event* out_event) {
    return _nsCmdGetEvent(&g_nssuSrv, out_event, false, 9);
}

Result nssuNotifySystemUpdateForContentDelivery(void) {
    return _nsCmdNoIO(&g_nssuSrv, 10);
}

Result nssuPrepareShutdown(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoIO(&g_nssuSrv, 11);
}

Result nssuDestroySystemUpdateTask(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoIO(&g_nssuSrv, 16);
}

Result nssuRequestSendSystemUpdate(AsyncResult *a, u32 addr, u16 port, NsSystemDeliveryInfo *info) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdRequestSendReceiveSystemUpdate(&g_nssuSrv, a, addr, port, info, 17);
}

Result nssuGetSendSystemUpdateProgress(NsSystemUpdateProgress *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutSystemUpdateProgress(&g_nssuSrv, out, 18);
}

// ISystemUpdateControl

void nssuControlClose(NsSystemUpdateControl *c) {
    serviceClose(&c->s);
    tmemClose(&c->tmem);
}

static Result _nssuControlGetEulaDataSize(NsSystemUpdateControl *c, u32 cmd_id, const char* path, u64 *filesize) {
    NsEulaDataPath datapath;

    memset(&datapath, 0, sizeof(datapath));
    strncpy(datapath.path, path, sizeof(datapath.path)-1);

    return serviceDispatchOut(&c->s, cmd_id, *filesize,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { &datapath, sizeof(datapath) } },
    );
}

static Result _nssuControlGetEulaData(NsSystemUpdateControl *c, u32 cmd_id, const char* path, void* buffer, size_t size, u64 *filesize) {
    NsEulaDataPath datapath;

    memset(&datapath, 0, sizeof(datapath));
    strncpy(datapath.path, path, sizeof(datapath.path)-1);

    return serviceDispatchOut(&c->s, cmd_id, *filesize,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { &datapath, sizeof(datapath) },
            { buffer, size },
        },
    );
}

static Result _nssuControlCmdTmemNoOut(NsSystemUpdateControl *c, void* buffer, size_t size, u32 cmd_id) {
    Result rc=0;

    if (buffer==NULL) rc = tmemCreate(&c->tmem, size, Perm_None);
    else rc = tmemCreateFromMemory(&c->tmem, buffer, size, Perm_None);
    if (R_FAILED(rc)) return rc;

    rc = _nsCmdInTmemNoOut(&c->s, &c->tmem, cmd_id);
    if (R_FAILED(rc)) tmemClose(&c->tmem);

    return rc;
}

Result nssuControlHasDownloaded(NsSystemUpdateControl *c, bool* out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutBool(&c->s, out, 0);
}

Result nssuControlRequestCheckLatestUpdate(NsSystemUpdateControl *c, AsyncValue *a) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutAsyncValue(&c->s, a, 1);
}

Result nssuControlRequestDownloadLatestUpdate(NsSystemUpdateControl *c, AsyncResult *a) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutAsyncResult(&c->s, a, 2);
}

Result nssuControlGetDownloadProgress(NsSystemUpdateControl *c, NsSystemUpdateProgress *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutSystemUpdateProgress(&c->s, out, 3);
}

Result nssuControlApplyDownloadedUpdate(NsSystemUpdateControl *c) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoIO(&c->s, 4);
}

Result nssuControlRequestPrepareCardUpdate(NsSystemUpdateControl *c, AsyncResult *a) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutAsyncResult(&c->s, a, 5);
}

Result nssuControlGetPrepareCardUpdateProgress(NsSystemUpdateControl *c, NsSystemUpdateProgress *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutSystemUpdateProgress(&c->s, out, 6);
}

Result nssuControlHasPreparedCardUpdate(NsSystemUpdateControl *c, bool* out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoInOutBool(&c->s, out, 7);
}

Result nssuControlApplyCardUpdate(NsSystemUpdateControl *c) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nsCmdNoIO(&c->s, 8);
}

Result nssuControlGetDownloadedEulaDataSize(NsSystemUpdateControl *c, const char* path, u64 *filesize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nssuControlGetEulaDataSize(c, 9, path, filesize);
}

Result nssuControlGetDownloadedEulaData(NsSystemUpdateControl *c, const char* path, void* buffer, size_t size, u64 *filesize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nssuControlGetEulaData(c, 10, path, buffer, size, filesize);
}

Result nssuControlSetupCardUpdate(NsSystemUpdateControl *c, void* buffer, size_t size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nssuControlCmdTmemNoOut(c, buffer, size, 11);
}

Result nssuControlGetPreparedCardUpdateEulaDataSize(NsSystemUpdateControl *c, const char* path, u64 *filesize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nssuControlGetEulaDataSize(c, 12, path, filesize);
}

Result nssuControlGetPreparedCardUpdateEulaData(NsSystemUpdateControl *c, const char* path, void* buffer, size_t size, u64 *filesize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nssuControlGetEulaData(c, 13, path, buffer, size, filesize);
}

Result nssuControlSetupCardUpdateViaSystemUpdater(NsSystemUpdateControl *c, void* buffer, size_t size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nssuControlCmdTmemNoOut(c, buffer, size, 14);
}

Result nssuControlHasReceived(NsSystemUpdateControl *c, bool* out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutBool(&c->s, out, 15);
}

Result nssuControlRequestReceiveSystemUpdate(NsSystemUpdateControl *c, AsyncResult *a, u32 addr, u16 port, NsSystemDeliveryInfo *info) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdRequestSendReceiveSystemUpdate(&c->s, a, addr, port, info, 16);
}

Result nssuControlGetReceiveProgress(NsSystemUpdateControl *c, NsSystemUpdateProgress *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutSystemUpdateProgress(&c->s, out, 17);
}

Result nssuControlApplyReceivedUpdate(NsSystemUpdateControl *c) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoIO(&c->s, 18);
}

Result nssuControlGetReceivedEulaDataSize(NsSystemUpdateControl *c, const char* path, u64 *filesize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nssuControlGetEulaDataSize(c, 19, path, filesize);
}

Result nssuControlGetReceivedEulaData(NsSystemUpdateControl *c, const char* path, void* buffer, size_t size, u64 *filesize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nssuControlGetEulaData(c, 20, path, buffer, size, filesize);
}

Result nssuControlSetupToReceiveSystemUpdate(NsSystemUpdateControl *c) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoIO(&c->s, 21);
}

Result nssuControlRequestCheckLatestUpdateIncludesRebootlessUpdate(NsSystemUpdateControl *c, AsyncValue *a) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutAsyncValue(&c->s, a, 22);
}

