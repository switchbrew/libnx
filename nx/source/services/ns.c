#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ns.h"
#include "services/fs.h"
#include "services/ncm.h"
#include "services/async.h"
#include "kernel/tmem.h"

static Service g_nsAppManSrv, g_nsGetterSrv;
static Service g_nsvmSrv;
static Service g_nsdevSrv;
static Service g_nssuSrv;

static Result _nsGetSession(Service* srv, Service* srv_out, u32 cmd_id);

// ns

NX_GENERATE_SERVICE_GUARD(ns);

Result _nsInitialize(void)
{
    Result rc=0;

    if(hosversionBefore(3,0,0))
        return smGetService(&g_nsAppManSrv, "ns:am");

    rc = smGetService(&g_nsGetterSrv, "ns:am2");//TODO: Support the other services?(Only useful when ns:am2 isn't accessible)
    if (R_FAILED(rc)) return rc;

    rc = _nsGetSession(&g_nsGetterSrv, &g_nsAppManSrv, 7996);

    if (R_FAILED(rc)) serviceClose(&g_nsGetterSrv);

    return rc;
}

void _nsCleanup(void)
{
    serviceClose(&g_nsAppManSrv);
    if(hosversionBefore(3,0,0)) return;

    serviceClose(&g_nsGetterSrv);
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

static Result _nsCmdInBool(Service* srv, bool inval, u32 cmd_id) {
    u8 in = inval!=0;

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _nsCmdInU64(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
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
    if (R_SUCCEEDED(rc) && out) *out = tmpout!=0;
    return rc;
}

static Result _nsCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nsCmdNoInOutSystemUpdateProgress(Service* srv, NsSystemUpdateProgress *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nsCmdRequestSendReceiveSystemUpdate(Service* srv, AsyncResult *a, u32 inval0, u16 inval1, NsSystemDeliveryInfo *info, u32 cmd_id) {
    const struct {
        u16 inval0;
        u32 inval1;
    } in = { inval0, inval1 };

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

Result nsListApplicationRecord(NsApplicationRecord* records, s32 count, s32 entry_offset, s32* out_entrycount) {
    return serviceDispatchInOut(&g_nsAppManSrv, 0, entry_offset, *out_entrycount,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { records, count*sizeof(NsApplicationRecord) } },
    );
}

Result nsListApplicationContentMetaStatus(u64 titleID, s32 index, NsApplicationContentMetaStatus* list, s32 count, s32* out_entrycount) {
    const struct {
        s32 index;
        u64 titleID;
    } in = { index, titleID };

    return serviceDispatchInOut(&g_nsAppManSrv, 601, in, *out_entrycount,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { list, count*sizeof(NsApplicationContentMetaStatus) } },
    );
}

Result nsGetApplicationControlData(NsApplicationControlSource source, u64 titleID, NsApplicationControlData* buffer, size_t size, u64* actual_size) {
    const struct {
        u8 source;
        u64 titleID;
    } in = { source, titleID };

    u32 tmp=0;

    Result rc = serviceDispatchInOut(&g_nsAppManSrv, 400, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc) && actual_size) *actual_size = tmp;
    return rc;
}

Result nsGetTotalSpaceSize(FsStorageId storage_id, u64 *size) {
    return _nsCmdInU64OutU64(&g_nsAppManSrv, storage_id, size, 47);
}

Result nsGetFreeSpaceSize(FsStorageId storage_id, u64 *size) {
    return _nsCmdInU64OutU64(&g_nsAppManSrv, storage_id, size, 48);
}

// ns:vm

NX_GENERATE_SERVICE_GUARD(nsvm);

Result _nsvmInitialize(void)
{
    if (hosversionBefore(3,0,0))
        return 0;

    return smGetService(&g_nsvmSrv, "ns:vm");
}

void _nsvmCleanup(void)
{
    if (hosversionBefore(3,0,0))
        return;

    serviceClose(&g_nsvmSrv);
}

Result nsvmNeedsUpdateVulnerability(bool *out) {
    Service *srv = &g_nsAppManSrv;
    if (hosversionAtLeast(3,0,0)) srv = &g_nsvmSrv;

    return _nsCmdNoInOutBool(srv, out, 1200);
}

Result nsvmGetSafeSystemVersion(NcmContentMetaKey *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_nsvmSrv, 1202, out);
}

// ns:dev

NX_GENERATE_SERVICE_GUARD(nsdev);

Result _nsdevInitialize(void) {
    return smGetService(&g_nsdevSrv, "ns:dev");
}

void _nsdevCleanup(void) {
    serviceClose(&g_nsdevSrv);
}

Result nsdevLaunchProgram(u64* out_pid, const NsLaunchProperties* properties, u32 flags) {
    const struct {
        u32 flags;
        NsLaunchProperties properties;
    } in = { flags, *properties};

    return serviceDispatchInOut(&g_nsdevSrv, 0, in, *out_pid);
}

Result nsdevTerminateProcess(u64 pid) {
    return _nsCmdInU64(&g_nsdevSrv, pid, 1);
}

Result nsdevTerminateProgram(u64 tid) {
    return _nsCmdInU64(&g_nsdevSrv, tid, 2);
}

Result nsdevGetShellEvent(Event* out_event) {
    return _nsCmdGetEvent(&g_nsdevSrv, out_event, true, 4);
}

Result nsdevGetShellEventInfo(NsShellEventInfo* out) {
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
    return serviceDispatchOut(&g_nsdevSrv, 7, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { path, path_len } },
    );
}

Result nsdevLaunchApplicationForDevelop(u64* out_pid, u64 app_title_id, u32 flags) {
    const struct {
        u32 flags;
        u64 app_title_id;
    } in = { .flags = flags, .app_title_id = app_title_id};

    return serviceDispatchInOut(&g_nsdevSrv, 8, in, *out_pid);
}

Result nsdevLaunchApplicationWithStorageIdForDevelop(u64* out_pid, u64 app_title_id, u32 flags, u8 app_storage_id, u8 patch_storage_id) {
    const struct {
        u8 app_storage_id;
        u8 patch_storage_id;
        u32 flags;
        u64 app_title_id;
    } in = { .app_storage_id = app_storage_id, .patch_storage_id = patch_storage_id, .flags = flags, .app_title_id = app_title_id};

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

Result nssuRequestSendSystemUpdate(AsyncResult *a, u32 inval0, u16 inval1, NsSystemDeliveryInfo *info) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdRequestSendReceiveSystemUpdate(&g_nssuSrv, a, inval0, inval1, info, 17);
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

Result nssuControlRequestReceiveSystemUpdate(NsSystemUpdateControl *c, AsyncResult *a, u32 inval0, u16 inval1, NsSystemDeliveryInfo *info) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdRequestSendReceiveSystemUpdate(&c->s, a, inval0, inval1, info, 16);
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

