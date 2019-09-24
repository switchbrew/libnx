/**
 * @file ncm.h
 * @brief Content Manager (ncm) service IPC wrapper.
 * @author Adubbz & zhuowei
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/fs.h"
#include "../services/sm.h"

typedef struct {
    Service s;
} NcmContentStorage;

typedef struct {
    Service s;
} NcmContentMetaDatabase;

typedef enum  {
    NcmContentType_Meta             = 0,
    NcmContentType_Program          = 1,
    NcmContentType_Data             = 2,
    NcmContentType_Control          = 3,
    NcmContentType_HtmlDocument     = 4,
    NcmContentType_LegalInformation = 5,
    NcmContentType_DeltaFragment    = 6,
} NcmContentType;

typedef enum {
    NcmContentMetaType_Unknown              = 0x0,
    NcmContentMetaType_SystemProgram        = 0x1,
    NcmContentMetaType_SystemData           = 0x2,
    NcmContentMetaType_SystemUpdate         = 0x3,
    NcmContentMetaType_BootImagePackage     = 0x4,
    NcmContentMetaType_BootImagePackageSafe = 0x5,
    NcmContentMetaType_Application          = 0x80,
    NcmContentMetaType_Patch                = 0x81,
    NcmContentMetaType_AddOnContent         = 0x82,
    NcmContentMetaType_Delta                = 0x83,
} NcmContentMetaType;

typedef enum {
    NcmContentMetaAttribute_None                = 0,
    NcmContentMetaAttribute_IncludesExFatDriver = BIT(0),
    NcmContentMetaAttribute_Rebootless          = BIT(1),
} NcmContentMetaAttribute;

typedef enum {
    NcmContentInstallType_Full         = 0,
    NcmContentInstallType_FragmentOnly = 1,
    NcmContentInstallType_Unknown      = 7,
} NcmContentInstallType;

typedef struct {
    u8 c[0x10];
} NcmNcaId;

typedef struct {
    u64 id;
    u32 version;
    NcmContentMetaType type;
    NcmContentInstallType install_type;
    u8 padding[2];
} NcmContentMetaKey;

typedef struct {
    NcmContentMetaKey key;
    u64 application_id;
} NcmApplicationContentMetaKey;

typedef struct {
    NcmNcaId content_id;
    u8 size[0x6];
    NcmContentType content_type;
    u8 id_offset;
} NcmContentInfo;

/* Used by system updates. They share the exact same struct as NcmContentMetaKey */
typedef NcmContentMetaKey NcmContentMetaInfo;

typedef struct {
    u16 extended_header_size;           ///< Size of optional struct that comes after this one.
    u16 content_count;                  ///< Number of NcmContentInfos after the extra bytes.
    u16 content_meta_count;             ///< Number of NcmContentMetaInfos that come after the NcmContentInfos.
    NcmContentMetaAttribute attributes; ///< Usually None (0).
    FsStorageId storage_id;             ///< Usually None (0).
} NcmContentMetaHeader;

typedef struct {
    u64 patch_id;
    u32 required_system_version;
    u32 padding;
} NcmApplicationMetaExtendedHeader;

typedef struct {
    u64 application_id;
    u32 required_system_version;
    u32 extended_data_size;
    u8 reserved[0x8];
} NcmPatchMetaExtendedHeader;

typedef struct {
    u64 application_id;
    u32 required_application_version;
    u32 padding;
} NcmAddOnContentMetaExtendedHeader;

typedef struct {
    u32 extended_data_size;
} NcmSystemUpdateMetaExtendedHeader;

Result ncmInitialize(void);
void ncmExit(void);
Service* ncmGetServiceSession(void);

Result ncmCreateContentStorage(FsStorageId storage_id);
Result ncmCreateContentMetaDatabase(FsStorageId storage_id);
Result ncmVerifyContentStorage(FsStorageId storage_id);
Result ncmVerifyContentMetaDatabase(FsStorageId storage_id);
Result ncmOpenContentStorage(NcmContentStorage* out_content_storage, FsStorageId storage_id);
Result ncmOpenContentMetaDatabase(NcmContentMetaDatabase* out_content_meta_database, FsStorageId storage_id);
Result ncmCloseContentStorageForcibly(FsStorageId storage_id);
Result ncmCloseContentMetaDatabaseForcibly(FsStorageId storage_id);
Result ncmCleanupContentMetaDatabase(FsStorageId storage_id);
Result ncmActivateContentStorage(FsStorageId storage_id);
Result ncmInactivateContentStorage(FsStorageId storage_id);
Result ncmActivateContentMetaDatabase(FsStorageId storage_id);
Result ncmInactivateContentMetaDatabase(FsStorageId storage_id);
Result ncmInvalidateRightsIdCache(void);

Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmNcaId* out_id);
Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmNcaId* content_id, const NcmNcaId* placeholder_id, u64 size);
Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholder_id);
Result ncmContentStorageHasPlaceHolder(NcmContentStorage* cs, bool* out, const NcmNcaId* placeholder_id);
Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholder_id, u64 offset, const void* data, size_t data_size);
Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmNcaId* content_id, const NcmNcaId* placeholder_id);
Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmNcaId* content_id);
Result ncmContentStorageHas(NcmContentStorage* cs, bool* out, const NcmNcaId* content_id);
Result ncmContentStorageGetPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmNcaId* content_id);
Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, const char* out_path, size_t out_size, NcmNcaId* placeholder_id);
Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs);
Result ncmContentStorageListPlaceHolder(NcmContentStorage* cs, NcmNcaId* out_ids, size_t out_ids_size, u32* out_count);
Result ncmContentStorageGetContentCount(NcmContentStorage* cs, u32* out_count);
Result ncmContentStorageListContentId(NcmContentStorage* cs, NcmNcaId* out_ids, size_t out_ids_size, u32* out_count, u32 start_offset);
Result ncmContentStorageGetSizeFromContentId(NcmContentStorage* cs, u64* out_size, const NcmNcaId* content_id);
Result ncmContentStorageDisableForcibly(NcmContentStorage* cs);
Result ncmContentStorageRevertToPlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholder_id, const NcmNcaId* old_content_id, const NcmNcaId* new_content_id);
Result ncmContentStorageSetPlaceHolderSize(NcmContentStorage* cs, const NcmNcaId* placeholder_id, u64 size);
Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, void* out_data, size_t out_data_size, const NcmNcaId* content_id, u64 offset);
Result ncmContentStorageGetRightsIdFromPlaceHolderId(NcmContentStorage* cs, FsRightsId* out_rights_id, u32* out_key_generation, const NcmNcaId* placeholder_id);
Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, FsRightsId* out_rights_id, u32* out_key_generation, const NcmNcaId* content_id);
Result ncmContentStorageWriteContentForDebug(NcmContentStorage* cs, const NcmNcaId* content_id, u64 offset, const void* data, size_t data_size);
Result ncmContentStorageGetFreeSpaceSize(NcmContentStorage* cs, u64* out_size);
Result ncmContentStorageGetTotalSpaceSize(NcmContentStorage* cs, u64* out_size);
Result ncmContentStorageFlushPlaceHolder(NcmContentStorage* cs);
Result ncmContentStorageGetSizeFromPlaceHolderId(NcmContentStorage* cs, u64* out_size, const NcmNcaId* placeholder_id);
Result ncmContentStorageRepairInvalidFileAttribute(NcmContentStorage* cs);
Result ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(NcmContentStorage* cs, FsRightsId* out_rights_id, u32* out_key_generation, const NcmNcaId* placeholder_id, const NcmNcaId* cache_content_id);

Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, const void* data, u64 data_size);
Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u64* out_size, void* out_data, u64 out_data_size);
Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmContentMetaKey *key);
Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmNcaId* out_content_id, const NcmContentMetaKey* key, NcmContentType type);
Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, u32* out_entries_written, NcmContentInfo* out_info, size_t out_info_size, const NcmContentMetaKey* key, u32 start_index);
Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, u32* out_entries_total, u32* out_entries_written, NcmContentMetaKey* out_keys, size_t out_keys_size, NcmContentMetaType meta_type, u64 application_title_id, u64 title_id_min, u64 title_id_max, NcmContentInstallType install_type);
Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, NcmContentMetaKey* out_key, u64 title_id);
Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, u32* out_entries_total, u32* out_entries_written, NcmApplicationContentMetaKey* out_keys, size_t out_keys_size, NcmContentMetaType meta_type);
Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseHasAll(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* keys, size_t keys_size);
Result ncmContentMetaDatabaseGetSize(NcmContentMetaDatabase* db, u64* out_size, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseGetRequiredSystemVersion(NcmContentMetaDatabase* db, u64* out_version, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseGetPatchId(NcmContentMetaDatabase* db, u64* out_patch_id, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseDisableForcibly(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseLookupOrphanContent(NcmContentMetaDatabase* db, bool* out_orphaned, size_t out_orphaned_size, const NcmNcaId* content_ids, size_t content_ids_size);
Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseHasContent(NcmContentMetaDatabase* db, bool* out, const NcmContentMetaKey* key, const NcmNcaId* content_id);
Result ncmContentMetaDatabaseListContentMetaInfo(NcmContentMetaDatabase* db, u32* out_entries_written, void* out_meta_info, size_t out_meta_info_size, const NcmContentMetaKey* key, u32 start_index);
Result ncmContentMetaDatabaseGetAttributes(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u8* out);
Result ncmContentMetaDatabaseGetRequiredApplicationVersion(NcmContentMetaDatabase* db, u64* out_version, const NcmContentMetaKey* key);
Result ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(NcmContentMetaDatabase* db, NcmNcaId* out_content_id, const NcmContentMetaKey* key, NcmContentType type, u8 id_offset);