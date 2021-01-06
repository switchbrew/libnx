#include "service_guard.h"
#include "services/avm.h"
#include "runtime/hosversion.h"

static Service g_AvmSrv;

NX_GENERATE_SERVICE_GUARD(avm);

Result _avmInitialize(void) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;

    rc = smGetService(&g_AvmSrv, "avm");

    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_AvmSrv);

    return rc;
}

void _avmCleanup(void) {
    serviceClose(&g_AvmSrv);
}

Service *avmGetServiceSession(void) {
    return &g_AvmSrv;
}

static Result _avmGetVersion(u32 cmd_id, u64 id_1, u64 id_2, u32 *version) {
    const struct {
        u64 a, b;
    } in = { id_1, id_2 };

    return serviceDispatchInOut(&g_AvmSrv, 100, in, *version);
}

Result avmGetHighestAvailableVersion(u64 id_1, u64 id_2, u32 *version) {
    return _avmGetVersion(100, id_1, id_2, version);
}

Result avmGetHighestRequiredVersion(u64 id_1, u64 id_2, u32 *version) {
    return _avmGetVersion(101, id_1, id_2, version);
}

Result avmGetVersionListEntry(u64 application_id, AvmVersionListEntry *entry) {
    return serviceDispatchInOut(&g_AvmSrv, 102, application_id, *entry);
}

Result avmGetVersionListImporter(AvmVersionListImporter *out) {
    return serviceDispatch(&g_AvmSrv, 103,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result avmGetLaunchRequiredVersion(u64 application_id, u32 *version) {
    return serviceDispatchInOut(&g_AvmSrv, 200, application_id, *version);
}

static Result _avmPushVersion(u32 cmd_id, u64 application_id, u32 version) {
    const struct {
        u64 application_id;
        u32 version;
    } in = { application_id, version };

    return serviceDispatchIn(&g_AvmSrv, cmd_id, in);
}

Result avmUpgradeLaunchRequiredVersion(u64 application_id, u32 version) {
    return _avmPushVersion(202, application_id, version);
}

Result avmPushLaunchVersion(u64 application_id, u32 version) {
    return _avmPushVersion(100, application_id, version);
}

Result avmListVersionList(AvmVersionListEntry *buffer, size_t count, u32 *out) {
    return serviceDispatchOut(&g_AvmSrv, 1001, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, count * sizeof(*buffer) } },
    );
}

Result avmListRequiredVersion(AvmRequiredVersionEntry *buffer, size_t count, u32 *out) {
    return serviceDispatchOut(&g_AvmSrv, 1002, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, count * sizeof(*buffer) } },
    );
}

void avmVersionListImporterClose(AvmVersionListImporter *srv) {
    return serviceClose(&srv->s);
}

Result avmVersionListImporterSetTimestamp(AvmVersionListImporter *srv, u64 timestamp) {
    return serviceDispatchIn(&srv->s, 0, timestamp);
}

Result avmVersionListImporterSetData(AvmVersionListImporter *srv, const AvmVersionListEntry *entries, u32 count) {
    return serviceDispatch(&srv->s, 1,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { entries, count * sizeof(entries) } },
    );
}

Result avmVersionListImporterFlush(AvmVersionListImporter *srv) {
    return serviceDispatch(&srv->s, 2);
}
