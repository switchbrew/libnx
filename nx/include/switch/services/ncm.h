/**
 * @file ncm.h
 * @brief Content Manager (ncm) service IPC wrapper.
 * @author Adubbz & zhuowei
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/ncm_types.h"
#include "../services/fs.h"
#include "../sf/service.h"

/// ContentStorage
typedef struct {
    Service s; ///< IContentStorage
} NcmContentStorage;

/// ContentMetaDatabase
typedef struct {
    Service s; ///< IContentMetaDatabase
} NcmContentMetaDatabase;

/// RightsId
typedef struct {
    FsRightsId rights_id;
    u8 key_generation;      ///< [3.0.0+]
    u8 pad[7];              ///< [3.0.0+]
} NcmRightsId;

/// Initialize ncm.
Result ncmInitialize(void);

/// Exit ncm.
void ncmExit(void);

/// Gets the Service object for the actual ncm service session.
Service* ncmGetServiceSession(void);

Result ncmCreateContentStorage(NcmStorageId storage_id);
Result ncmCreateContentMetaDatabase(NcmStorageId storage_id);
Result ncmVerifyContentStorage(NcmStorageId storage_id);
Result ncmVerifyContentMetaDatabase(NcmStorageId storage_id);
Result ncmOpenContentStorage(NcmContentStorage* out_content_storage, NcmStorageId storage_id);
Result ncmOpenContentMetaDatabase(NcmContentMetaDatabase* out_content_meta_database, NcmStorageId storage_id);
Result ncmCloseContentStorageForcibly(NcmStorageId storage_id); ///< [1.0.0]
Result ncmCloseContentMetaDatabaseForcibly(NcmStorageId storage_id); ///< [1.0.0]
Result ncmCleanupContentMetaDatabase(NcmStorageId storage_id);
Result ncmActivateContentStorage(NcmStorageId storage_id); ///< [2.0.0+]
Result ncmInactivateContentStorage(NcmStorageId storage_id); ///< [2.0.0+]
Result ncmActivateContentMetaDatabase(NcmStorageId storage_id); ///< [2.0.0+]
Result ncmInactivateContentMetaDatabase(NcmStorageId storage_id); ///< [2.0.0+]
Result ncmInvalidateRightsIdCache(void); ///< [9.0.0+]

void ncmContentStorageClose(NcmContentStorage* cs);
Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmPlaceHolderId* out_id);
Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmContentId* content_id, const NcmPlaceHolderId* placeholder_id, s64 size);
Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id);
Result ncmContentStorageHasPlaceHolder(NcmContentStorage* cs, bool* out, const NcmPlaceHolderId* placeholder_id);
Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id, u64 offset, const void* data, size_t data_size);
Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmContentId* content_id, const NcmPlaceHolderId* placeholder_id);
Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmContentId* content_id);
Result ncmContentStorageHas(NcmContentStorage* cs, bool* out, const NcmContentId* content_id);
Result ncmContentStorageGetPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmContentId* content_id);
Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmPlaceHolderId* placeholder_id);
Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs);
Result ncmContentStorageListPlaceHolder(NcmContentStorage* cs, NcmPlaceHolderId* out_ids, s32 count, s32* out_count);
Result ncmContentStorageGetContentCount(NcmContentStorage* cs, s32* out_count);
Result ncmContentStorageListContentId(NcmContentStorage* cs, NcmContentId* out_ids, s32 count, s32* out_count, s32 start_offset);
Result ncmContentStorageGetSizeFromContentId(NcmContentStorage* cs, s64* out_size, const NcmContentId* content_id);
Result ncmContentStorageDisableForcibly(NcmContentStorage* cs);
Result ncmContentStorageRevertToPlaceHolder(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id, const NcmContentId* old_content_id, const NcmContentId* new_content_id); ///< [2.0.0+]
Result ncmContentStorageSetPlaceHolderSize(NcmContentStorage* cs, const NcmPlaceHolderId* placeholder_id, s64 size); ///< [2.0.0+]
Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, void* out_data, size_t out_data_size, const NcmContentId* content_id, s64 offset); ///< [2.0.0+]
Result ncmContentStorageGetRightsIdFromPlaceHolderId(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmPlaceHolderId* placeholder_id); ///< [2.0.0+]
Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmContentId* content_id); ///< [2.0.0+]
Result ncmContentStorageWriteContentForDebug(NcmContentStorage* cs, const NcmContentId* content_id, s64 offset, const void* data, size_t data_size); ///< [2.0.0+]
Result ncmContentStorageGetFreeSpaceSize(NcmContentStorage* cs, s64* out_size); ///< [2.0.0+]
Result ncmContentStorageGetTotalSpaceSize(NcmContentStorage* cs, s64* out_size); ///< [2.0.0+]
Result ncmContentStorageFlushPlaceHolder(NcmContentStorage* cs); ///< [3.0.0+]
Result ncmContentStorageGetSizeFromPlaceHolderId(NcmContentStorage* cs, s64* out_size, const NcmPlaceHolderId* placeholder_id); ///< [4.0.0+]
Result ncmContentStorageRepairInvalidFileAttribute(NcmContentStorage* cs); ///< [4.0.0+]
Result ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmPlaceHolderId* placeholder_id, const NcmContentId* cache_content_id); ///< [8.0.0+]

void ncmContentMetaDatabaseClose(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, const void* data, u64 data_size);
Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u64* out_size, void* out_data, u64 out_data_size);
Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmContentMetaKey *key);
Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmContentId* out_content_id, const NcmContentMetaKey* key, NcmContentType type);
Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, s32* out_entries_written, NcmContentInfo* out_info, s32 count, const NcmContentMetaKey* key, s32 start_index);
Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, s32* out_entries_total, s32* out_entries_written, NcmContentMetaKey* out_keys, s32 count, NcmContentMetaType meta_type, u64 id, u64 id_min, u64 id_max, NcmContentInstallType install_type);
Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, NcmContentMetaKey* out_key, u64 id);
Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, s32* out_entries_total, s32* out_entries_written, NcmApplicationContentMetaKey* out_keys, s32 count, NcmContentMetaType meta_type);
Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseHasAll(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* keys, s32 count);
Result ncmContentMetaDatabaseGetSize(NcmContentMetaDatabase* db, u64* out_size, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseGetRequiredSystemVersion(NcmContentMetaDatabase* db, u32* out_version, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseGetPatchId(NcmContentMetaDatabase* db, u64* out_patch_id, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseDisableForcibly(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseLookupOrphanContent(NcmContentMetaDatabase* db, bool* out_orphaned, const NcmContentId* content_ids, s32 count);
Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseHasContent(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key, const NcmContentId* content_id);
Result ncmContentMetaDatabaseListContentMetaInfo(NcmContentMetaDatabase* db, s32* out_entries_written, void* out_meta_info, s32 count, const NcmContentMetaKey* key, s32 start_index);
Result ncmContentMetaDatabaseGetAttributes(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u8* out);
Result ncmContentMetaDatabaseGetRequiredApplicationVersion(NcmContentMetaDatabase* db, u32* out_version, const NcmContentMetaKey* key); ///< [2.0.0+]
Result ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(NcmContentMetaDatabase* db, NcmContentId* out_content_id, const NcmContentMetaKey* key, NcmContentType type, u8 id_offset); ///< [5.0.0+]
