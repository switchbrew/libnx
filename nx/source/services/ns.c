#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ns.h"

static Service g_nsAppManSrv, g_nsGetterSrv, g_nsvmSrv, g_nsdevSrv;

static Result _nsGetInterface(Service* srv_out, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(ns);

Result _nsInitialize(void)
{
    Result rc=0;

    if(hosversionBefore(3,0,0))
        return smGetService(&g_nsAppManSrv, "ns:am");

    rc = smGetService(&g_nsGetterSrv, "ns:am2");//TODO: Support the other services?(Only useful when ns:am2 isn't accessible)
    if (R_FAILED(rc)) return rc;

    rc = _nsGetInterface(&g_nsAppManSrv, 7996);

    if (R_FAILED(rc)) serviceClose(&g_nsGetterSrv);

    return rc;
}

void _nsCleanup(void)
{
    serviceClose(&g_nsAppManSrv);
    if(hosversionBefore(3,0,0)) return;

    serviceClose(&g_nsGetterSrv);
}

static Result _nsGetInterface(Service* srv_out, u32 cmd_id) {
    return serviceDispatch(&g_nsGetterSrv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _appletGetEvent(Service* srv, u32 cmd_id, Event* out_event, bool autoclear) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _nsCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _nsCmdInBool(Service* srv, u32 cmd_id, bool inval) {
    u8 in = inval!=0;

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _nsCmdInU64(Service* srv, u32 cmd_id, u64 inval) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _nsCmdInU64OutU64(Service* srv, u32 cmd_id, u64 inval, u64 *out) {
    return serviceDispatchInOut(srv, cmd_id, inval, *out);
}

static Result _nsCmdNoInOutBool(Service* srv, u32 cmd_id, bool *out) {
    u8 tmpout=0;
    Result rc = serviceDispatchOut(srv, cmd_id, tmpout);
    if (R_SUCCEEDED(rc) && out) *out = tmpout!=0;
    return rc;
}

static Result _nsCmdNoInOutU64(Service* srv, u32 cmd_id, u64 *out) {
    return serviceDispatchOut(srv, cmd_id, *out);
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

Result nsGetApplicationControlData(u8 flag, u64 titleID, NsApplicationControlData* buffer, size_t size, u64* actual_size) {
    const struct {
        u8 flag;
        u64 titleID;
    } in = { flag, titleID };

    return serviceDispatchInOut(&g_nsAppManSrv, 400, in, *actual_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result nsGetTotalSpaceSize(FsStorageId storage_id, u64 *size) {
    return _nsCmdInU64OutU64(&g_nsAppManSrv, 47, storage_id, size);
}

Result nsGetFreeSpaceSize(FsStorageId storage_id, u64 *size) {
    return _nsCmdInU64OutU64(&g_nsAppManSrv, 48, storage_id, size);
}

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

    return _nsCmdNoInOutBool(srv, 1200, out);
}

Result nsvmGetSafeSystemVersion(u16 *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_nsvmSrv, 1202, out);
}

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
    return _nsCmdInU64(&g_nsdevSrv, 1, pid);
}

Result nsdevTerminateProgram(u64 tid) {
    return _nsCmdInU64(&g_nsdevSrv, 2, tid);
}

Result nsdevGetShellEvent(Event* out_event) {
    return _appletGetEvent(&g_nsdevSrv, 4, out_event, true);
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

    return _nsCmdNoInOutBool(&g_nsdevSrv, 10, out);
}

Result nsdevGetRunningApplicationProcessIdForDevelop(u64* out_pid) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdNoInOutU64(&g_nsdevSrv, 11, out_pid);
}

Result nsdevSetCurrentApplicationRightsEnvironmentCanBeActiveForDevelop(bool can_be_active) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nsCmdInBool(&g_nsdevSrv, 12, can_be_active);
}
