#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "services/news.h"
#include "service_guard.h"
#include "runtime/hosversion.h"
#include <string.h>

static NewsServiceType g_newsServiceType;
static Service g_newsCreatorSrv;
static Service g_newsSrv;

static Result _newsCreateNewsService(Service *srv_out);

NX_GENERATE_SERVICE_GUARD_PARAMS(news, (NewsServiceType service_type), (service_type));

Result _newsInitialize(NewsServiceType service_type) {
    Result rc;

    if (service_type >= NewsServiceType_Count)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    static const char* const service_names[] = {
        "news:a",
        "news:c",
        "news:m",
        "news:p",
        "news:v",
    };

    g_newsServiceType = service_type;

    if (hosversionBefore(2,0,0)) {
        rc = smGetService(&g_newsSrv, service_names[g_newsServiceType]);
    } else {
        rc = smGetService(&g_newsCreatorSrv, service_names[g_newsServiceType]);
        
        if (R_SUCCEEDED(rc)) rc = _newsCreateNewsService(&g_newsSrv);
    }

    return rc;
}

void _newsCleanup(void) {
    serviceClose(&g_newsSrv);
    serviceClose(&g_newsCreatorSrv);
}

Service *newsGetServiceSession(void) {
    return &g_newsSrv;
}

static Result _newsSrvOut(Service *srv, Service *srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _newsCreateNewsService(Service *srv_out) {
    return _newsSrvOut(&g_newsCreatorSrv, srv_out, 0);
}

Result newsCreateNewlyArrivedEventHolder(NewsNewlyArrivedEventHolder *out) {
    Service *srv;
    u32 cmd_id;
    if (hosversionBefore(2,0,0)) {
        srv = &g_newsSrv;
        cmd_id = 30900;
    } else {
        srv = &g_newsCreatorSrv;
        cmd_id = 1;
    }
    return _newsSrvOut(srv, &out->s, cmd_id);
}

Result newsCreateNewsDataService(NewsDataService *out) {
    Service *srv;
    u32 cmd_id;
    if (hosversionBefore(2,0,0)) {
        srv = &g_newsSrv;
        cmd_id = 30901;
    } else {
        srv = &g_newsCreatorSrv;
        cmd_id = 2;
    }
    return _newsSrvOut(srv, &out->s, cmd_id);
}

Result newsCreateNewsDatabaseService(NewsDatabaseService *out) {
    Service *srv;
    u32 cmd_id;
    if (hosversionBefore(2,0,0)) {
        srv = &g_newsSrv;
        cmd_id = 30902;
    } else {
        srv = &g_newsCreatorSrv;
        cmd_id = 3;
    }
    return _newsSrvOut(srv, &out->s, cmd_id);
}

Result newsCreateOverwriteEventHolder(NewsOverwriteEventHolder *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _newsSrvOut(&g_newsCreatorSrv, &out->s, 4);
}

Result newsPostLocalNews(const void *news, size_t size) {
    return serviceDispatch(&g_newsSrv, 10100,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { news, size }, },
    );
}

Result newsSetPassphrase(u64 program_id, const char *passphrase) {
    return serviceDispatchIn(&g_newsSrv, 20100, program_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { passphrase, strlen(passphrase) + 1 }, },
    );
}

Result newsGetSubscriptionStatus(const char *filter, u32 *status) {
    return serviceDispatchOut(&g_newsSrv, 30100, *status,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { filter, strlen(filter) + 1 }, },
    );
}

Result newsGetTopicList(u32 unk, u32 *out_count, NewsTopicName *out, u32 max_count) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_newsSrv, 30101, unk, *out_count,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, max_count * sizeof(*out) }, },
    );
}

Result newsGetSavedataUsage(u64 *current, u64 *total) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u64 current;
        u64 total;
    } out;

    Result rc = serviceDispatchOut(&g_newsSrv, 30110, out);

    if (R_SUCCEEDED(rc)) {
        if (current) *current = out.current;
        if (total) *total = out.total;
    }

    return rc;
}

Result newsIsSystemUpdateRequired(bool *out) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&g_newsSrv, 30200, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result newsGetDatabaseVersion(u32 *version) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_newsSrv, 30210, *version);
}

Result newsRequestImmediateReception(const char *filter) {
    return serviceDispatch(&g_newsSrv, 30300,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { filter, strlen(filter) + 1 }, },
    );
}

Result newsSetSubscriptionStatus(const char *filter, u32 status) {
    return serviceDispatchIn(&g_newsSrv, 40100, status,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { filter, strlen(filter) + 1 }, },
    );
}

Result newsClearStorage(void) {
    return serviceDispatch(&g_newsSrv, 40200);
}

Result newsClearSubscriptionStatusAll(void) {
    return serviceDispatch(&g_newsSrv, 40201);
}

Result newsGetNewsDatabaseDump(void *buffer, u64 size, u64 *out) {
    return serviceDispatchOut(&g_newsSrv, 90100, *out,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { buffer, size }, },
    );
}

void newsNewlyArrivedEventHolderClose(NewsNewlyArrivedEventHolder *srv) {
    serviceClose(&srv->s);
}

Result newsNewlyArrivedEventHolderGet(NewsNewlyArrivedEventHolder *srv, Event *out) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;
    
    rc = serviceDispatch(&srv->s, 0,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out, tmp_handle, true);
    return rc;
}

void newsDataClose(NewsDataService *srv) {
    serviceClose(&srv->s);
}

Result newsDataOpen(NewsDataService *srv, const char *file_name) {
    return serviceDispatch(&srv->s, 0,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { file_name, strlen(file_name) + 1 }, },
    );
}

Result newsDataOpenWithNewsRecordV1(NewsDataService *srv, NewsRecordV1 *record) {
    return serviceDispatchIn(&srv->s, 1, *record);
}

Result newsDataRead(NewsDataService *srv, u64 *bytes_read, u64 offset, void *out, size_t out_size) {
    return serviceDispatchInOut(&srv->s, 2, offset, *bytes_read,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { out, out_size }, },
    );
}

Result newsDataGetSize(NewsDataService *srv, u64 *size) {
    return serviceDispatchOut(&srv->s, 3, *size);
}

Result newsDataOpenWithNewsRecord(NewsDataService *srv, NewsRecord *record) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&srv->s, 1001, *record);
}

void newsDatabaseClose(NewsDatabaseService *srv) {
    serviceClose(&srv->s);
}

Result newsDatabaseGetListV1(NewsDatabaseService *srv, NewsRecordV1 *out, u32 max_count, const char *where, const char *order, u32 *count, u32 offset) {
    return serviceDispatchInOut(&srv->s, 0, offset, *count,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { out, max_count * sizeof(NewsRecordV1) },
            { where, strlen(where) + 1 },
            { order, strlen(order) + 1 },
        },
    );
}

Result newsDatabaseCount(NewsDatabaseService *srv, const char *filter, u32 *count) {
    return serviceDispatchOut(&srv->s, 1, *count,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { filter, strlen(filter) + 1 }, },
    );
}

Result newsDatabaseGetList(NewsDatabaseService *srv, NewsRecord *out, u32 max_count, const char *where, const char *order, u32 *count, u32 offset) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&srv->s, 1000, offset, *count,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { out, max_count * sizeof(NewsRecord) },
            { where, strlen(where) + 1 },
            { order, strlen(order) + 1 },
        },
    );
}

void newsOverwriteEventHolderClose(NewsOverwriteEventHolder *srv) {
    serviceClose(&srv->s);
}

Result newsOverwriteEventHolderGet(NewsOverwriteEventHolder *srv, Event *out) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;
    
    rc = serviceDispatch(&srv->s, 0,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out, tmp_handle, true);
    return rc;
}
