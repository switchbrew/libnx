#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include "kernel/event.h"
#include "services/pdm.h"
#include "services/acc.h"
#include "runtime/hosversion.h"

static Service g_pdmqrySrv;

NX_GENERATE_SERVICE_GUARD(pdmqry);

Result _pdmqryInitialize(void) {
    return smGetService(&g_pdmqrySrv, "pdm:qry");
}

void _pdmqryCleanup(void) {
    serviceClose(&g_pdmqrySrv);
}

Service* pdmqryGetServiceSession(void) {
    return &g_pdmqrySrv;
}

static Result _pdmCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _pdmqryQueryEvent(u32 cmd_id, u32 entryindex, void* events, size_t entrysize, s32 count, s32 *total_out) {
    return serviceDispatchInOut(&g_pdmqrySrv, cmd_id, entryindex, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { events, count*entrysize } },
    );
}

Result pdmqryQueryApplicationEvent(u32 entryindex, PdmApplicationEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(0, entryindex, events, sizeof(PdmApplicationEvent), count, total_out);
}

Result pdmqryQueryPlayStatisticsByApplicationId(u64 titleID, PdmPlayStatistics *stats) {
    return serviceDispatchInOut(&g_pdmqrySrv, 4, titleID, *stats);
}

Result pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(u64 titleID, AccountUid *userID, PdmPlayStatistics *stats) {
    const struct {
        u64 titleID;
        AccountUid userID;
    } in = { titleID, *userID };

    return serviceDispatchInOut(&g_pdmqrySrv, 5, in, *stats);
}

Result pdmqryQueryLastPlayTime(PdmLastPlayTime *playtimes, const u64 *titleIDs, s32 count, s32 *total_out) {
    return serviceDispatchOut(&g_pdmqrySrv, 7, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { playtimes, count*sizeof(PdmLastPlayTime) },
            { titleIDs, count*sizeof(u64) },
        },
    );
}

Result pdmqryQueryPlayEvent(u32 entryindex, PdmPlayEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(8, entryindex, events, sizeof(PdmPlayEvent), count, total_out);
}

Result pdmqryGetAvailablePlayEventRange(u32 *total_entries, u32 *start_entryindex, u32 *end_entryindex) {
    struct {
        u32 total_entries;
        u32 start_entryindex;
        u32 end_entryindex;
    } out;

    Result rc = serviceDispatchOut(&g_pdmqrySrv, 9, out);
    if (R_SUCCEEDED(rc)) {
        if (total_entries) *total_entries = out.total_entries;
        if (start_entryindex) *start_entryindex = out.start_entryindex;
        if (end_entryindex) *end_entryindex = out.end_entryindex;
    }
    return rc;
}

Result pdmqryQueryAccountEvent(u32 entryindex, PdmAccountEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(10, entryindex, events, sizeof(PdmAccountEvent), count, total_out);
}

Result pdmqryQueryAccountPlayEvent(u32 entryindex, AccountUid *userID, PdmAccountPlayEvent *events, s32 count, s32 *total_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 entryindex;
        AccountUid userID;
    } in = { entryindex, *userID };

    return serviceDispatchInOut(&g_pdmqrySrv, 11, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { events, count*sizeof(PdmAccountPlayEvent) } },
    );
}

Result pdmqryGetAvailableAccountPlayEventRange(AccountUid *userID, u32 *total_entries, u32 *start_entryindex, u32 *end_entryindex) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 total_entries;
        u32 start_entryindex;
        u32 end_entryindex;
    } out;

    Result rc = serviceDispatchInOut(&g_pdmqrySrv, 12, *userID, out);
    if (R_SUCCEEDED(rc)) {
        if (total_entries) *total_entries = out.total_entries;
        if (start_entryindex) *start_entryindex = out.start_entryindex;
        if (end_entryindex) *end_entryindex = out.end_entryindex;
    }
    return rc;
}

Result pdmqryQueryRecentlyPlayedApplication(AccountUid *userID, u64 *titleIDs, size_t count, u32 *total_out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_pdmqrySrv, 14, *userID, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { titleIDs, count*sizeof(u64) } },
    );
}

Result pdmqryGetRecentlyPlayedApplicationUpdateEvent(Event* out_event) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _pdmCmdGetEvent(&g_pdmqrySrv, out_event, false, 15);
}

