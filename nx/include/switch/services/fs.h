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

typedef struct {
    u64 created;  ///< POSIX timestamp.
    u64 modified; ///< POSIX timestamp.
    u64 accessed; ///< POSIX timestamp.
    u8 is_valid;  ///< 0x1 when the timestamps are set.
    u8 padding[7];
} FsTimeStampRaw;

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
    FsContentStorageId_System = 0,
    FsContentStorageId_User   = 1,
    FsContentStorageId_SdCard = 2,
} FsContentStorageId;

typedef enum {
    FsCustomStorageId_System   = 0,
    FsCustomStorageId_SdCard   = 1,
} FsCustomStorageId;

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

/// FsPriority
typedef enum {
    FsPriority_Normal     = 0,
    FsPriority_Realtime   = 1,
    FsPriority_Low        = 2,
    FsPriority_Background = 3,
} FsPriority;

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
Result fsOpenFileSystemWithPatch(FsFileSystem* out, u64 id, FsFileSystemType fsType); ///< [2.0.0+], like OpenFileSystemWithId but without content path.
Result fsOpenFileSystemWithId(FsFileSystem* out, u64 id, FsFileSystemType fsType, const char* contentPath); ///< works on all firmwares, id is ignored on [1.0.0]

Result fsOpenBisFileSystem(FsFileSystem* out, FsBisPartitionId partitionId, const char* string);
Result fsOpenBisStorage(FsStorage* out, FsBisPartitionId partitionId);

/// Do not call this directly, see fs_dev.h.
Result fsOpenSdCardFileSystem(FsFileSystem* out);

Result fsCreateSaveDataFileSystemBySystemSaveDataId(const FsSaveDataAttribute* attr, const FsSaveDataCreationInfo* creation_info);
Result fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId save_data_space_id, u64 saveID); /// [2.0.0+]

Result fsIsExFatSupported(bool* out);

Result fsOpenGameCardFileSystem(FsFileSystem* out, const FsGameCardHandle* handle, FsGameCardPartition partition);

Result fsExtendSaveDataFileSystem(FsSaveDataSpaceId save_data_space_id, u64 saveID, s64 dataSize, s64 journalSize); /// [3.0.0+]

Result fsOpenSaveDataFileSystem(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);
Result fsOpenSaveDataFileSystemBySystemSaveDataId(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);

Result fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(void* buf, size_t len, FsSaveDataSpaceId save_data_space_id, u64 saveID);
Result fsReadSaveDataFileSystemExtraData(void* buf, size_t len, u64 saveID);
Result fsWriteSaveDataFileSystemExtraData(const void* buf, size_t len, FsSaveDataSpaceId save_data_space_id, u64 saveID);

Result fsOpenSaveDataInfoReader(FsSaveDataInfoReader* out, FsSaveDataSpaceId save_data_space_id);

Result fsOpenContentStorageFileSystem(FsFileSystem* out, FsContentStorageId content_storage_id);
Result fsOpenCustomStorageFileSystem(FsFileSystem* out, FsCustomStorageId custom_storage_id); /// [7.0.0+]

Result fsOpenDataStorageByCurrentProcess(FsStorage* out);
Result fsOpenDataStorageByDataId(FsStorage* out, u64 dataId, NcmStorageId storageId);

Result fsOpenDeviceOperator(FsDeviceOperator* out);
Result fsOpenSdCardDetectionEventNotifier(FsEventNotifier* out);

/// Retrieves the rights id corresponding to the content path. Only available on [2.0.0+].
Result fsGetRightsIdByPath(const char* path, FsRightsId* out_rights_id);

/// Retrieves the rights id and key generation corresponding to the content path. Only available on [3.0.0+].
Result fsGetRightsIdAndKeyGenerationByPath(const char* path, u8* out_key_generation, FsRightsId* out_rights_id);

Result fsDisableAutoSaveDataCreation(void);

Result fsSetGlobalAccessLogMode(u32 mode);
Result fsGetGlobalAccessLogMode(u32* out_mode);

// Wrapper(s) for fsCreateSaveDataFileSystemBySystemSaveDataId.
Result fsCreate_SystemSaveDataWithOwner(FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, AccountUid uid, u64 owner_id, s64 size, s64 journal_size, u32 flags);
Result fsCreate_SystemSaveData(FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, s64 size, s64 journal_size, u32 flags);

/// Wrapper(s) for fsOpenSaveDataFileSystem.
/// See FsSave for program_id and uid.
Result fsOpen_SaveData(FsFileSystem* out, u64 application_id, AccountUid uid);

/// Wrapper for fsOpenSaveDataFileSystemBySystemSaveDataId.
/// WARNING: You can brick when writing to SystemSaveData, if the data is corrupted etc.
Result fsOpen_SystemSaveData(FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, AccountUid uid);

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
Result fsDeviceOperatorIsGameCardInserted(FsDeviceOperator* d, bool* out);
Result fsDeviceOperatorGetGameCardHandle(FsDeviceOperator* d, FsGameCardHandle* out);
Result fsDeviceOperatorGetGameCardAttribute(FsDeviceOperator* d, const FsGameCardHandle* handle, u8 *out);
void fsDeviceOperatorClose(FsDeviceOperator* d);
