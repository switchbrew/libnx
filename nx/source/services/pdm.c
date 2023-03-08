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

static void _pdmConvertAppletEventFromV1(const PdmAppletEventV1 *in, PdmAppletEvent *out) {
    memset(out, 0, sizeof(*out));

    out->program_id = in->program_id;

    out->entry_index = in->entry_index;
    out->timestamp_user = pdmPlayTimestampToPosix(in->timestamp_user);
    out->timestamp_network = pdmPlayTimestampToPosix(in->timestamp_network);

    out->event_type = in->event_type;
}

static void _pdmConvertPlayStatisticsFromV1(const PdmPlayStatisticsV1 *in, PdmPlayStatistics *out) {
    memset(out, 0, sizeof(*out));

    out->program_id = in->program_id;

    out->first_entry_index = in->first_entry_index;
    out->first_timestamp_user = pdmPlayTimestampToPosix(in->first_timestamp_user);
    out->first_timestamp_network = pdmPlayTimestampToPosix(in->first_timestamp_network);

    out->last_entry_index = in->last_entry_index;
    out->last_timestamp_user = pdmPlayTimestampToPosix(in->last_timestamp_user);
    out->last_timestamp_network = pdmPlayTimestampToPosix(in->last_timestamp_network);

    out->playtime = ((u64)in->playtime_minutes) * 60 * 1000000000UL;
    out->total_launches = in->total_launches;
}

static void _pdmConvertAccountEventFromV3(const PdmAccountEventV3 *in, PdmAccountEvent *out) {
    memset(out, 0, sizeof(*out));

    out->uid = in->uid;

    out->entry_index = in->entry_index;
    out->timestamp_user = in->timestamp_user;
    out->timestamp_network = in->timestamp_network;

    out->type = in->type;
}

static void _pdmConvertAccountEventFromV10(const PdmAccountEventV10 *in, PdmAccountEvent *out) {
    memset(out, 0, sizeof(*out));

    out->uid = in->uid;
    out->program_id = in->program_id;

    out->entry_index = in->entry_index;
    out->timestamp_user = in->timestamp_user;
    out->timestamp_network = in->timestamp_network;

    out->type = in->type;
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

Result pdmqryQueryAppletEvent(s32 entry_index, bool flag, PdmAppletEvent *events, s32 count, s32 *total_out) {
    Result rc=0;

    if (hosversionBefore(10,0,0)) {
        rc = serviceDispatchInOut(&g_pdmqrySrv, 0, entry_index, *total_out,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
            .buffers = { { events, count*sizeof(PdmAppletEvent) } },
        );
    }
    else {
        const struct {
            u8 flag;
            u8 pad[3];
            s32 entry_index;
        } in = { flag!=0, {0}, entry_index };

        size_t entrysize = hosversionBefore(16,0,0) ? sizeof(PdmAppletEventV1) : sizeof(PdmAppletEvent);
        rc = serviceDispatchInOut(&g_pdmqrySrv, 0, in, *total_out,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
            .buffers = { { events, count*entrysize } },
        );
    }

    if (R_SUCCEEDED(rc) && hosversionBefore(16,0,0)) {
        s32 end = count;
        if (total_out && *total_out < end) end = *total_out;
        PdmAppletEventV1 *v1_in = (PdmAppletEventV1*)events;
        for (s32 i=end-1; i>=0; i--) { // V1 is smaller than latest, so loop backwards.
            PdmAppletEventV1 tmp = v1_in[i];
            _pdmConvertAppletEventFromV1(&tmp, &events[i]);
        }
    }

    return rc;
}

Result pdmqryQueryPlayStatisticsByApplicationId(u64 application_id, bool flag, PdmPlayStatistics *stats) {
    PdmPlayStatisticsV1 tmp={};
    Result rc=0;

    if (hosversionBefore(10,0,0)) {
        rc = serviceDispatchInOut(&g_pdmqrySrv, 4, application_id, tmp);
        if (R_SUCCEEDED(rc)) _pdmConvertPlayStatisticsFromV1(&tmp, stats);
        return rc;
    }

    const struct {
        u8 flag;
        u8 pad[7];
        u64 application_id;
    } in = { flag!=0, {0}, application_id };

    if (hosversionBefore(16,0,0)) {
        Result rc = serviceDispatchInOut(&g_pdmqrySrv, 4, in, tmp);
        if (R_SUCCEEDED(rc)) _pdmConvertPlayStatisticsFromV1(&tmp, stats);
    }
    else
        rc = serviceDispatchInOut(&g_pdmqrySrv, 4, in, *stats);
    return rc;
}

Result pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(u64 application_id, AccountUid uid, bool flag, PdmPlayStatistics *stats) {
    PdmPlayStatisticsV1 tmp={};
    Result rc=0;

    if (hosversionBefore(10,0,0)) {
        const struct {
            u64 application_id;
            AccountUid uid;
        } in = { application_id, uid };

        rc = serviceDispatchInOut(&g_pdmqrySrv, 5, in, tmp);
        if (R_SUCCEEDED(rc)) _pdmConvertPlayStatisticsFromV1(&tmp, stats);
        return rc;
    }

    const struct {
        u8 flag;
        u8 pad[7];
        u64 application_id;
        AccountUid uid;
    } in = { flag!=0, {0}, application_id, uid };

    if (hosversionBefore(16,0,0)) {
        Result rc = serviceDispatchInOut(&g_pdmqrySrv, 5, in, tmp);
        if (R_SUCCEEDED(rc)) _pdmConvertPlayStatisticsFromV1(&tmp, stats);
    }
    else
        rc = serviceDispatchInOut(&g_pdmqrySrv, 5, in, *stats);
    return rc;
}

Result pdmqryQueryLastPlayTime(bool flag, PdmLastPlayTime *playtimes, const u64 *application_ids, s32 count, s32 *total_out) {
    if (hosversionBefore(10,0,0)) {
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

    const u8 in = flag!=0;
    return serviceDispatchInOut(&g_pdmqrySrv, 17, in, *total_out,
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
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    size_t entrysize = sizeof(PdmAccountEventV3);
    u32 ver=0;
    if (hosversionBetween(10, 16)) {
        entrysize = sizeof(PdmAccountEventV10);
        ver=1;
    }
    else if (hosversionAtLeast(16,0,0)) {
        entrysize = sizeof(PdmAccountEvent);
        ver=2;
    }

    Result rc=0;
    if (ver!=1) rc = _pdmqryQueryEvent(entry_index, events, entrysize, count, total_out, 10);
    if (R_SUCCEEDED(rc) && ver==0) {
        s32 end = count;
        if (total_out && *total_out < end) end = *total_out;
        PdmAccountEventV3 *v3_in = (PdmAccountEventV3*)events;
        for (s32 i=0; i<end; i++) {
            PdmAccountEventV3 tmp = v3_in[i];
            _pdmConvertAccountEventFromV3(&tmp, &events[i]);
        }
    }
    else if (ver==1) { // V10 needs special handling since it's larger than latest.
        PdmAccountEventV10 v10_tmp[4]={};
        s32 tmp_out=0;
        if (total_out) *total_out = 0;

        for (s32 i=0; i<count; i+=4) {
            size_t cur_count = 4;
            if (count-i < cur_count) cur_count = count-i;
            rc = _pdmqryQueryEvent(entry_index+i, v10_tmp, entrysize, cur_count, &tmp_out, 10);
            if (R_FAILED(rc) || tmp_out<=0) break;
            if (tmp_out>cur_count) tmp_out = cur_count;
            if (total_out) *total_out += tmp_out;
            for (s32 j=0; j<tmp_out; j++) {
                _pdmConvertAccountEventFromV10(&v10_tmp[j], &events[i+j]);
            }
            if (tmp_out < 4) break;
        }
    }
    return rc;
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

Result pdmqryQueryRecentlyPlayedApplication(AccountUid uid, bool flag, u64 *application_ids, s32 count, s32 *total_out) {
    if (!hosversionBetween(6,15))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (hosversionBefore(10,0,0)) {
        return serviceDispatchInOut(&g_pdmqrySrv, 14, uid, *total_out,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
            .buffers = { { application_ids, count*sizeof(u64) } },
        );
    }

    const struct {
        u8 flag;
        u8 pad[7];
        AccountUid uid;
    } in = { flag!=0, {0}, uid };

    return serviceDispatchInOut(&g_pdmqrySrv, 14, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { application_ids, count*sizeof(u64) } },
    );
}

Result pdmqryGetRecentlyPlayedApplicationUpdateEvent(Event* out_event) {
    if (!hosversionBetween(6,15))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _pdmCmdGetEvent(&g_pdmqrySrv, out_event, false, 15);
}

