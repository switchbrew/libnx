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

/// For use with FsSave.
#define FS_SAVEDATA_CURRENT_PROGRAMID 0

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
    u64 fileSize;                   ///< File size.
} FsDirectoryEntry;

/// Save Struct
typedef struct {
    u64 program_id;                 ///< ProgramId of the savedata to access when accessing other programs' savedata via SaveData, otherwise FS_SAVEDATA_CURRENT_PROGRAMID.
    AccountUid uid;                 ///< \ref AccountUid for the user-specific savedata to access, otherwise 0 for common savedata.
    u64 saveID;                     ///< saveID, 0 for SaveData.
    u8 saveDataType;                ///< See \ref FsSaveDataType.
    u8 rank;                        ///< Save data 'rank' or 'precedence'. 0 if this save data is considered the primary save data. 1 if it's considered the secondary save data.
    u16 index;                      ///< Save data index.
    u32 pad_x24;                    ///< Padding.
    u64 unk_x28;                    ///< 0 for SystemSaveData/SaveData.
    u64 unk_x30;                    ///< 0 for SystemSaveData/SaveData.
    u64 unk_x38;                    ///< 0 for SystemSaveData/SaveData.
} FsSave;

/// SaveDataExtraData Struct
typedef struct {
    FsSave save;      ///< Save struct.
    u64 ownerId;      ///< Id of the owner of this save data. 0 for SystemSaveData.
    u64 timestamp;    ///< POSIX timestamp.
    u32 flags;        ///< Save data flags. See \ref FsSaveDataFlags.
    u32 unk_x54;      ///< Normally 0. Possibly unused?
    s64 dataSize;     ///< Usable save data size.
    s64 journalSize;  ///< Journal size of the save data.
    u64 commitId;     ///< Id of the latest commit.
    u8 unused[0x190]; ///< Uninitialized.
} FsSaveDataExtraData;

/// SaveCreate Struct
typedef struct {
    s64 size;           ///< Size of the save data.
    s64 journalSize;    ///< Journal size of the save data.
    u64 blockSize;      ///< Block size of the save data.
    u64 ownerId;        ///< ProgramId of the owner of this save data. 0 for SystemSaveData.
    u32 flags;          ///< Save data flags. See \ref FsSaveDataFlags.
    u8 saveDataSpaceId; ///< See \ref FsSaveDataSpaceId.
    u8 unk;             ///< 0 for SystemSaveData.
    u8 padding[0x1A];   ///< Uninitialized for SystemSaveData.
} FsSaveCreate;

typedef struct {
    u64 saveID_unk;
    u8 saveDataSpaceId; ///< See \ref FsSaveDataSpaceId.
    u8 saveDataType;    ///< See \ref FsSaveDataType.
    u8 pad[6];          ///< Padding.
    AccountUid uid;     ///< FsSave::userID
    u64 saveID;         ///< See saveID for \ref FsSave.
    u64 application_id; ///< ApplicationId for FsSaveDataType_SaveData.
    u64 size;           ///< Raw saveimage size.
    u16 index;          ///< Save data index.
    u8 rank;            ///< Save data 'rank' or 'precedence'. 0 if this save data is considered the primary save data. 1 if it's considered the secondary save data.
    u8 unk_x3b[0x25];   ///< Unknown. Usually zeros?
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
    FsContentStorageId_NandSystem = 0,
    FsContentStorageId_NandUser   = 1,
    FsContentStorageId_SdCard     = 2,
} FsContentStorageId;

typedef enum {
    FsCustomStorageId_NandUser = 0,
    FsCustomStorageId_SdCard   = 1,
} FsCustomStorageId;

typedef enum {
    FsSaveDataSpaceId_NandSystem       = 0,
    FsSaveDataSpaceId_NandUser         = 1,
    FsSaveDataSpaceId_SdCard           = 2,
    FsSaveDataSpaceId_TemporaryStorage = 3,

    FsSaveDataSpaceId_All              = -1, ///< Pseudo value for fsOpenSaveDataInfoReader().
} FsSaveDataSpaceId;

typedef enum {
    FsSaveDataType_SystemSaveData           = 0,
    FsSaveDataType_SaveData                 = 1,
    FsSaveDataType_BcatDeliveryCacheStorage = 2,
    FsSaveDataType_DeviceSaveData           = 3,
    FsSaveDataType_TemporaryStorage         = 4, ///< [3.0.0+]
    FsSaveDataType_CacheStorage             = 5, ///< [3.0.0+]
} FsSaveDataType;

/// SaveDataFlags
typedef enum {
    FsSaveDataFlags_SurviveFactoryReset                    = BIT(0),
    FsSaveDataFlags_SurviveFactoryResetForRefurbishment    = BIT(1),
    FsSaveDataFlags_SurviveFactoryResetWithoutUserSaveData = BIT(2),
} FsSaveDataFlags;

typedef enum {
    FsGameCardAttribute_AutoBoot   = BIT(0), ///< Causes the cartridge to automatically start on bootup
    FsGameCardAttribute_ForceError = BIT(1), ///< Causes NS to throw an error on attempt to load the cartridge
    FsGameCardAttribute_Repair     = BIT(2), ///< Indicates that this gamecard is a repair tool.
} FsGameCardAttribute;

typedef enum {
    FsGameCardPartiton_Update = 0,
    FsGameCardPartiton_Normal = 1,
    FsGameCardPartiton_Secure = 2,
} FsGameCardPartiton;

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

typedef enum {
    FsBisStorageId_Boot0                           = 0,

    FsBisStorageId_Boot1                           = 10,

    FsBisStorageId_UserDataRoot                    = 20,
    FsBisStorageId_BootConfigAndPackage2NormalMain = 21,
    FsBisStorageId_BootConfigAndPackage2NormalSub  = 22,
    FsBisStorageId_BootConfigAndPackage2SafeMain   = 23,
    FsBisStorageId_BootConfigAndPackage2SafeSub    = 24,
    FsBisStorageId_BootConfigAndPackage2RepairMain = 25,
    FsBisStorageId_BootConfigAndPackage2RepairSub  = 26,
    FsBisStorageId_CalibrationBinary               = 27,
    FsBisStorageId_CalibrationFile                 = 28,
    FsBisStorageId_SafeMode                        = 29,
    FsBisStorageId_User                            = 30,
    FsBisStorageId_System                          = 31,
    FsBisStorageId_SystemProperEncryption          = 32,
    FsBisStorageId_SystemProperPartition           = 33,
} FsBisStorageId;

typedef enum {
    FsFileSystemType_Logo               = 2,
    FsFileSystemType_ContentControl     = 3,
    FsFileSystemType_ContentManual      = 4,
    FsFileSystemType_ContentMeta        = 5,
    FsFileSystemType_ContentData        = 6,
    FsFileSystemType_ApplicationPackage = 7,
} FsFileSystemType;

typedef enum {
    FsFileSystemQueryType_SetArchiveBit = 0,
} FsFileSystemQueryType;

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

void fsSetPriority(FsPriority prio);

/// Mount requested filesystem type from content file
Result fsOpenFileSystem(FsFileSystem* out, FsFileSystemType fsType, const char* contentPath); ///< same as calling fsOpenFileSystemWithId with 0 as id
Result fsOpenFileSystemWithPatch(FsFileSystem* out, u64 id, FsFileSystemType fsType); ///< [2.0.0+], like OpenFileSystemWithId but without content path.
Result fsOpenFileSystemWithId(FsFileSystem* out, u64 id, FsFileSystemType fsType, const char* contentPath); ///< works on all firmwares, id is ignored on [1.0.0]

Result fsOpenBisFileSystem(FsFileSystem* out, FsBisStorageId partitionId, const char* string);
Result fsOpenBisStorage(FsStorage* out, FsBisStorageId partitionId);

/// Do not call this directly, see fs_dev.h.
Result fsOpenSdCardFileSystem(FsFileSystem* out);

Result fsCreateSaveDataFileSystemBySystemSaveDataId(const FsSave* save, const FsSaveCreate* create);
Result fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId saveDataSpaceId, u64 saveID); /// [2.0.0+]

Result fsIsExFatSupported(bool* out);

Result fsOpenGameCardFileSystem(FsFileSystem* out, const FsGameCardHandle* handle, FsGameCardPartiton partition);

Result fsExtendSaveDataFileSystem(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, s64 dataSize, s64 journalSize); /// [3.0.0+]

Result fsOpenSaveDataFileSystem(FsFileSystem* out, FsSaveDataSpaceId saveDataSpaceId, const FsSave *save);
Result fsOpenSaveDataFileSystemBySystemSaveDataId(FsFileSystem* out, FsSaveDataSpaceId saveDataSpaceId, const FsSave *save);

Result fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(void* buf, size_t len, FsSaveDataSpaceId saveDataSpaceId, u64 saveID);
Result fsReadSaveDataFileSystemExtraData(void* buf, size_t len, u64 saveID);
Result fsWriteSaveDataFileSystemExtraData(const void* buf, size_t len, FsSaveDataSpaceId saveDataSpaceId, u64 saveID);

Result fsOpenSaveDataInfoReader(FsSaveDataInfoReader* out, FsSaveDataSpaceId saveDataSpaceId);

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
Result fsCreate_SystemSaveDataWithOwner(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, AccountUid uid, u64 ownerId, u64 size, u64 journalSize, u32 flags);
Result fsCreate_SystemSaveData(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, u64 size, u64 journalSize, u32 flags);

/// Wrapper(s) for fsOpenSaveDataFileSystem.
/// See FsSave for program_id and uid.
Result fsOpen_SaveData(FsFileSystem* out, u64 program_id, AccountUid uid);

/// Wrapper for fsOpenSaveDataFileSystemBySystemSaveDataId.
/// WARNING: You can brick when writing to SystemSaveData, if the data is corrupted etc.
Result fsOpen_SystemSaveData(FsFileSystem* out, FsSaveDataSpaceId saveDataSpaceId, u64 saveID, AccountUid uid);

// IFileSystem
Result fsFsCreateFile(FsFileSystem* fs, const char* path, u64 size, u32 option);
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
Result fsFsGetFreeSpace(FsFileSystem* fs, const char* path, u64* out);
Result fsFsGetTotalSpace(FsFileSystem* fs, const char* path, u64* out);
Result fsFsGetFileTimeStampRaw(FsFileSystem* fs, const char* path, FsTimeStampRaw *out); ///< [3.0.0+]
Result fsFsCleanDirectoryRecursively(FsFileSystem* fs, const char* path); ///< [3.0.0+]
Result fsFsQueryEntry(FsFileSystem* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryType query_type); ///< [4.0.0+]
void fsFsClose(FsFileSystem* fs);

/// Uses \ref fsFsQueryEntry to set the archive bit on the specified absolute directory path.
/// This will cause HOS to treat the directory as if it were a file containing the directory's concatenated contents.
Result fsFsSetArchiveBit(FsFileSystem* fs, const char *path);

// IFile
Result fsFileRead(FsFile* f, u64 off, void* buf, u64 read_size, u32 option, u64* bytes_read);
Result fsFileWrite(FsFile* f, u64 off, const void* buf, u64 write_size, u32 option);
Result fsFileFlush(FsFile* f);
Result fsFileSetSize(FsFile* f, u64 sz);
Result fsFileGetSize(FsFile* f, u64* out);
Result fsFileOperateRange(FsFile* f, FsOperationId op_id, u64 off, u64 len, FsRangeInfo* out); ///< [4.0.0+]
void fsFileClose(FsFile* f);

// IDirectory
Result fsDirRead(FsDir* d, u64 inval, u64* total_entries, size_t max_entries, FsDirectoryEntry *buf);
Result fsDirGetEntryCount(FsDir* d, u64* count);
void fsDirClose(FsDir* d);

// IStorage
Result fsStorageRead(FsStorage* s, u64 off, void* buf, u64 read_size);
Result fsStorageWrite(FsStorage* s, u64 off, const void* buf, u64 write_size);
Result fsStorageFlush(FsStorage* s);
Result fsStorageSetSize(FsStorage* s, u64 sz);
Result fsStorageGetSize(FsStorage* s, u64* out);
Result fsStorageOperateRange(FsStorage* s, FsOperationId op_id, u64 off, u64 len, FsRangeInfo* out); ///< [4.0.0+]
void fsStorageClose(FsStorage* s);

// ISaveDataInfoReader

/// Read FsSaveDataInfo data into the buf array.
Result fsSaveDataInfoReaderRead(FsSaveDataInfoReader *s, FsSaveDataInfo* buf, size_t max_entries, u64* total_entries);
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
