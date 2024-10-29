/**
 * @file fs.h
 * @brief Filesystem (fsp-srv) service IPC wrapper.
 * Normally applications should just use standard stdio not FS-serv directly. However this can be used if obtaining a FsFileSystem, FsFile, or FsStorage, for mounting with fs_dev/romfs_dev, etc.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/ncm_types.h"
#include "../services/acc.h"
#include "../sf/service.h"

// We use wrapped handles for type safety.

#define FS_MAX_PATH 0x301

/// For use with \ref FsSaveDataAttribute.
#define FS_SAVEDATA_CURRENT_APPLICATIONID 0

typedef struct {
    u8 c[0x10];
} FsRightsId;

typedef struct {
    Service s;
} FsFileSystem;

typedef struct {
    Service s;
} FsFile;

typedef struct {
    Service s;
} FsDir;

typedef struct {
    Service s;
} FsStorage;

typedef struct {
    Service s;
} FsSaveDataInfoReader;

typedef struct {
    Service s;
} FsEventNotifier;

typedef struct {
    Service s;
} FsDeviceOperator;

/// Directory entry.
typedef struct {
    char name[FS_MAX_PATH];         ///< Entry name.
    u8 pad[3];
    s8 type;                        ///< See FsDirEntryType.
    u8 pad2[3];                     ///< ?
    s64 file_size;                  ///< File size.
} FsDirectoryEntry;

/// SaveDataAttribute
typedef struct {
    u64 application_id;             ///< ApplicationId of the savedata to access when accessing other programs' savedata via SaveData, otherwise FS_SAVEDATA_CURRENT_APPLICATIONID.
    AccountUid uid;                 ///< \ref AccountUid for the user-specific savedata to access, otherwise 0 for common savedata.
    u64 system_save_data_id;        ///< SystemSaveDataId, 0 for ::FsSaveDataType_Account.
    u8 save_data_type;              ///< \ref FsSaveDataType
    u8 save_data_rank;              ///< \ref FsSaveDataRank
    u16 save_data_index;            ///< SaveDataIndex
    u32 pad_x24;                    ///< Padding.
    u64 unk_x28;                    ///< 0 for ::FsSaveDataType_System/::FsSaveDataType_Account.
    u64 unk_x30;                    ///< 0 for ::FsSaveDataType_System/::FsSaveDataType_Account.
    u64 unk_x38;                    ///< 0 for ::FsSaveDataType_System/::FsSaveDataType_Account.
} FsSaveDataAttribute;

/// SaveDataExtraData
typedef struct {
    FsSaveDataAttribute attr;       ///< \ref FsSaveDataAttribute
    u64 owner_id;                   ///< ProgramId of the owner of this save data. 0 for ::FsSaveDataType_System.
    u64 timestamp;                  ///< POSIX timestamp.
    u32 flags;                      ///< \ref FsSaveDataFlags
    u32 unk_x54;                    ///< Normally 0. Possibly unused?
    s64 data_size;                  ///< Usable save data size.
    s64 journal_size;               ///< Journal size of the save data.
    u64 commit_id;                  ///< Id of the latest commit.
    u8 unused[0x190];               ///< Uninitialized.
} FsSaveDataExtraData;

/// SaveDataMetaInfo
typedef struct {
    u32 size;
    u8 type;                        ///< \ref FsSaveDataMetaType
    u8 reserved[0x0B];
} FsSaveDataMetaInfo;

/// SaveDataCreationInfo
typedef struct {
    s64 save_data_size;    ///< Size of the save data.
    s64 journal_size;      ///< Journal size of the save data.
    u64 available_size;    ///< AvailableSize
    u64 owner_id;          ///< ProgramId of the owner of this save data. 0 for ::FsSaveDataType_System.
    u32 flags;             ///< \ref FsSaveDataFlags
    u8 save_data_space_id; ///< \ref FsSaveDataSpaceId
    u8 unk;                ///< 0 for ::FsSaveDataType_System.
    u8 padding[0x1a];      ///< Uninitialized for ::FsSaveDataType_System.
} FsSaveDataCreationInfo;

/// SaveDataInfo
typedef struct {
    u64 save_data_id;         ///< SaveDataId
    u8 save_data_space_id;    ///< \ref FsSaveDataSpaceId
    u8 save_data_type;        ///< \ref FsSaveDataType
    u8 pad[6];                ///< Padding.
    AccountUid uid;           ///< FsSave::userID
    u64 system_save_data_id;  ///< FsSaveDataAttribute::system_save_data_id
    u64 application_id;       ///< ApplicationId for ::FsSaveDataType_Account.
    u64 size;                 ///< Raw saveimage size.
    u16 save_data_index;      ///< SaveDataIndex
    u8 save_data_rank;        ///< \ref FsSaveDataRank
    u8 unk_x3b[0x25];         ///< Unknown. Usually zeros?
} FsSaveDataInfo;

/// SaveDataFilter
typedef struct {
    bool filter_by_application_id;       ///< Filter by \ref FsSaveDataAttribute::application_id
    bool filter_by_save_data_type;       ///< Filter by \ref FsSaveDataAttribute::save_data_type
    bool filter_by_user_id;              ///< Filter by \ref FsSaveDataAttribute::uid
    bool filter_by_system_save_data_id;  ///< Filter by \ref FsSaveDataAttribute::system_save_data_id
    bool filter_by_index;                ///< Filter by \ref FsSaveDataAttribute::save_data_index
    u8 save_data_rank;                   ///< \ref FsSaveDataRank
    u8 padding[0x2];                     ///< Padding
    FsSaveDataAttribute attr;            ///< \ref FsSaveDataAttribute
} FsSaveDataFilter;

typedef struct {
    u64 created;  ///< POSIX timestamp.
    u64 modified; ///< POSIX timestamp.
    u64 accessed; ///< POSIX timestamp.
    u8 is_valid;  ///< 0x1 when the timestamps are set.
    u8 padding[7];
} FsTimeStampRaw;

/// This is nn::fssystem::ArchiveMacKey. Used by \ref setsysGetThemeKey and \ref setsysSetThemeKey. Does not appear to be in use elsewhere.
typedef struct {
    u8 key[0x10];
} FsArchiveMacKey;

/// Returned by fsFsGetEntryType.
typedef enum {
    FsDirEntryType_Dir  = 0, ///< Entry is a directory.
    FsDirEntryType_File = 1, ///< Entry is a file.
} FsDirEntryType;

/// For use with fsFsOpenFile.
typedef enum {
    FsOpenMode_Read   = BIT(0), ///< Open for reading.
    FsOpenMode_Write  = BIT(1), ///< Open for writing.
    FsOpenMode_Append = BIT(2), ///< Append file.
} FsOpenMode;

/// For use with fsFsCreateFile.
typedef enum {
    FsCreateOption_BigFile = BIT(0), ///< Creates a ConcatenationFile (dir with archive bit) instead of file.
} FsCreateOption;

/// For use with fsFsOpenDirectory.
typedef enum {
    FsDirOpenMode_ReadDirs   = BIT(0),  ///< Enable reading directory entries.
    FsDirOpenMode_ReadFiles  = BIT(1),  ///< Enable reading file entries.
    FsDirOpenMode_NoFileSize = BIT(31), ///< Causes result entries to not contain filesize information (always 0).
} FsDirOpenMode;

/// For use with fsFileRead.
typedef enum {
    FsReadOption_None = 0, ///< No option.
} FsReadOption;

/// For use with fsFileWrite.
typedef enum {
    FsWriteOption_None  = 0,      ///< No option.
    FsWriteOption_Flush = BIT(0), ///< Forces a flush after write.
} FsWriteOption;

typedef enum {
    FsContentStorageId_System  = 0, ///< System
    FsContentStorageId_User    = 1, ///< User
    FsContentStorageId_SdCard  = 2, ///< SdCard
    FsContentStorageId_System0 = 3, ///< [16.0.0+] System0
} FsContentStorageId;

typedef enum {
    FsCustomStorageId_System   = 0,
    FsCustomStorageId_SdCard   = 1,
} FsCustomStorageId;

/// ImageDirectoryId
typedef enum {
    FsImageDirectoryId_Nand = 0,
    FsImageDirectoryId_Sd   = 1,
} FsImageDirectoryId;

/// SaveDataSpaceId
typedef enum {
    FsSaveDataSpaceId_System           = 0,    ///< System
    FsSaveDataSpaceId_User             = 1,    ///< User
    FsSaveDataSpaceId_SdSystem         = 2,    ///< SdSystem
    FsSaveDataSpaceId_Temporary        = 3,    ///< [3.0.0+] Temporary
    FsSaveDataSpaceId_SdUser           = 4,    ///< [4.0.0+] SdUser
    FsSaveDataSpaceId_ProperSystem     = 100,  ///< [3.0.0+] ProperSystem
    FsSaveDataSpaceId_SafeMode         = 101,  ///< [3.0.0+] SafeMode

    FsSaveDataSpaceId_All              = -1,   ///< Pseudo value for fsOpenSaveDataInfoReader().
} FsSaveDataSpaceId;

/// SaveDataType
typedef enum {
    FsSaveDataType_System              = 0, ///< System
    FsSaveDataType_Account             = 1, ///< Account
    FsSaveDataType_Bcat                = 2, ///< Bcat
    FsSaveDataType_Device              = 3, ///< Device
    FsSaveDataType_Temporary           = 4, ///< [3.0.0+] Temporary
    FsSaveDataType_Cache               = 5, ///< [3.0.0+] Cache
    FsSaveDataType_SystemBcat          = 6, ///< [4.0.0+] SystemBcat
} FsSaveDataType;

/// SaveDataRank
typedef enum {
    FsSaveDataRank_Primary   = 0,           ///< Primary
    FsSaveDataRank_Secondary = 1,           ///< Secondary
} FsSaveDataRank;

/// SaveDataFlags
typedef enum {
    FsSaveDataFlags_KeepAfterResettingSystemSaveData                    = BIT(0),
    FsSaveDataFlags_KeepAfterRefurbishment                              = BIT(1),
    FsSaveDataFlags_KeepAfterResettingSystemSaveDataWithoutUserSaveData = BIT(2),
    FsSaveDataFlags_NeedsSecureDelete                                   = BIT(3),
} FsSaveDataFlags;

/// SaveDataMetaType
typedef enum {
    FsSaveDataMetaType_None             = 0,
    FsSaveDataMetaType_Thumbnail        = 1,
    FsSaveDataMetaType_ExtensionContext = 2,
} FsSaveDataMetaType;

typedef enum {
    FsGameCardAttribute_AutoBootFlag                          = BIT(0), ///< Causes the cartridge to automatically start on bootup
    FsGameCardAttribute_HistoryEraseFlag                      = BIT(1), ///< Causes NS to throw an error on attempt to load the cartridge
    FsGameCardAttribute_RepairToolFlag                        = BIT(2), ///< [4.0.0+] Indicates that this gamecard is a repair tool.
    FsGameCardAttribute_DifferentRegionCupToTerraDeviceFlag   = BIT(3), ///< [9.0.0+] DifferentRegionCupToTerraDeviceFlag
    FsGameCardAttribute_DifferentRegionCupToGlobalDeviceFlag  = BIT(4), ///< [9.0.0+] DifferentRegionCupToGlobalDeviceFlag
} FsGameCardAttribute;

typedef enum {
    FsGameCardPartition_Update = 0,
    FsGameCardPartition_Normal = 1,
    FsGameCardPartition_Secure = 2,
    FsGameCardPartition_Logo   = 3,  ///< [4.0.0+]
} FsGameCardPartition;

typedef struct {
    u32 value;
} FsGameCardHandle;

typedef struct {
    u32 version;
    u8 pad[0x4];
    u64 id;
} FsGameCardUpdatePartitionInfo;

typedef struct {
    u32 aes_ctr_key_type;           ///< Contains bitflags describing how data is AES encrypted.
    u32 speed_emulation_type;       ///< Contains bitflags describing how data is emulated.
    u32 reserved[0x38/sizeof(u32)];
} FsRangeInfo;

typedef enum {
    FsOperationId_Clear,           ///< Fill range with zero for supported file/storage.
    FsOperationId_ClearSignature,  ///< Clears signature for supported file/storage.
    FsOperationId_InvalidateCache, ///< Invalidates cache for supported file/storage.
    FsOperationId_QueryRange,      ///< Retrieves information on data for supported file/storage.
} FsOperationId;

/// BisPartitionId
typedef enum {
    FsBisPartitionId_BootPartition1Root              = 0,

    FsBisPartitionId_BootPartition2Root              = 10,

    FsBisPartitionId_UserDataRoot                    = 20,
    FsBisPartitionId_BootConfigAndPackage2Part1      = 21,
    FsBisPartitionId_BootConfigAndPackage2Part2      = 22,
    FsBisPartitionId_BootConfigAndPackage2Part3      = 23,
    FsBisPartitionId_BootConfigAndPackage2Part4      = 24,
    FsBisPartitionId_BootConfigAndPackage2Part5      = 25,
    FsBisPartitionId_BootConfigAndPackage2Part6      = 26,
    FsBisPartitionId_CalibrationBinary               = 27,
    FsBisPartitionId_CalibrationFile                 = 28,
    FsBisPartitionId_SafeMode                        = 29,
    FsBisPartitionId_User                            = 30,
    FsBisPartitionId_System                          = 31,
    FsBisPartitionId_SystemProperEncryption          = 32,
    FsBisPartitionId_SystemProperPartition           = 33,
    FsBisPartitionId_SignedSystemPartitionOnSafeMode = 34,
    FsBisPartitionId_DeviceTreeBlob                  = 35,
    FsBisPartitionId_System0                         = 36,
} FsBisPartitionId;

/// FileSystemType
typedef enum {
    FsFileSystemType_Logo               = 2,  ///< Logo
    FsFileSystemType_ContentControl     = 3,  ///< ContentControl
    FsFileSystemType_ContentManual      = 4,  ///< ContentManual
    FsFileSystemType_ContentMeta        = 5,  ///< ContentMeta
    FsFileSystemType_ContentData        = 6,  ///< ContentData
    FsFileSystemType_ApplicationPackage = 7,  ///< ApplicationPackage
    FsFileSystemType_RegisteredUpdate   = 8,  ///< [4.0.0+] RegisteredUpdate
} FsFileSystemType;

/// FileSystemQueryId
typedef enum {
    FsFileSystemQueryId_SetConcatenationFileAttribute           = 0,  ///< [4.0.0+]
    FsFileSystemQueryId_IsValidSignedSystemPartitionOnSdCard    = 2,  ///< [8.0.0+]
} FsFileSystemQueryId;

/// FileSystemAttribute
typedef struct {
    bool directory_name_length_max_has_value;
    bool file_name_length_max_has_value;
    bool directory_path_length_max_has_value;
    bool file_path_length_max_has_value;
    bool utf16_create_directory_path_length_max_has_value;
    bool utf16_delete_directory_path_length_max_has_value;
    bool utf16_rename_source_directory_path_length_max_has_value;
    bool utf16_rename_destination_directory_path_length_max_has_value;
    bool utf16_open_directory_path_length_max_has_value;
    bool utf16_directory_name_length_max_has_value;
    bool utf16_file_name_length_max_has_value;
    bool utf16_directory_path_length_max_has_value;
    bool utf16_file_path_length_max_has_value;
    u8 reserved1[0x1B];
    s32 directory_name_length_max;
    s32 file_name_length_max;
    s32 directory_path_length_max;
    s32 file_path_length_max;
    s32 utf16_create_directory_path_length_max;
    s32 utf16_delete_directory_path_length_max;
    s32 utf16_rename_source_directory_path_length_max;
    s32 utf16_rename_destination_directory_path_length_max;
    s32 utf16_open_directory_path_length_max;
    s32 utf16_directory_name_length_max;
    s32 utf16_file_name_length_max;
    s32 utf16_directory_path_length_max;
    s32 utf16_file_path_length_max;
    u8 reserved2[0x64];
} FsFileSystemAttribute;

/// FsPriority
typedef enum {
    FsPriority_Normal     = 0,
    FsPriority_Realtime   = 1,
    FsPriority_Low        = 2,
    FsPriority_Background = 3,
} FsPriority;

/// FsContentAttributes
typedef enum {
    FsContentAttributes_None = 0x0,
    FsContentAttributes_All  = 0xF,
} FsContentAttributes;

/// For use with fsOpenHostFileSystemWithOption
typedef enum {
    FsMountHostOptionFlag_None                = 0,      ///< Host filesystem will be case insensitive.
    FsMountHostOptionFlag_PseudoCaseSensitive = BIT(0), ///< Host filesystem will be pseudo case sensitive.
} FsMountHostOption;

/// FsStorageErrorInfo
typedef struct {
    u32 num_activation_failures;
    u32 num_activation_error_corrections;
    u32 num_read_write_failures;
    u32 num_read_write_error_corrections;
} FsStorageErrorInfo;

/// FatFatError
typedef struct {
    s32 error;
    s32 extra_error;
    s32 drive_id;
    char name[16];
    u8 reserved[4];
} FatFatError;

/// FatFatReportInfo1
typedef struct {
    u16 open_file_peak_count;
    u16 open_directory_peak_count;
} FatFatReportInfo1;

/// FatFatReportInfo2
typedef struct {
    u16 open_unique_file_entry_peak_count;
    u16 open_unique_directory_entry_peak_count;
} FatFatReportInfo2;

/// FatFatSafeInfo
typedef struct {
    u32 result;
    u32 error_number;
    u32 safe_error_number;
} FatFatSafeInfo;

/// FsFileSystemProxyErrorInfo
typedef struct {
    u32 rom_fs_remount_for_data_corruption_count;
    u32 rom_fs_unrecoverable_data_corruption_by_remount_count;
    FatFatError fat_fs_error;
    u32 rom_fs_recovered_by_invalidate_cache_count;
    u32 save_data_index_count;
    FatFatReportInfo1 bis_system_fat_report_info_1;
    FatFatReportInfo1 bis_user_fat_report_info_1;
    FatFatReportInfo1 sd_card_fat_report_info_1;
    FatFatReportInfo2 bis_system_fat_report_info_2;
    FatFatReportInfo2 bis_user_fat_report_info_2;
    FatFatReportInfo2 sd_card_fat_report_info_2;
    u32 rom_fs_deep_retry_start_count;
    u32 rom_fs_unrecoverable_by_game_card_access_failed_count;
    FatFatSafeInfo bis_system_fat_safe_info;
    FatFatSafeInfo bis_user_fat_safe_info;

    u8 reserved[0x18];
} FsFileSystemProxyErrorInfo;

/// FsMemoryReportInfo
typedef struct {
    u64 pooled_buffer_peak_free_size;
    u64 pooled_buffer_retried_count;
    u64 pooled_buffer_reduce_allocation_count;
    u64 buffer_manager_peak_free_size;
    u64 buffer_manager_retried_count;
    u64 exp_heap_peak_free_size;
    u64 buffer_pool_peak_free_size;
    u64 patrol_read_allocate_buffer_success_count;
    u64 patrol_read_allocate_buffer_failure_count;
    u64 buffer_manager_peak_total_allocatable_size;
    u64 buffer_pool_max_allocate_size;
    u64 pooled_buffer_failed_ideal_allocation_count_on_async_access;

    u8 reserved[0x20];
} FsMemoryReportInfo;

/// FsGameCardErrorReportInfo
typedef struct {
    u16 game_card_crc_error_num;
    u16 reserved1;
    u16 asic_crc_error_num;
    u16 reserved2;
    u16 refresh_num;
    u16 reserved3;
    u16 retry_limit_out_num;
    u16 timeout_retry_num;
    u16 asic_reinitialize_failure_detail;
    u16 insertion_count;
    u16 removal_count;
    u16 asic_reinitialize_num;
    u32 initialize_count;
    u16 asic_reinitialize_failure_num;
    u16 awaken_failure_num;
    u16 reserved4;
    u16 refresh_succeeded_count;
    u32 last_read_error_page_address;
    u32 last_read_error_page_count;
    u32 awaken_count;
    u32 read_count_from_insert;
    u32 read_count_from_awaken;
    u8  reserved5[8];
} FsGameCardErrorReportInfo;

/// Initialize fsp-srv. Used automatically during app startup.
Result fsInitialize(void);

/// Exit fsp-srv. Used automatically during app exit.
void fsExit(void);

/// Gets the Service object for the actual fsp-srv service session.
Service* fsGetServiceSession(void);

/// [5.0.0+] Configures the \ref FsPriority of all filesystem commands issued within the current thread.
void fsSetPriority(FsPriority prio);

/// Mount requested filesystem type from content file
Result fsOpenFileSystem(FsFileSystem* out, FsFileSystemType fsType, const char* contentPath); ///< same as calling fsOpenFileSystemWithId with 0 as id
Result fsOpenDataFileSystemByCurrentProcess(FsFileSystem *out);
Result fsOpenFileSystemWithPatch(FsFileSystem* out, u64 id, FsFileSystemType fsType); ///< [2.0.0+], like OpenFileSystemWithId but without content path.
Result fsOpenFileSystemWithId(FsFileSystem* out, u64 id, FsFileSystemType fsType, const char* contentPath, FsContentAttributes attr); ///< works on all firmwares, id is ignored on [1.0.0], attr is ignored before [16.0.0]
Result fsOpenDataFileSystemByProgramId(FsFileSystem *out, u64 program_id); ///< [3.0.0+]
Result fsOpenBisFileSystem(FsFileSystem* out, FsBisPartitionId partitionId, const char* string);
Result fsOpenBisStorage(FsStorage* out, FsBisPartitionId partitionId);

/// Do not call this directly, see fs_dev.h.
Result fsOpenSdCardFileSystem(FsFileSystem* out);

Result fsOpenHostFileSystem(FsFileSystem* out, const char *path);
Result fsOpenHostFileSystemWithOption(FsFileSystem* out, const char *path, u32 flags); ///< [9.0.0+]

Result fsDeleteSaveDataFileSystem(u64 application_id);
Result fsCreateSaveDataFileSystem(const FsSaveDataAttribute* attr, const FsSaveDataCreationInfo* creation_info, const FsSaveDataMetaInfo* meta);
Result fsCreateSaveDataFileSystemBySystemSaveDataId(const FsSaveDataAttribute* attr, const FsSaveDataCreationInfo* creation_info);
Result fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId save_data_space_id, u64 saveID); ///< [2.0.0+]
Result fsDeleteSaveDataFileSystemBySaveDataAttribute(FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute* attr); ///< [4.0.0+]

Result fsIsExFatSupported(bool* out);

Result fsOpenGameCardFileSystem(FsFileSystem* out, const FsGameCardHandle* handle, FsGameCardPartition partition);

Result fsExtendSaveDataFileSystem(FsSaveDataSpaceId save_data_space_id, u64 saveID, s64 dataSize, s64 journalSize); ///< [3.0.0+]

Result fsOpenSaveDataFileSystem(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);
Result fsOpenSaveDataFileSystemBySystemSaveDataId(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);
Result fsOpenReadOnlySaveDataFileSystem(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr); ///< [2.0.0+].

Result fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(void* buf, size_t len, FsSaveDataSpaceId save_data_space_id, u64 saveID);
Result fsReadSaveDataFileSystemExtraData(void* buf, size_t len, u64 saveID);
Result fsWriteSaveDataFileSystemExtraData(const void* buf, size_t len, FsSaveDataSpaceId save_data_space_id, u64 saveID);

Result fsOpenSaveDataInfoReader(FsSaveDataInfoReader* out, FsSaveDataSpaceId save_data_space_id);

Result fsOpenSaveDataInfoReaderWithFilter(FsSaveDataInfoReader* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataFilter *save_data_filter); ///< [6.0.0+]

Result fsOpenImageDirectoryFileSystem(FsFileSystem* out, FsImageDirectoryId image_directory_id);
Result fsOpenContentStorageFileSystem(FsFileSystem* out, FsContentStorageId content_storage_id);
Result fsOpenCustomStorageFileSystem(FsFileSystem* out, FsCustomStorageId custom_storage_id); ///< [7.0.0+]

Result fsOpenDataStorageByCurrentProcess(FsStorage* out);
Result fsOpenDataStorageByProgramId(FsStorage *out, u64 program_id); /// <[3.0.0+]
Result fsOpenDataStorageByDataId(FsStorage* out, u64 dataId, NcmStorageId storageId);
Result fsOpenPatchDataStorageByCurrentProcess(FsStorage* out);

Result fsOpenDeviceOperator(FsDeviceOperator* out);
Result fsOpenSdCardDetectionEventNotifier(FsEventNotifier* out);

Result fsIsSignedSystemPartitionOnSdCardValid(bool *out);

Result fsGetProgramId(u64* out, const char *path, FsContentAttributes attr); ///< [17.0.0+]

/// Retrieves the rights id corresponding to the content path. Only available on [2.0.0-15.0.1].
Result fsGetRightsIdByPath(const char* path, FsRightsId* out_rights_id);

/// Retrieves the rights id and key generation corresponding to the content path. Only available on [3.0.0+], attr is ignored before [16.0.0].
Result fsGetRightsIdAndKeyGenerationByPath(const char* path, FsContentAttributes attr, u8* out_key_generation, FsRightsId* out_rights_id);

Result fsGetContentStorageInfoIndex(s32 *out); ///< [19.0.0+]

Result fsDisableAutoSaveDataCreation(void);

Result fsSetGlobalAccessLogMode(u32 mode);
Result fsGetGlobalAccessLogMode(u32* out_mode);
Result fsOutputAccessLogToSdCard(const char *log, size_t size);

Result fsGetAndClearErrorInfo(FsFileSystemProxyErrorInfo *out); ///< [2.0.0+]

Result fsGetAndClearMemoryReportInfo(FsMemoryReportInfo* out); ///< [4.0.0+]

/// Only available on [7.0.0+].
Result fsGetProgramIndexForAccessLog(u32 *out_program_index, u32 *out_program_count);

// Wrapper(s) for fsCreateSaveDataFileSystem.
Result fsCreate_TemporaryStorage(u64 application_id, u64 owner_id, s64 size, u32 flags);

// Wrapper(s) for fsCreateSaveDataFileSystemBySystemSaveDataId.
Result fsCreate_SystemSaveDataWithOwner(FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, AccountUid uid, u64 owner_id, s64 size, s64 journal_size, u32 flags);
Result fsCreate_SystemSaveData(FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, s64 size, s64 journal_size, u32 flags);

/// Wrapper for fsOpenSaveDataFileSystem.
/// See \ref FsSaveDataAttribute for application_id and uid.
Result fsOpen_SaveData(FsFileSystem* out, u64 application_id, AccountUid uid);

/// Wrapper for fsOpenReadOnlySaveDataFileSystem.
/// Only available on [2.0.0+].
/// See \ref FsSaveDataAttribute for application_id and uid.
Result fsOpen_SaveDataReadOnly(FsFileSystem* out, u64 application_id, AccountUid uid);

/// Wrapper for fsOpenSaveDataFileSystem, for opening BcatSaveData.
Result fsOpen_BcatSaveData(FsFileSystem* out, u64 application_id);

/// Wrapper for fsOpenSaveDataFileSystem, for opening DeviceSaveData.
/// See \ref FsSaveDataAttribute for application_id.
Result fsOpen_DeviceSaveData(FsFileSystem* out, u64 application_id);

/// Wrapper for fsOpenSaveDataFileSystem, for opening TemporaryStorage.
/// Only available on [3.0.0+].
Result fsOpen_TemporaryStorage(FsFileSystem* out);

/// Wrapper for fsOpenSaveDataFileSystem, for opening CacheStorage.
/// Only available on [3.0.0+].
/// See \ref FsSaveDataAttribute for application_id.
Result fsOpen_CacheStorage(FsFileSystem* out, u64 application_id, u16 save_data_index);

/// Wrapper for fsOpenSaveDataFileSystemBySystemSaveDataId, for opening SystemSaveData.
/// WARNING: You can brick when writing to SystemSaveData, if the data is corrupted etc.
Result fsOpen_SystemSaveData(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, AccountUid uid);

/// Wrapper for fsOpenSaveDataFileSystemBySystemSaveDataId, for opening SystemBcatSaveData.
/// Only available on [4.0.0+].
Result fsOpen_SystemBcatSaveData(FsFileSystem* out, u64 system_save_data_id);

// IFileSystem
Result fsFsCreateFile(FsFileSystem* fs, const char* path, s64 size, u32 option);
Result fsFsDeleteFile(FsFileSystem* fs, const char* path);
Result fsFsCreateDirectory(FsFileSystem* fs, const char* path);
Result fsFsDeleteDirectory(FsFileSystem* fs, const char* path);
Result fsFsDeleteDirectoryRecursively(FsFileSystem* fs, const char* path);
Result fsFsRenameFile(FsFileSystem* fs, const char* cur_path, const char* new_path);
Result fsFsRenameDirectory(FsFileSystem* fs, const char* cur_path, const char* new_path);
Result fsFsGetEntryType(FsFileSystem* fs, const char* path, FsDirEntryType* out);
Result fsFsOpenFile(FsFileSystem* fs, const char* path, u32 mode, FsFile* out);
Result fsFsOpenDirectory(FsFileSystem* fs, const char* path, u32 mode, FsDir* out);
Result fsFsCommit(FsFileSystem* fs);
Result fsFsGetFreeSpace(FsFileSystem* fs, const char* path, s64* out);
Result fsFsGetTotalSpace(FsFileSystem* fs, const char* path, s64* out);
Result fsFsGetFileTimeStampRaw(FsFileSystem* fs, const char* path, FsTimeStampRaw *out); ///< [3.0.0+]
Result fsFsCleanDirectoryRecursively(FsFileSystem* fs, const char* path); ///< [3.0.0+]
Result fsFsQueryEntry(FsFileSystem* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryId query_id); ///< [4.0.0+]
Result fsFsGetFileSystemAttribute(FsFileSystem* fs, FsFileSystemAttribute *out); ///< [15.0.0+]
void fsFsClose(FsFileSystem* fs);

/// Uses \ref fsFsQueryEntry to set the archive bit on the specified absolute directory path.
/// This will cause HOS to treat the directory as if it were a file containing the directory's concatenated contents.
Result fsFsSetConcatenationFileAttribute(FsFileSystem* fs, const char *path);

/// Wrapper for fsFsQueryEntry with FsFileSystemQueryId_IsValidSignedSystemPartitionOnSdCard.
/// Only available on [8.0.0+].
Result fsFsIsValidSignedSystemPartitionOnSdCard(FsFileSystem* fs, bool *out);

// IFile
Result fsFileRead(FsFile* f, s64 off, void* buf, u64 read_size, u32 option, u64* bytes_read);
Result fsFileWrite(FsFile* f, s64 off, const void* buf, u64 write_size, u32 option);
Result fsFileFlush(FsFile* f);
Result fsFileSetSize(FsFile* f, s64 sz);
Result fsFileGetSize(FsFile* f, s64* out);
Result fsFileOperateRange(FsFile* f, FsOperationId op_id, s64 off, s64 len, FsRangeInfo* out); ///< [4.0.0+]
void fsFileClose(FsFile* f);

// IDirectory
Result fsDirRead(FsDir* d, s64* total_entries, size_t max_entries, FsDirectoryEntry *buf);
Result fsDirGetEntryCount(FsDir* d, s64* count);
void fsDirClose(FsDir* d);

// IStorage
Result fsStorageRead(FsStorage* s, s64 off, void* buf, u64 read_size);
Result fsStorageWrite(FsStorage* s, s64 off, const void* buf, u64 write_size);
Result fsStorageFlush(FsStorage* s);
Result fsStorageSetSize(FsStorage* s, s64 sz);
Result fsStorageGetSize(FsStorage* s, s64* out);
Result fsStorageOperateRange(FsStorage* s, FsOperationId op_id, s64 off, s64 len, FsRangeInfo* out); ///< [4.0.0+]
void fsStorageClose(FsStorage* s);

// ISaveDataInfoReader

/// Read FsSaveDataInfo data into the buf array.
Result fsSaveDataInfoReaderRead(FsSaveDataInfoReader *s, FsSaveDataInfo* buf, size_t max_entries, s64* total_entries);
void fsSaveDataInfoReaderClose(FsSaveDataInfoReader *s);

// IEventNotifier
Result fsEventNotifierGetEventHandle(FsEventNotifier* e, Event* out, bool autoclear);
void fsEventNotifierClose(FsEventNotifier* e);

// IDeviceOperator
Result fsDeviceOperatorIsSdCardInserted(FsDeviceOperator* d, bool* out);
Result fsDeviceOperatorGetSdCardSpeedMode(FsDeviceOperator* d, s64* out);
Result fsDeviceOperatorGetSdCardCid(FsDeviceOperator* d, void* dst, size_t dst_size, s64 size);
Result fsDeviceOperatorGetSdCardUserAreaSize(FsDeviceOperator* d, s64* out);
Result fsDeviceOperatorGetSdCardProtectedAreaSize(FsDeviceOperator* d, s64* out);
Result fsDeviceOperatorGetAndClearSdCardErrorInfo(FsDeviceOperator* d, FsStorageErrorInfo* out, s64 *out_log_size, void *dst, size_t dst_size, s64 size);
Result fsDeviceOperatorGetMmcCid(FsDeviceOperator* d, void* dst, size_t dst_size, s64 size);
Result fsDeviceOperatorGetMmcSpeedMode(FsDeviceOperator* d, s64* out);
Result fsDeviceOperatorGetMmcPatrolCount(FsDeviceOperator* d, u32* out);
Result fsDeviceOperatorGetAndClearMmcErrorInfo(FsDeviceOperator* d, FsStorageErrorInfo* out, s64 *out_log_size, void *dst, size_t dst_size, s64 size);
Result fsDeviceOperatorGetMmcExtendedCsd(FsDeviceOperator* d, void* dst, size_t dst_size, s64 size);
Result fsDeviceOperatorIsGameCardInserted(FsDeviceOperator* d, bool* out);
Result fsDeviceOperatorGetGameCardHandle(FsDeviceOperator* d, FsGameCardHandle* out);
Result fsDeviceOperatorGetGameCardUpdatePartitionInfo(FsDeviceOperator* d, const FsGameCardHandle* handle, FsGameCardUpdatePartitionInfo* out);
Result fsDeviceOperatorGetGameCardAttribute(FsDeviceOperator* d, const FsGameCardHandle* handle, u8 *out);
Result fsDeviceOperatorGetGameCardDeviceCertificate(FsDeviceOperator* d, const FsGameCardHandle* handle, void* dst, size_t dst_size, s64* out_size, s64 size);
Result fsDeviceOperatorGetGameCardIdSet(FsDeviceOperator* d, void* dst, size_t dst_size, s64 size);
Result fsDeviceOperatorGetGameCardErrorReportInfo(FsDeviceOperator* d, FsGameCardErrorReportInfo* out);
Result fsDeviceOperatorGetGameCardDeviceId(FsDeviceOperator* d, void* dst, size_t dst_size, s64 size);
Result fsDeviceOperatorChallengeCardExistence(FsDeviceOperator* d, const FsGameCardHandle* handle, void* dst, size_t dst_size, void* seed, size_t seed_size, void* value, size_t value_size);
void fsDeviceOperatorClose(FsDeviceOperator* d);
