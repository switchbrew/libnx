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

/// ContentStorage
typedef struct {
    Service s; ///< IContentStorage
} NcmContentStorage;

/// ContentMetaDatabase
typedef struct {
    Service s; ///< IContentMetaDatabase
} NcmContentMetaDatabase;

/// ContentType
typedef enum  {
    NcmContentType_Meta             = 0, ///< Meta
    NcmContentType_Program          = 1, ///< Program
    NcmContentType_Data             = 2, ///< Data
    NcmContentType_Control          = 3, ///< Control
    NcmContentType_HtmlDocument     = 4, ///< HtmlDocument
    NcmContentType_LegalInformation = 5, ///< LegalInformation
    NcmContentType_DeltaFragment    = 6, ///< DeltaFragment
} NcmContentType;

/// ContentMetaType
typedef enum {
    NcmContentMetaType_Unknown              = 0x0,  ///< Unknown
    NcmContentMetaType_SystemProgram        = 0x1,  ///< SystemProgram
    NcmContentMetaType_SystemData           = 0x2,  ///< SystemData
    NcmContentMetaType_SystemUpdate         = 0x3,  ///< SystemUpdate
    NcmContentMetaType_BootImagePackage     = 0x4,  ///< BootImagePackage
    NcmContentMetaType_BootImagePackageSafe = 0x5,  ///< BootImagePackageSafe
    NcmContentMetaType_Application          = 0x80, ///< Application
    NcmContentMetaType_Patch                = 0x81, ///< Patch
    NcmContentMetaType_AddOnContent         = 0x82, ///< AddOnContent
    NcmContentMetaType_Delta                = 0x83, ///< Delta
} NcmContentMetaType;

/// ContentMetaAttribute
typedef enum {
    NcmContentMetaAttribute_None                = 0,      ///< None
    NcmContentMetaAttribute_IncludesExFatDriver = BIT(0), ///< IncludesExFatDriver
    NcmContentMetaAttribute_Rebootless          = BIT(1), ///< Rebootless
} NcmContentMetaAttribute;

/// ContentInstallType
typedef enum {
    NcmContentInstallType_Full         = 0, ///< Full
    NcmContentInstallType_FragmentOnly = 1, ///< FragmentOnly
    NcmContentInstallType_Unknown      = 7, ///< Unknown
} NcmContentInstallType;

/// ContentId
typedef struct {
    u8 c[0x10]; ///< Id
} NcmContentId;

/// ContentMetaKey
typedef struct {
    u64 title_id;                       ///< Title id.
    u32 version;                        ///< Title version.
    u8 type;                            ///< \ref NcmContentMetaType
    u8 install_type;                    ///< \ref NcmContentInstallType
    u8 padding[2];                      ///< Padding.
} NcmContentMetaKey;

/// ApplicationContentMetaKey
typedef struct {
    NcmContentMetaKey key; ///< \ref NcmContentMetaKey
    u64 application_id;    ///< Title id of an application.
} NcmApplicationContentMetaKey;

/// ContentInfo
typedef struct {
    NcmContentId content_id;         ///< \ref NcmContentId
    u8 size[0x6];                ///< Content size.
    u8 content_type;             ///< \ref NcmContentType.
    u8 id_offset;                ///< Offset of this content. Unused by most applications.
} NcmContentInfo;

/// Used by system updates. They share the exact same struct as NcmContentMetaKey
typedef NcmContentMetaKey NcmContentMetaInfo;

/// ContentMetaHeader
typedef struct {
    u16 extended_header_size;           ///< Size of optional struct that comes after this one.
    u16 content_count;                  ///< Number of NcmContentInfos after the extra bytes.
    u16 content_meta_count;             ///< Number of NcmContentMetaInfos that come after the NcmContentInfos.
    u8 attributes;                      ///< Usually None (0).
    u8 storage_id;                      ///< Usually None (0).
} NcmContentMetaHeader;

/// ApplicationMetaExtendedHeader
typedef struct {
    u64 patch_id;                     ///< Title id of this application's patch.
    u32 required_system_version;      ///< Firmware version required by this application.
    u32 required_application_version; ///< [9.0.0+] Owner application version required by this application. Previously padding.
} NcmApplicationMetaExtendedHeader;

/// PatchMetaExtendedHeader
typedef struct {
    u64 application_id;          ///< Title id of this patch's corresponding application.
    u32 required_system_version; ///< Firmware version required by this patch.
    u32 extended_data_size;      ///< Size of the extended data following the NcmContentInfos.
    u8 reserved[0x8];            ///< Unused.
} NcmPatchMetaExtendedHeader;

/// AddOnContentMetaExtendedHeader
typedef struct {
    u64 application_id;               ///< Title id of this add-on-content's corresponding application.
    u32 required_application_version; ///< Version of the application required by this add-on-content.
    u32 padding;                      ///< Padding.
} NcmAddOnContentMetaExtendedHeader;

/// SystemUpdateMetaExtendedHeader
typedef struct {
    u32 extended_data_size; ///< Size of the extended data after NcmContentInfos and NcmContentMetaInfos.
} NcmSystemUpdateMetaExtendedHeader;

/// RightsId
typedef struct {
    FsRightsId rights_id;
    u8 key_generation;      ///< [3.0.0+]
    u8 pad[7];              ///< [3.0.0+]
} NcmRightsId;

/// ProgramLocation
typedef struct {
    u64 titleID;
    u8 storageID;           ///< \ref FsStorageId
    u8 pad[7];
} NcmProgramLocation;

Result ncmInitialize(void);
void ncmExit(void);
Service* ncmGetServiceSession(void);

Result ncmCreateContentStorage(FsStorageId storage_id);
Result ncmCreateContentMetaDatabase(FsStorageId storage_id);
Result ncmVerifyContentStorage(FsStorageId storage_id);
Result ncmVerifyContentMetaDatabase(FsStorageId storage_id);
Result ncmOpenContentStorage(NcmContentStorage* out_content_storage, FsStorageId storage_id);
Result ncmOpenContentMetaDatabase(NcmContentMetaDatabase* out_content_meta_database, FsStorageId storage_id);
Result ncmCloseContentStorageForcibly(FsStorageId storage_id); ///< [1.0.0]
Result ncmCloseContentMetaDatabaseForcibly(FsStorageId storage_id); ///< [1.0.0]
Result ncmCleanupContentMetaDatabase(FsStorageId storage_id);
Result ncmActivateContentStorage(FsStorageId storage_id); ///< [2.0.0+]
Result ncmInactivateContentStorage(FsStorageId storage_id); ///< [2.0.0+]
Result ncmActivateContentMetaDatabase(FsStorageId storage_id); ///< [2.0.0+]
Result ncmInactivateContentMetaDatabase(FsStorageId storage_id); ///< [2.0.0+]
Result ncmInvalidateRightsIdCache(void); ///< [9.0.0+]

void ncmContentStorageClose(NcmContentStorage* cs);
Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmContentId* out_id);
Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmContentId* content_id, const NcmContentId* placeholder_id, u64 size);
Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmContentId* placeholder_id);
Result ncmContentStorageHasPlaceHolder(NcmContentStorage* cs, bool* out, const NcmContentId* placeholder_id);
Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmContentId* placeholder_id, u64 offset, const void* data, size_t data_size);
Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmContentId* content_id, const NcmContentId* placeholder_id);
Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmContentId* content_id);
Result ncmContentStorageHas(NcmContentStorage* cs, bool* out, const NcmContentId* content_id);
Result ncmContentStorageGetPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmContentId* content_id);
Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, char* out_path, size_t out_size, const NcmContentId* placeholder_id);
Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs);
Result ncmContentStorageListPlaceHolder(NcmContentStorage* cs, NcmContentId* out_ids, s32 count, s32* out_count);
Result ncmContentStorageGetContentCount(NcmContentStorage* cs, s32* out_count);
Result ncmContentStorageListContentId(NcmContentStorage* cs, NcmContentId* out_ids, s32 count, s32* out_count, s32 start_offset);
Result ncmContentStorageGetSizeFromContentId(NcmContentStorage* cs, s64* out_size, const NcmContentId* content_id);
Result ncmContentStorageDisableForcibly(NcmContentStorage* cs);
Result ncmContentStorageRevertToPlaceHolder(NcmContentStorage* cs, const NcmContentId* placeholder_id, const NcmContentId* old_content_id, const NcmContentId* new_content_id); ///< [2.0.0+]
Result ncmContentStorageSetPlaceHolderSize(NcmContentStorage* cs, const NcmContentId* placeholder_id, s64 size); ///< [2.0.0+]
Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, void* out_data, size_t out_data_size, const NcmContentId* content_id, s64 offset); ///< [2.0.0+]
Result ncmContentStorageGetRightsIdFromPlaceHolderId(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmContentId* placeholder_id); ///< [2.0.0+]
Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmContentId* content_id); ///< [2.0.0+]
Result ncmContentStorageWriteContentForDebug(NcmContentStorage* cs, const NcmContentId* content_id, s64 offset, const void* data, size_t data_size); ///< [2.0.0+]
Result ncmContentStorageGetFreeSpaceSize(NcmContentStorage* cs, s64* out_size); ///< [2.0.0+]
Result ncmContentStorageGetTotalSpaceSize(NcmContentStorage* cs, s64* out_size); ///< [2.0.0+]
Result ncmContentStorageFlushPlaceHolder(NcmContentStorage* cs); ///< [3.0.0+]
Result ncmContentStorageGetSizeFromPlaceHolderId(NcmContentStorage* cs, s64* out_size, const NcmContentId* placeholder_id); ///< [4.0.0+]
Result ncmContentStorageRepairInvalidFileAttribute(NcmContentStorage* cs); ///< [4.0.0+]
Result ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(NcmContentStorage* cs, NcmRightsId* out_rights_id, const NcmContentId* placeholder_id, const NcmContentId* cache_content_id); ///< [8.0.0+]

void ncmContentMetaDatabaseClose(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, const void* data, u64 data_size);
Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmContentMetaKey* key, u64* out_size, void* out_data, u64 out_data_size);
Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmContentMetaKey *key);
Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmContentId* out_content_id, const NcmContentMetaKey* key, NcmContentType type);
Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, s32* out_entries_written, NcmContentInfo* out_info, s32 count, const NcmContentMetaKey* key, s32 start_index);
Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, s32* out_entries_total, s32* out_entries_written, NcmContentMetaKey* out_keys, s32 count, NcmContentMetaType meta_type, u64 application_title_id, u64 title_id_min, u64 title_id_max, NcmContentInstallType install_type);
Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, NcmContentMetaKey* out_key, u64 title_id);
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
