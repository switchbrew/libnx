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

static Result _ncmGetInterfaceInU8(Service* srv_out, u32 cmd_id, u8 inval) {
    return serviceDispatchIn(&g_ncmSrv, cmd_id, inval,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _ncmCmdNoIo(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _ncmCmdNoInOutU64(Service* srv, u32 cmd_id, u64* outval) {
    return serviceDispatchOut(srv, cmd_id, *outval);
}

static Result _ncmCmdOutNcaId(Service* srv, u32 cmd_id, NcmNcaId* outval) {
    return serviceDispatchOut(srv, cmd_id, *outval);
}

static Result _ncmCmdInU8(Service* srv, u32 cmd_id, u8 inval) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _ncmCmdInNcaId(Service* srv, u32 cmd_id, const NcmNcaId* inval) {
    return serviceDispatchIn(srv, cmd_id, *inval);
}

static Result _ncmCmdInNcaIdOutU64(Service* srv, u32 cmd_id, const NcmNcaId* inval, u64* outval) {
    return serviceDispatchInOut(srv, cmd_id, *inval, *outval);
}

Result ncmCreateContentStorage(FsStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, 0, storage_id);
}

Result ncmCreateContentMetaDatabase(FsStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, 1, storage_id);
}

Result ncmVerifyContentStorage(FsStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, 2, storage_id);
}

Result ncmVerifyContentMetaDatabase(FsStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, 3, storage_id);
}

Result ncmOpenContentStorage(NcmContentStorage* out_content_storage, FsStorageId storage_id) {
    return _ncmGetInterfaceInU8(&out_content_storage->s, 4, storage_id);
}

Result ncmOpenContentMetaDatabase(NcmContentMetaDatabase* out_content_meta_database, FsStorageId storage_id) {
    return _ncmGetInterfaceInU8(&out_content_meta_database->s, 5, storage_id);
}

Result ncmCloseContentStorageForcibly(FsStorageId storage_id) {
    if (hosversionAtLeast(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, 6, storage_id);
}

Result ncmCloseContentMetaDatabaseForcibly(FsStorageId storage_id) {
    if (hosversionAtLeast(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, 7, storage_id);
}

Result ncmCleanupContentMetaDatabase(FsStorageId storage_id) {
    return _ncmCmdInU8(&g_ncmSrv, 8, storage_id);
}

Result ncmActivateContentStorage(FsStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, 9, storage_id);
}

Result ncmInactivateContentStorage(FsStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, 10, storage_id);
}

Result ncmActivateContentMetaDatabase(FsStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, 11, storage_id);
}

Result ncmInactivateContentMetaDatabase(FsStorageId storage_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInU8(&g_ncmSrv, 12, storage_id);
}

Result ncmInvalidateRightsIdCache(void) {
    if (hosversionBefore(9,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoIo(&g_ncmSrv, 13);
}

void ncmContentStorageClose(NcmContentStorage* cs) {
    serviceClose(&cs->s);
}

Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmNcaId* out_id) {
    return _ncmCmdOutNcaId(&cs->s, 0, out_id);
}

Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmNcaId* content_id, const NcmNcaId* placeholder_id, u64 size) {
    const struct {
        NcmNcaId content_id;
        NcmNcaId placeholder_id;
        u64 size;
    } in = { *content_id, *placeholder_id, size };
    return serviceDispatchIn(&cs->s, 1, in);
}

Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholder_id) {
    return _ncmCmdInNcaId(&cs->s, 2, placeholder_id);
}

Result ncmContentStorageHasPlaceHolder(NcmContentStorage* cs, bool* out, const NcmNcaId* placeholder_id) {
    return serviceDispatchInOut(&cs->s, 3, *placeholder_id, *out);
}

Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholder_id, u64 offset, const void* data, size_t data_size) {
    const struct {
        NcmNcaId placeholder_id;
        u64 offset;
    } in = { *placeholder_id, offset };
    return serviceDispatchIn(&cs->s, 4, in, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { data, data_size } },
    );
}

Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmNcaId* content_id, const NcmNcaId* placeholder_id) {
    const struct {
        NcmNcaId content_id;
        NcmNcaId placeholder_id;
    } in = { *content_id, *placeholder_id };
    return serviceDispatchIn(&cs->s, 5, in);
}

Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmNcaId* content_id) {
    return _ncmCmdInNcaId(&cs->s, 6, content_id);
}

Result ncmContentStorageHas(NcmContentStorage* cs, bool* out, const NcmNcaId* content_id) {
    return serviceDispatchInOut(&cs->s, 7, *content_id, *out);
}

Result ncmContentStorageGetPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmNcaId* content_id) {
    return serviceDispatchIn(&cs->s, 8, *content_id, 
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out_path, out_size } },
    );
}

Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, const char* out_path, size_t out_size, const NcmNcaId* placeholder_id) {
    return serviceDispatchIn(&cs->s, 9, *placeholder_id, 
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out_path, out_size } },
    );
}

Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs) {
    return _ncmCmdNoIo(&cs->s, 10);
}

Result ncmContentStorageListPlaceHolder(NcmContentStorage* cs, NcmNcaId* out_ids, size_t out_ids_size, u32* out_count) {
    return serviceDispatchOut(&cs->s, 11, out_count, *out_count, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_ids, out_ids_size } },
    );
}

Result ncmContentStorageGetContentCount(NcmContentStorage* cs, u32* out_count) {
    return serviceDispatchOut(&cs->s, 12, *out_count);
}

Result ncmContentStorageListContentId(NcmContentStorage* cs, NcmNcaId* out_ids, size_t out_ids_size, u32* out_count, u32 start_offset) {
    return serviceDispatchInOut(&cs->s, 13, start_offset, *out_count, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_ids, out_ids_size } },
    );
}

Result ncmContentStorageGetSizeFromContentId(NcmContentStorage* cs, u64* out_size, const NcmNcaId* content_id) {
    return _ncmCmdInNcaIdOutU64(&cs->s, 14, content_id, out_size);
}

Result ncmContentStorageDisableForcibly(NcmContentStorage* cs) {
    return _ncmCmdNoIo(&cs->s, 15);
}

Result ncmContentStorageRevertToPlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholder_id, const NcmNcaId* old_content_id, const NcmNcaId* new_content_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmNcaId old_content_id;
        NcmNcaId new_content_id;
        NcmNcaId placeholder_id;
    } in = { *old_content_id, *new_content_id, *placeholder_id };
    return serviceDispatchIn(&cs->s, 16, in);
}

Result ncmContentStorageSetPlaceHolderSize(NcmContentStorage* cs, const NcmNcaId* placeholder_id, u64 size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmNcaId placeholder_id;
        u64 size;
    } in = { *placeholder_id, size };
    return serviceDispatchIn(&cs->s, 17, in);
}

Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, void* out_data, size_t out_data_size, const NcmNcaId* content_id, u64 offset) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmNcaId content_id;
        u64 offset;
    } in = { *content_id, offset };
    return serviceDispatchIn(&cs->s, 18, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_data, out_data_size } },
    );
}

Result ncmContentStorageGetRightsIdFromPlaceHolderId(NcmContentStorage* cs, FsRightsId* out_rights_id, u32* out_key_generation, const NcmNcaId* placeholder_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        FsRightsId rights_id;
        u32 key_generation;
    } out;
    Result rc = serviceDispatchInOut(&cs->s, 19, *placeholder_id, out);
    if (R_SUCCEEDED(rc)) {
        if (out_rights_id) *out_rights_id = out.rights_id;
        if (out_key_generation) *out_key_generation = out.key_generation;
    }
    return rc;
}

Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, FsRightsId* out_rights_id, u32* out_key_generation, const NcmNcaId* content_id) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        FsRightsId rights_id;
        u32 key_generation;
    } out;
    Result rc = serviceDispatchInOut(&cs->s, 20, *content_id, out);
    if (R_SUCCEEDED(rc)) {
        if (out_rights_id) *out_rights_id = out.rights_id;
        if (out_key_generation) *out_key_generation = out.key_generation;
    }
    return rc;
}

Result ncmContentStorageWriteContentForDebug(NcmContentStorage* cs, const NcmNcaId* content_id, u64 offset, const void* data, size_t data_size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmNcaId content_id;
        u64 offset;
    } in = { *content_id, offset };
    return serviceDispatchIn(&cs->s, 21, in, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { data, data_size } },
    );
}

Result ncmContentStorageGetFreeSpaceSize(NcmContentStorage* cs, u64* out_size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoInOutU64(&cs->s, 22, out_size);
}

Result ncmContentStorageGetTotalSpaceSize(NcmContentStorage* cs, u64* out_size) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoInOutU64(&cs->s, 23, out_size);
}

Result ncmContentStorageFlushPlaceHolder(NcmContentStorage* cs) {
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoIo(&cs->s, 24);
}

Result ncmContentStorageGetSizeFromPlaceHolderId(NcmContentStorage* cs, u64* out_size, const NcmNcaId* placeholder_id) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdInNcaIdOutU64(&cs->s, 25, placeholder_id, out_size);
}

Result ncmContentStorageRepairInvalidFileAttribute(NcmContentStorage* cs) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _ncmCmdNoIo(&cs->s, 26);
}

Result ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(NcmContentStorage* cs, FsRightsId* out_rights_id, u32* out_key_generation, const NcmNcaId* placeholder_id, const NcmNcaId* cache_content_id) {
    if (hosversionBefore(8,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        NcmNcaId cache_content_id;
        NcmNcaId placeholder_id;
    } in = { *cache_content_id, *placeholder_id };
    struct {
        FsRightsId rights_id;
        u32 key_generation;
    } out;
    Result rc = serviceDispatchInOut(&cs->s, 27, in, out);
    if (R_SUCCEEDED(rc)) {
        if (out_rights_id) *out_rights_id = out.rights_id;
        if (out_key_generation) *out_key_generation = out.key_generation;
    }
    return rc;
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

Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmNcaId* out_content_id, const NcmContentMetaKey* key, NcmContentType type) {
    const struct {
        u32 type;
        u32 padding;
        NcmContentMetaKey key;
    } in = { type, 0, *key };
    return serviceDispatchInOut(&db->s, 3, in, *out_content_id);
}

Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, u32* out_entries_written, NcmContentInfo* out_info, size_t out_info_size, const NcmContentMetaKey* key, u32 start_index) {
    const struct {
        u32 start_index;
        NcmContentMetaKey key;
    } in = { start_index, *key };
    return serviceDispatchInOut(&db->s, 4, in, *out_entries_written,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_info, out_info_size } },
    );
}
Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, u32* out_entries_total, u32* out_entries_written, NcmContentMetaKey* out_keys, size_t out_keys_size, NcmContentMetaType meta_type, u64 application_title_id, u64 title_id_min, u64 title_id_max, NcmContentInstallType install_type) {
    const struct {
        u8 meta_type;
        u8 install_type;
        u16 padding;
        u64 application_title_id;
        u64 title_id_min;
        u64 title_id_max;
    } in = { meta_type, install_type, 0, application_title_id, title_id_min, title_id_max };
    struct {
        u32 out_entries_total;
        u32 out_entries_written;
    } out;
    return serviceDispatchInOut(&db->s, 5, in, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_keys, out_keys_size } },
    );
}

Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, NcmContentMetaKey* out_key, u64 title_id) {
    return serviceDispatchInOut(&db->s, 6, title_id, *out_key);
}

Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, u32* out_entries_total, u32* out_entries_written, NcmApplicationContentMetaKey* out_keys, size_t out_keys_size, NcmContentMetaType meta_type) {
    struct {
        u32 out_entries_total;
        u32 out_entries_written;
    } out;
    return serviceDispatchInOut(&db->s, 7, meta_type, out, 
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_keys, out_keys_size } },
    );
}

Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 8, *key, *out);
}

Result ncmContentMetaDatabaseHasAll(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* keys, size_t keys_size) {
    return serviceDispatchOut(&db->s, 9, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { keys, keys_size } },
    );
}

Result ncmContentMetaDatabaseGetSize(NcmContentMetaDatabase* db, u64* out_size, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 10, *key, *out_size);
}

Result ncmContentMetaDatabaseGetRequiredSystemVersion(NcmContentMetaDatabase* db, u64* out_version, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 11, *key, *out_version);
}

Result ncmContentMetaDatabaseGetPatchId(NcmContentMetaDatabase* db, u64* out_patch_id, const NcmContentMetaKey* key) {
    return serviceDispatchInOut(&db->s, 12, *key, *out_patch_id);
}

Result ncmContentMetaDatabaseDisableForcibly(NcmContentMetaDatabase* db) {
    return _ncmCmdNoIo(&db->s, 13);
}

Result ncmContentMetaDatabaseLookupOrphanContent(NcmContentMetaDatabase* db, bool* out_orphaned, size_t out_orphaned_size, const NcmNcaId* content_ids, size_t content_ids_size) {
    return serviceDispatch(&db->s, 14, 
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = { 
            { out_orphaned, out_orphaned_size },
            { content_ids, content_ids_size },
        },
    );
}

Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db) {
    return _ncmCmdNoIo(&db->s, 15);
}

Result ncmContentMetaDatabaseHasContent(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key, const NcmNcaId* content_id) {
    const struct {
        NcmNcaId content_id;
        NcmContentMetaKey key;
    } in = { *content_id, *key };
    return serviceDispatchInOut(&db->s, 16, in, *out);
}

Result ncmContentMetaDatabaseListContentMetaInfo(NcmContentMetaDatabase* db, u32* out_entries_written, void* out_meta_info, size_t out_meta_info_size, const NcmContentMetaKey* key, u32 start_index) {
    const struct {
        u32 start_index;
        u32 padding;
        NcmContentMetaKey key;
    } in = { start_index, 0, *key };
    return serviceDispatchInOut(&db->s, 17, in, *out_entries_written,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_meta_info, out_meta_info_size } },
    );
}

Result ncmContentMetaDatabaseGetAttributes(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u8* out) {
    return serviceDispatchInOut(&db->s, 18, *key, *out);
}

Result ncmContentMetaDatabaseGetRequiredApplicationVersion(NcmContentMetaDatabase* db, u64* out_version, const NcmContentMetaKey* key) {
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchInOut(&db->s, 19, *key, *out_version);
}

Result ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(NcmContentMetaDatabase* db, NcmNcaId* out_content_id, const NcmContentMetaKey* key, NcmContentType type, u8 id_offset) {
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        u32 type;
        u8 id_offset;
        u8 padding[3];
        NcmContentMetaKey key;
    } in = { type, id_offset, {0}, *key };
    return serviceDispatchInOut(&db->s, 20, in, *out_content_id);
}
