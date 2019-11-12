#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include "services/pdm.h"
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

static Result _pdmqryQueryEvent(s32 entry_index, void* events, size_t entrysize, s32 count, s32 *total_out, u32 cmd_id) {
    return serviceDispatchInOut(&g_pdmqrySrv, cmd_id, entry_index, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { events, count*entrysize } },
    );
}

Result pdmqryQueryAppletEvent(s32 entry_index, PdmAppletEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(entry_index, events, sizeof(PdmAppletEvent), count, total_out, 0);
}

Result pdmqryQueryPlayStatisticsByApplicationId(u64 application_id, PdmPlayStatistics *stats) {
    return serviceDispatchInOut(&g_pdmqrySrv, 4, application_id, *stats);
}

Result pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(u64 application_id, AccountUid uid, PdmPlayStatistics *stats) {
    const struct {
        u64 application_id;
        AccountUid uid;
    } in = { application_id, uid };

    return serviceDispatchInOut(&g_pdmqrySrv, 5, in, *stats);
}

Result pdmqryQueryLastPlayTime(PdmLastPlayTime *playtimes, const u64 *application_ids, s32 count, s32 *total_out) {
    return serviceDispatchOut(&g_pdmqrySrv, 7, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { playtimes, count*sizeof(PdmLastPlayTime) },
            { application_ids, count*sizeof(u64) },
        },
    );
}

Result pdmqryQueryPlayEvent(s32 entry_index, PdmPlayEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(entry_index, events, sizeof(PdmPlayEvent), count, total_out, 8);
}

Result pdmqryGetAvailablePlayEventRange(s32 *total_entries, s32 *start_entry_index, s32 *end_entry_index) {
    struct {
        s32 total_entries;
        s32 start_entry_index;
        s32 end_entry_index;
    } out;

    Result rc = serviceDispatchOut(&g_pdmqrySrv, 9, out);
    if (R_SUCCEEDED(rc)) {
        if (total_entries) *total_entries = out.total_entries;
        if (start_entry_index) *start_entry_index = out.start_entry_index;
        if (end_entry_index) *end_entry_index = out.end_entry_index;
    }
    return rc;
}

Result pdmqryQueryAccountEvent(s32 entry_index, PdmAccountEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(entry_index, events, sizeof(PdmAccountEvent), count, total_out, 10);
}

Result pdmqryQueryAccountPlayEvent(s32 entry_index, AccountUid uid, PdmAccountPlayEvent *events, s32 count, s32 *total_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 entry_index;
        u32 pad;
        AccountUid uid;
    } in = { entry_index, 0, uid };

    return serviceDispatchInOut(&g_pdmqrySrv, 11, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { events, count*sizeof(PdmAccountPlayEvent) } },
    );
}

Result pdmqryGetAvailableAccountPlayEventRange(AccountUid uid, s32 *total_entries, s32 *start_entry_index, s32 *end_entry_index) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        s32 total_entries;
        s32 start_entry_index;
        s32 end_entry_index;
    } out;

    Result rc = serviceDispatchInOut(&g_pdmqrySrv, 12, uid, out);
    if (R_SUCCEEDED(rc)) {
        if (total_entries) *total_entries = out.total_entries;
        if (start_entry_index) *start_entry_index = out.start_entry_index;
        if (end_entry_index) *end_entry_index = out.end_entry_index;
    }
    return rc;
}

Result pdmqryQueryRecentlyPlayedApplication(AccountUid uid, u64 *application_ids, s32 count, s32 *total_out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_pdmqrySrv, 14, uid, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { application_ids, count*sizeof(u64) } },
    );
}

Result pdmqryGetRecentlyPlayedApplicationUpdateEvent(Event* out_event) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _pdmCmdGetEvent(&g_pdmqrySrv, out_event, false, 15);
}

