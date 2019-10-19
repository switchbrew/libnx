#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ldr.h"

#define LDR_GENERATE_SERVICE_INIT(name, srvname)            \
static Service g_ldr##name##Srv;                            \
                                                            \
NX_GENERATE_SERVICE_GUARD(ldr##name);                       \
                                                            \
Result _ldr##name##Initialize(void) {                       \
    return smGetService(&g_ldr##name##Srv, "ldr:"#srvname); \
}                                                           \
                                                            \
void _ldr##name##Cleanup(void) {                            \
    serviceClose(&g_ldr##name##Srv);                        \
}                                                           \
                                                            \
Service* ldr##name##GetServiceSession(void) {               \
    return &g_ldr##name##Srv;                               \
}

LDR_GENERATE_SERVICE_INIT(Shell, shel);
LDR_GENERATE_SERVICE_INIT(Dmnt,  dmnt);
LDR_GENERATE_SERVICE_INIT(Pm,    pm);

NX_INLINE Result _ldrAddTitleToLaunchQueue(Service* srv, u64 tid, const void *args, size_t args_size) {
    return serviceDispatchIn(srv, 0, tid,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer },
        .buffers = { { args,  args_size } },
    );
}

NX_INLINE Result _ldrClearLaunchQueue(Service* srv) {
    return serviceDispatch(srv, 1);
}

Result ldrShellAddTitleToLaunchQueue(u64 tid, const void *args, size_t args_size) {
    return _ldrAddTitleToLaunchQueue(&g_ldrShellSrv, tid, args, args_size);
}

Result ldrShellClearLaunchQueue(void) {
    return _ldrClearLaunchQueue(&g_ldrShellSrv);
}

Result ldrDmntAddTitleToLaunchQueue(u64 tid, const void *args, size_t args_size) {
    return _ldrAddTitleToLaunchQueue(&g_ldrDmntSrv, tid, args, args_size);
}

Result ldrDmntClearLaunchQueue(void) {
    return _ldrClearLaunchQueue(&g_ldrDmntSrv);
}

Result ldrDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, u32 *num_out) {
    return serviceDispatchInOut(&g_ldrDmntSrv, 2, pid, *num_out,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcPointer },
        .buffers = { { out_module_infos, max_out_modules * sizeof(*out_module_infos) } },
    );
}

Result ldrPmCreateProcess(u64 flags, u64 launch_index, Handle reslimit_h, Handle *out_process_h) {
    const struct {
        u64 flags;
        u64 launch_index;
    } in = { flags, launch_index };
    return serviceDispatchIn(&g_ldrPmSrv, 0, in,
        .in_num_handles = 1,
        .in_handles = { reslimit_h },
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = out_process_h,
    );
}

Result ldrPmGetProgramInfo(u64 title_id, FsStorageId storage_id, LoaderProgramInfo *out_program_info) {
    const struct {
        u64 title_id;
        u64 storage_id;
    } in = { title_id, storage_id };
    return serviceDispatchIn(&g_ldrPmSrv, 1, in,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers = { { out_program_info, sizeof(*out_program_info) } },
    );
}

Result ldrPmRegisterTitle(u64 title_id, FsStorageId storage_id, u64 *out_index) {
    const struct {
        u64 title_id;
        u64 storage_id;
    } in = { title_id, storage_id };
    return serviceDispatchInOut(&g_ldrPmSrv, 2, in, *out_index);
}

Result ldrPmUnregisterTitle(u64 launch_index) {
    return serviceDispatchIn(&g_ldrPmSrv, 3, launch_index);
}
