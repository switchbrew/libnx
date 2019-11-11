#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ncm.h"

static Service g_ncmSrv;

NX_GENERATE_SERVICE_GUARD(ncm);

Result _ncmInitialize(void) {
    return smGetService(&g_ncmSrv, "ncm");
}

void _ncmCleanup(void) {
    serviceClose(&g_ncmSrv);
}

Service* ncmGetServiceSession(void) {
    return &g_ncmSrv;
}

static Result _ncmGetInterfaceInU8(Service* srv_out, u8 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_ncmSrv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _ncmCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _ncmCmdNoInOutU64(Service* srv, u64* outval, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *outval);
}

static Result _ncmCmdOutPlaceHolderId(Service* srv, NcmPlaceHolderId* outval, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *outval);
}

static Result _ncmCmdInU8(Service* srv, u8 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _ncmCmdInContentId(Service* srv, const NcmContentId* inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, *inval);
}

static Result _ncmCmdInPlaceHolderId(Service* srv, const NcmPlaceHolderId* inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, *inval);
}

static Result _ncmCmdInContentIdOutU64(Service* srv, const NcmContentId* inval, u64* outval, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, *inval, *outval);
}

static Result _ncmCmdInPlaceHolderIdOutU64(Service* srv, const NcmPlaceHolderId* inval, u64* outval, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, *inval, *outval);
}

Result ncmCreateContentStorage(NcmStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 0);
}

Result ncmCreateContentMetaDatabase(NcmStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 1);
}

Result ncmVerifyContentStorage(NcmStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 2);
}

Result ncmVerifyContentMetaDatabase(NcmStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 3);
}

Result ncmOpenContentStorage(NcmContentStorage* out_content_storage, NcmStorageId storage_id) {
    return _ncmGetInterfaceInU8(&out_content_storage->s, storage_id, 4);
}

Result ncmOpenContentMetaDatabase(NcmContentMetaDatabase* out_content_meta_database, NcmStorageId storage_id) {
    return _ncmGetInterfaceInU8(&out_content_meta_database->s, storage_id, 5);
}

Result ncmCloseContentStorageForcibly(NcmStorageId storage_id) {
    if (hosversionAtLeast(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 6);
}

Result ncmCloseContentMetaDatabaseForcibly(NcmStorageId storage_id) {
    if (hosversionAtLeast(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 7);
}

Result ncmCleanupContentMetaDatabase(NcmStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 8);
}

Result ncmActivateContentStorage(NcmStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 9);
}

Result ncmInactivateContentStorage(NcmStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 10);
}

Result ncmActivateContentMetaDatabase(NcmStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 11);
}

Result ncmInactivateContentMetaDatabase(NcmStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, storage_id, 12);
}

Result ncmInvalidateRightsIdCache(void) {
    if (hosversionBefore(9,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoIO(&g_ncmSrv, 13);
}

void ncmContentStorageClose(NcmContentStorage* cs) {
    serviceClose(&cs->s);
}

Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmPlaceHolderId* out_id) {
    return _ncmCmdOutPlaceHolderId(&cs->s, out_id, 0);
}

Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmContentId* content_id, const NcmPlaceHolderId* placeholder_id, s64 size) {
    const struct {
        NcmContentId content_id;
        NcmPlaceHolderId placeholder_id;
        s64 size;
    } in = { *content_id, *placeholder_id, size };
    return serviceDispatchIn(&cs->s, 1, in);
}

Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id) {
    return _ncmCmdInPlaceHolderId(&cs->s, placeholder_id, 2);
}

Result ncmContentStorageHasPlaceHolder(NcmContentStorage* cs, bool* out, const NcmPlaceHolderId* placeholder_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&cs->s, 3, *placeholder_id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id, u64 offset, const void* data, size_t data_size) {
    const struct {
        NcmPlaceHolderId placeholder_id;
        u64 offset;
    } in = { *placeholder_id, offset };
    return serviceDispatchIn(&cs->s, 4, in, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { data, data_size } },
    );
}

Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmContentId* content_id, const NcmPlaceHolderId* placeholder_id) {
    const struct {
        NcmContentId content_id;
        NcmPlaceHolderId placeholder_id;
    } in = { *content_id, *placeholder_id };
    return serviceDispatchIn(&cs->s, 5, in);
}

Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmContentId* content_id) {
    return _ncmCmdInContentId(&cs->s, content_id, 6);
}

Result ncmContentStorageHas(NcmContentStorage* cs, bool* out, const NcmContentId* content_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&cs->s, 7, *content_id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ncmContentStorageGetPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmContentId* content_id) {
    char tmpbuf[0x300]={0};
    Result rc = serviceDispatchIn(&cs->s, 8, *content_id,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { tmpbuf, sizeof(tmpbuf) } },
    );
    if (R_SUCCEEDED(rc) && out_path) {
        strncpy(out_path, tmpbuf, out_size-1);
        out_path[out_size-1] = 0;
    }
    return rc;
}

Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmPlaceHolderId* placeholder_id) {
    char tmpbuf[0x300]={0};
    Result rc = serviceDispatchIn(&cs->s, 9, *placeholder_id,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { tmpbuf, sizeof(tmpbuf) } },
    );
    if (R_SUCCEEDED(rc) && out_path) {
        strncpy(out_path, tmpbuf, out_size-1);
        out_path[out_size-1] = 0;
    }
    return rc;
}

Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs) {
    return _ncmCmdNoIO(&cs->s, 10);
}

Result ncmContentStorageListPlaceHolder(NcmContentStorage* cs, NcmPlaceHolderId* out_ids, s32 count, s32* out_count) {
    return serviceDispatchOut(&cs->s, 11, *out_count,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_ids, count*sizeof(NcmPlaceHolderId) } },
    );
}

Result ncmContentStorageGetContentCount(NcmContentStorage* cs, s32* out_count) {
    return serviceDispatchOut(&cs->s, 12, *out_count);
}

Result ncmContentStorageListContentId(NcmContentStorage* cs, NcmContentId* out_ids, s32 count, s32* out_count, s32 start_offset) {
    return serviceDispatchInOut(&cs->s, 13, start_offset, *out_count, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_ids, count*sizeof(NcmContentId) } },
    );
}

Result ncmContentStorageGetSizeFromContentId(NcmContentStorage* cs, s64* out_size, const NcmContentId* content_id) {
    return _ncmCmdInContentIdOutU64(&cs->s, content_id, (u64*)out_size, 14);
}

Result ncmContentStorageDisableForcibly(NcmContentStorage* cs) {
    return _ncmCmdNoIO(&cs->s, 15);
}

Result ncmContentStorageRevertToPlaceHolder(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id, const NcmContentId* old_content_id, const NcmContentId* new_content_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmContentId old_content_id;
        NcmContentId new_content_id;
        NcmPlaceHolderId placeholder_id;
    } in = { *old_content_id, *new_content_id, *placeholder_id };
    return serviceDispatchIn(&cs->s, 16, in);
}

Result ncmContentStorageSetPlaceHolderSize(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id, s64 size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmPlaceHolderId placeholder_id;
        s64 size;
    } in = { *placeholder_id, size };
    return serviceDispatchIn(&cs->s, 17, in);
}

Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, void* out_data, size_t out_data_size, const NcmContentId* content_id, s64 offset) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmContentId content_id;
        s64 offset;
    } in = { *content_id, offset };
    return serviceDispatchIn(&cs->s, 18, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_data, out_data_size } },
    );
}

Result ncmContentStorageGetRightsIdFromPlaceHolderId(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmPlaceHolderId* placeholder_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (hosversionBefore(3,0,0))
        return serviceDispatchInOut(&cs->s, 19, *placeholder_id, out_rights_id->rights_id);
    else
        return serviceDispatchInOut(&cs->s, 19, *placeholder_id, *out_rights_id);
}

Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmContentId* content_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    memset(out_rights_id, 0, sizeof(*out_rights_id));
    if (hosversionBefore(3,0,0))
        return serviceDispatchInOut(&cs->s, 20, *content_id, out_rights_id->rights_id);
    else
        return serviceDispatchInOut(&cs->s, 20, *content_id, *out_rights_id);
}

Result ncmContentStorageWriteContentForDebug(NcmContentStorage* cs, const NcmContentId* content_id, s64 offset, const void* data, size_t data_size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmContentId content_id;
        s64 offset;
    } in = { *content_id, offset };
    return serviceDispatchIn(&cs->s, 21, in, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { data, data_size } },
    );
}

Result ncmContentStorageGetFreeSpaceSize(NcmContentStorage* cs, s64* out_size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoInOutU64(&cs->s, (u64*)out_size, 22);
}

Result ncmContentStorageGetTotalSpaceSize(NcmContentStorage* cs, s64* out_size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoInOutU64(&cs->s, (u64*)out_size, 23);
}

Result ncmContentStorageFlushPlaceHolder(NcmContentStorage* cs) {
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoIO(&cs->s, 24);
}

Result ncmContentStorageGetSizeFromPlaceHolderId(NcmContentStorage* cs, s64* out_size, const NcmPlaceHolderId* placeholder_id) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInPlaceHolderIdOutU64(&cs->s, placeholder_id, (u64*)out_size, 25);
}

Result ncmContentStorageRepairInvalidFileAttribute(NcmContentStorage* cs) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoIO(&cs->s, 26);
}

Result ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmPlaceHolderId* placeholder_id, const NcmContentId* cache_content_id) {
    if (hosversionBefore(8,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmContentId cache_content_id;
        NcmPlaceHolderId placeholder_id;
    } in = { *cache_content_id, *placeholder_id };

    if (hosversionBefore(3,0,0))
        return serviceDispatchInOut(&cs->s, 27, in, out_rights_id->rights_id);
    else
        return serviceDispatchInOut(&cs->s, 27, in, *out_rights_id);
}

void ncmContentMetaDatabaseClose(NcmContentMetaDatabase* db) {
    serviceClose(&db->s);
}

Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, const void* data, u64 data_size) {
    return serviceDispatchIn(&db->s, 0, *key,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { data, data_size } },
    );
}

Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u64* out_size, void* out_data, u64 out_data_size) {
    return serviceDispatchInOut(&db->s, 1, *key, *out_size, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_data, out_data_size } },
    );
}

Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmContentMetaKey *key) {
    return serviceDispatchIn(&db->s, 2, *key);
}

Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmContentId* out_content_id, const NcmContentMetaKey* key, NcmContentType type) {
    const struct {
        u8 type;
        u8 padding[7];
        NcmContentMetaKey key;
    } in = { type, {0}, *key };
    return serviceDispatchInOut(&db->s, 3, in, *out_content_id);
}

Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, s32* out_entries_written, NcmContentInfo* out_info, s32 count, const NcmContentMetaKey* key, s32 start_index) {
    const struct {
        s32 start_index;
        u32 pad;
        NcmContentMetaKey key;
    } in = { start_index, 0, *key };
    return serviceDispatchInOut(&db->s, 4, in, *out_entries_written,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_info, count*sizeof(NcmContentInfo) } },
    );
}

Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, s32* out_entries_total, s32* out_entries_written, NcmContentMetaKey* out_keys, s32 count, NcmContentMetaType meta_type, u64 id, u64 id_min, u64 id_max, NcmContentInstallType install_type) {
    const struct {
        u8 meta_type;
        u8 install_type;
        u8 padding[6];
        u64 id;
        u64 id_min;
        u64 id_max;
    } in = { meta_type, install_type, {0}, id, id_min, id_max };
    struct {
        s32 out_entries_total;
        s32 out_entries_written;
    } out;
    Result rc = serviceDispatchInOut(&db->s, 5, in, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_keys, count*sizeof(NcmContentMetaKey) } },
    );
    if (R_SUCCEEDED(rc)) {
        if (out_entries_total) *out_entries_total = out.out_entries_total;
        if (out_entries_written) *out_entries_written = out.out_entries_written;
    }
    return rc;
}

Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, NcmContentMetaKey* out_key, u64 id) {
    return serviceDispatchInOut(&db->s, 6, id, *out_key);
}

Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, s32* out_entries_total, s32* out_entries_written, NcmApplicationContentMetaKey* out_keys, s32 count, NcmContentMetaType meta_type) {
    struct {
        s32 out_entries_total;
        s32 out_entries_written;
    } out;
    Result rc = serviceDispatchInOut(&db->s, 7, meta_type, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_keys, count*sizeof(NcmApplicationContentMetaKey) } },
    );
    if (R_SUCCEEDED(rc)) {
        if (out_entries_total) *out_entries_total = out.out_entries_total;
        if (out_entries_written) *out_entries_written = out.out_entries_written;
    }
    return rc;
}

Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&db->s, 8, *key, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ncmContentMetaDatabaseHasAll(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* keys, s32 count) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&db->s, 9, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { keys, count*sizeof(NcmContentMetaKey) } },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ncmContentMetaDatabaseGetSize(NcmContentMetaDatabase* db, u64* out_size, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 10, *key, *out_size);
}

Result ncmContentMetaDatabaseGetRequiredSystemVersion(NcmContentMetaDatabase* db, u32* out_version, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 11, *key, *out_version);
}

Result ncmContentMetaDatabaseGetPatchId(NcmContentMetaDatabase* db, u64* out_patch_id, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 12, *key, *out_patch_id);
}

Result ncmContentMetaDatabaseDisableForcibly(NcmContentMetaDatabase* db) {
    return _ncmCmdNoIO(&db->s, 13);
}

Result ncmContentMetaDatabaseLookupOrphanContent(NcmContentMetaDatabase* db, bool* out_orphaned, const NcmContentId* content_ids, s32 count) {
    return serviceDispatch(&db->s, 14, 
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = { 
            { out_orphaned, count },
            { content_ids, count*sizeof(NcmContentId) },
        },
    );
}

Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db) {
    return _ncmCmdNoIO(&db->s, 15);
}

Result ncmContentMetaDatabaseHasContent(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key, const NcmContentId* content_id) {
    const struct {
        NcmContentId content_id;
        NcmContentMetaKey key;
    } in = { *content_id, *key };
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&db->s, 16, in, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ncmContentMetaDatabaseListContentMetaInfo(NcmContentMetaDatabase* db, s32* out_entries_written, void* out_meta_info, s32 count, const NcmContentMetaKey* key, s32 start_index) {
    const struct {
        s32 start_index;
        u32 padding;
        NcmContentMetaKey key;
    } in = { start_index, 0, *key };
    return serviceDispatchInOut(&db->s, 17, in, *out_entries_written,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_meta_info, count*sizeof(NcmContentMetaInfo) } },
    );
}

Result ncmContentMetaDatabaseGetAttributes(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u8* out) {
    return serviceDispatchInOut(&db->s, 18, *key, *out);
}

Result ncmContentMetaDatabaseGetRequiredApplicationVersion(NcmContentMetaDatabase* db, u32* out_version, const NcmContentMetaKey* key) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchInOut(&db->s, 19, *key, *out_version);
}

Result ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(NcmContentMetaDatabase* db, NcmContentId* out_content_id, const NcmContentMetaKey* key, NcmContentType type, u8 id_offset) {
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        u8 type;
        u8 id_offset;
        u8 padding[6];
        NcmContentMetaKey key;
    } in = { type, id_offset, {0}, *key };
    return serviceDispatchInOut(&db->s, 20, in, *out_content_id);
}
