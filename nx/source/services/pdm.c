#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "services/sm.h"
#include "services/pdm.h"
#include "runtime/hosversion.h"

static Service g_pdmqrySrv;
static u64 g_pdmqryRefCnt;

Result pdmqryInitialize(void) {
    atomicIncrement64(&g_pdmqryRefCnt);

    if (serviceIsActive(&g_pdmqrySrv))
        return 0;

    Result rc = smGetService(&g_pdmqrySrv, "pdm:qry");

    if (R_FAILED(rc)) pdmqryExit();

    return rc;
}

void pdmqryExit(void) {
    if (atomicDecrement64(&g_pdmqryRefCnt) == 0)
        serviceClose(&g_pdmqrySrv);
}

static Result _pdmqryQueryEvent(u64 cmd_id, u32 entryindex, void* events, size_t entrysize, s32 count, s32 *total_out) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, events, count*entrysize, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 entryindex;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->entryindex = entryindex;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result pdmqryQueryApplicationEvent(u32 entryindex, PdmApplicationEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(0, entryindex, events, sizeof(PdmApplicationEvent), count, total_out);
}

Result pdmqryQueryPlayStatisticsByApplicationId(u64 titleID, PdmPlayStatistics *stats) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            PdmPlayStatistics stats;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && stats) memcpy(stats, &resp->stats, sizeof(resp->stats));
    }

    return rc;
}

Result pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(u64 titleID, u128 userID, PdmPlayStatistics *stats) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
        union { u128 userID; } PACKED;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->titleID = titleID;
    raw->userID = userID;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            PdmPlayStatistics stats;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && stats) memcpy(stats, &resp->stats, sizeof(resp->stats));
    }

    return rc;
}

Result pdmqryQueryLastPlayTime(PdmLastPlayTime *playtimes, const u64 *titleIDs, s32 count, s32 *total_out) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, titleIDs, count*sizeof(u64), BufferType_Normal);
    ipcAddRecvBuffer(&c, playtimes, count*sizeof(PdmLastPlayTime), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result pdmqryQueryPlayEvent(u32 entryindex, PdmPlayEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(8, entryindex, events, sizeof(PdmPlayEvent), count, total_out);
}

Result pdmqryGetAvailablePlayEventRange(u32 *total_entries, u32 *start_entryindex, u32 *end_entryindex) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 total_entries;
            u32 start_entryindex;
            u32 end_entryindex;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
        if (R_SUCCEEDED(rc) && start_entryindex) *start_entryindex = resp->start_entryindex;
        if (R_SUCCEEDED(rc) && end_entryindex) *end_entryindex = resp->end_entryindex;
    }

    return rc;
}

Result pdmqryQueryAccountEvent(u32 entryindex, PdmAccountEvent *events, s32 count, s32 *total_out) {
    return _pdmqryQueryEvent(10, entryindex, events, sizeof(PdmAccountEvent), count, total_out);
}

Result pdmqryQueryAccountPlayEvent(u32 entryindex, u128 userID, PdmAccountPlayEvent *events, s32 count, s32 *total_out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    ipcAddRecvBuffer(&c, events, count*sizeof(PdmAccountPlayEvent), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 entryindex;
        u32 pad;
        union { u128 userID; } PACKED;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->entryindex = entryindex;
    raw->userID = userID;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_out;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result pdmqryGetAvailableAccountPlayEventRange(u128 userID, u32 *total_entries, u32 *start_entryindex, u32 *end_entryindex) {
    IpcCommand c;
    ipcInitialize(&c);

    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 magic;
        u64 cmd_id;
        u128 userID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->userID = userID;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 total_entries;
            u32 start_entryindex;
            u32 end_entryindex;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
        if (R_SUCCEEDED(rc) && start_entryindex) *start_entryindex = resp->start_entryindex;
        if (R_SUCCEEDED(rc) && end_entryindex) *end_entryindex = resp->end_entryindex;
    }

    return rc;
}

Result pdmqryGetUserPlayedApplications(u128 userID, u64 *titleIDs, size_t count, u32 *total_out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    ipcAddRecvBuffer(&c, titleIDs, count*sizeof(u64), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u128 userID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    raw->userID = userID;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 total_out;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_out) *total_out = resp->total_out;
    }

    return rc;
}

Result pdmqryGetUserAccountEvent(Event* event_out) {
    IpcCommand c;
    ipcInitialize(&c);

    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pdmqrySrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;

    Result rc = serviceIpcDispatch(&g_pdmqrySrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pdmqrySrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event_out, r.Handles[0], false);
        }
    }

    return rc;
}

