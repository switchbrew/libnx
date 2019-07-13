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
#include "../services/sm.h"

// We use wrapped handles for type safety.

#define FS_MAX_PATH 0x301

/// For use with FsSave.
#define FS_SAVEDATA_CURRENT_TITLEID 0

/// For use with \ref FsSave and \ref FsSaveDataInfo.
#define FS_SAVEDATA_USERID_COMMONSAVE 0

typedef struct {
    u8 c[0x10];
} FsRightsId;

typedef struct {
    Service  s;
} FsFileSystem;

typedef struct {
    Service  s;
} FsFile;

typedef struct {
    Service  s;
} FsDir;

typedef struct {
    Service  s;
} FsStorage;

typedef struct {
    Service  s;
} FsSaveDataIterator;

typedef struct {
    Service  s;
} FsEventNotifier;

typedef struct {
    Service  s;
} FsDeviceOperator;

/// Directory entry.
typedef struct
{
    char name[FS_MAX_PATH];      ///< Entry name.
    u8 pad[3];
    s8 type;       ///< See FsEntryType.
    u8 pad2[3];     ///< ?
    u64 fileSize;         ///< File size.
} FsDirectoryEntry;

/// Save Struct
typedef struct
{
    u64 titleID;                    ///< titleID of the savedata to access when accessing other titles' savedata via SaveData, otherwise FS_SAVEDATA_CURRENT_TITLEID.
    union { u128 userID; } PACKED;  ///< userID of the user-specific savedata to access, otherwise FS_SAVEDATA_USERID_COMMONSAVE. See account.h.
    u64 saveID;                     ///< saveID, 0 for SaveData.
    u8 saveDataType;                ///< See \ref FsSaveDataType.
    u8 rank;                        ///< Save data 'rank' or 'precedence'. 0 if this save data is considered the primary save data. 1 if it's considered the secondary save data.
    u16 index;                      ///< Save data index.
    u32 pad_x24;                    ///< Padding.
    u64 unk_x28;                    ///< 0 for SystemSaveData/SaveData.
    u64 unk_x30;                    ///< 0 for SystemSaveData/SaveData.
    u64 unk_x38;                    ///< 0 for SystemSaveData/SaveData.
} FsSave;

/// SaveCreate Struct
typedef struct {
    u64 size;           ///< Size of the save data.
    u64 journalSize;    ///< Journal size of the save data.
    u64 blockSize;      ///< Block size of the save data.
    u64 ownerId;        ///< Title id of the owner of this save data. 0 for SystemSaveData.
    u32 flags;          ///< Save data flags.
    u8 saveDataSpaceId; ///< See \ref FsSaveDataSpaceId.
    u8 unk;             ///< 0 for SystemSaveData.
    u8 padding[0x1A];   ///< Uninitialized for SystemSaveData.
} FsSaveCreate;

typedef struct
{
    u64 saveID_unk;
    u8 saveDataSpaceId; ///< See \ref FsSaveDataSpaceId.
    u8 saveDataType;    ///< See \ref FsSaveDataType.
    u8 pad[6];          ///< Padding.
    u128 userID;        ///< See userID for \ref FsSave.
    u64 saveID;         ///< See saveID for \ref FsSave.
    u64 titleID;        ///< titleID for FsSaveDataType_SaveData.
    u64 size;           ///< Raw saveimage size.
    u16 index;          ///< Save data index.
    u8 rank;            ///< Save data 'rank' or 'precedence'. 0 if this save data is considered the primary save data. 1 if it's considered the secondary save data.
    u8 unk_x3b[0x25];   ///< Unknown. Usually zeros?
} FsSaveDataInfo;

typedef struct
{
    u64 created;  ///< POSIX timestamp.
    u64 modified; ///< POSIX timestamp.
    u64 accessed; ///< POSIX timestamp.
    u8 is_valid;  ///< 0x1 when the timestamps are set.
    u8 padding[7];
} FsTimeStampRaw;

typedef enum {
    ENTRYTYPE_DIR  = 0,
    ENTRYTYPE_FILE = 1,
} FsEntryType;

typedef enum
{
    FS_OPEN_READ   = BIT(0), ///< Open for reading.
    FS_OPEN_WRITE  = BIT(1), ///< Open for writing.
    FS_OPEN_APPEND = BIT(2), ///< Append file.
} FsFileFlags;

typedef enum
{
    FS_CREATE_BIG_FILE = BIT(0), ///< Creates a ConcatenationFile (dir with archive bit) instead of file.
} FsFileCreateFlags;

/// For use with fsFsOpenDirectory.
typedef enum
{
    FS_DIROPEN_DIRECTORY    = BIT(0),  ///< Enable reading directory entries.
    FS_DIROPEN_FILE         = BIT(1),  ///< Enable reading file entries.
    FS_DIROPEN_NO_FILE_SIZE = BIT(31), ///< Causes result entries to not contain filesize information (always 0).
} FsDirectoryFlags;

typedef enum
{
    FS_READOPTION_NONE = 0, ///< No Option.
} FsReadOption;

typedef enum
{
    FS_WRITEOPTION_NONE  = 0,      ///< No option.
    FS_WRITEOPTION_FLUSH = BIT(0), ///< Forces a flush after write.
} FsWriteOption;

typedef enum
{
    FsStorageId_None       = 0,
    FsStorageId_Host       = 1,
    FsStorageId_GameCard   = 2,
    FsStorageId_NandSystem = 3,
    FsStorageId_NandUser   = 4,
    FsStorageId_SdCard     = 5,
} FsStorageId;

typedef enum
{
    FS_CONTENTSTORAGEID_NandSystem = 0,
    FS_CONTENTSTORAGEID_NandUser   = 1,
    FS_CONTENTSTORAGEID_SdCard     = 2,
} FsContentStorageId;

typedef enum
{
    FsSaveDataSpaceId_NandSystem       = 0,
    FsSaveDataSpaceId_NandUser         = 1,
    FsSaveDataSpaceId_SdCard           = 2,
    FsSaveDataSpaceId_TemporaryStorage = 3,

    FsSaveDataSpaceId_All              = -1, ///< Pseudo value for fsOpenSaveDataIterator().
} FsSaveDataSpaceId;

typedef enum
{
    FsSaveDataType_SystemSaveData           = 0,
    FsSaveDataType_SaveData                 = 1,
    FsSaveDataType_BcatDeliveryCacheStorage = 2,
    FsSaveDataType_DeviceSaveData           = 3,
    FsSaveDataType_TemporaryStorage         = 4, ///< [3.0.0+]
    FsSaveDataType_CacheStorage             = 5, ///< [3.0.0+]
} FsSaveDataType;

typedef enum {
    FsGameCardAttribute_AutoBoot   = (1 << 0), ///< Causes the cartridge to automatically start on bootup
    FsGameCardAttribute_ForceError = (1 << 1), ///< Causes NS to throw an error on attempt to load the cartridge
    FsGameCardAttribute_Repair     = (1 << 2), ///< Indicates that this gamecard is a repair tool.
} FsGameCardAttribute;

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

Result fsInitialize(void);
void fsExit(void);

Service* fsGetServiceSession(void);

Result fsOpenBisStorage(FsStorage* out, FsBisStorageId partitionId);
Result fsOpenBisFileSystem(FsFileSystem* out, FsBisStorageId partitionId, const char* string);

Result fsCreateSaveDataFileSystemBySystemSaveDataId(const FsSave* save, const FsSaveCreate* create);
Result fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId saveDataSpaceId, u64 saveID);

Result fsIsExFatSupported(bool* out);

/// Do not call this directly, see fs_dev.h.
Result fsMountSdcard(FsFileSystem* out);

Result fsMountSaveData(FsFileSystem* out, u8 inval, FsSave *save);
Result fsMountSystemSaveData(FsFileSystem* out, u8 inval, FsSave *save);
Result fsOpenSaveDataIterator(FsSaveDataIterator* out, s32 saveDataSpaceId);
Result fsOpenContentStorageFileSystem(FsFileSystem* out, FsContentStorageId content_storage_id);
Result fsOpenDataStorageByCurrentProcess(FsStorage* out);
Result fsOpenDataStorageByDataId(FsStorage* out, u64 dataId, FsStorageId storageId);
Result fsOpenDeviceOperator(FsDeviceOperator* out);
Result fsOpenSdCardDetectionEventNotifier(FsEventNotifier* out);

/// Retrieves the rights id corresponding to the content path. Only available on [2.0.0+].
Result fsGetRightsIdByPath(const char* path, FsRightsId* out_rights_id);

/// Retrieves the rights id and key generation corresponding to the content path. Only available on [3.0.0+].
Result fsGetRightsIdAndKeyGenerationByPath(const char* path, u8* out_key_generation, FsRightsId* out_rights_id);

Result fsDisableAutoSaveDataCreation(void);
// todo: Rest of commands here

// Wrapper(s) for fsCreateSaveDataFileSystemBySystemSaveDataId.
Result fsCreate_SystemSaveDataWithOwner(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, u128 userID, u64 ownerId, u64 size, u64 journalSize, u32 flags);
Result fsCreate_SystemSaveData(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, u64 size, u64 journalSize, u32 flags);

/// FsFileSystem can be mounted with fs_dev for use with stdio, see fs_dev.h.

/// Wrapper(s) for fsMountSaveData.
/// See FsSave for titleID and userID.
Result fsMount_SaveData(FsFileSystem* out, u64 titleID, u128 userID);

/// Wrapper for fsMountSystemSaveData.
/// WARNING: You can brick when writing to SystemSaveData, if the data is corrupted etc.
Result fsMount_SystemSaveData(FsFileSystem* out, u64 saveID);

typedef enum
{
    FsFileSystemType_Logo               = 2,
    FsFileSystemType_ContentControl     = 3,
    FsFileSystemType_ContentManual      = 4,
    FsFileSystemType_ContentMeta        = 5,
    FsFileSystemType_ContentData        = 6,
    FsFileSystemType_ApplicationPackage = 7,
} FsFileSystemType;

typedef enum
{
    FsFileSystemQueryType_SetArchiveBit = 0,
} FsFileSystemQueryType;

/// Mount requested filesystem type from content file
Result fsOpenFileSystem(FsFileSystem* out, FsFileSystemType fsType, const char* contentPath); /// same as calling fsOpenFileSystemWithId with 0 as titleId
Result fsOpenFileSystemWithId(FsFileSystem* out, u64 titleId, FsFileSystemType fsType, const char* contentPath); /// works on all firmwares, titleId is ignored on 1.0.0
Result fsOpenFileSystemWithPatch(FsFileSystem* out, u64 titleId, FsFileSystemType fsType); /// 2.0.0+, like OpenFileSystemWithId but without content path.

// IFileSystem
Result fsFsCreateFile(FsFileSystem* fs, const char* path, size_t size, int flags);
Result fsFsDeleteFile(FsFileSystem* fs, const char* path);
Result fsFsCreateDirectory(FsFileSystem* fs, const char* path);
Result fsFsDeleteDirectory(FsFileSystem* fs, const char* path);
Result fsFsDeleteDirectoryRecursively(FsFileSystem* fs, const char* path);
Result fsFsRenameFile(FsFileSystem* fs, const char* cur_path, const char* new_path);
Result fsFsRenameDirectory(FsFileSystem* fs, const char* cur_path, const char* new_path);
Result fsFsGetEntryType(FsFileSystem* fs, const char* path, FsEntryType* out);
Result fsFsOpenFile(FsFileSystem* fs, const char* path, int flags, FsFile* out);
Result fsFsOpenDirectory(FsFileSystem* fs, const char* path, int flags, FsDir* out);
Result fsFsCommit(FsFileSystem* fs);
Result fsFsGetFreeSpace(FsFileSystem* fs, const char* path, u64* out);
Result fsFsGetTotalSpace(FsFileSystem* fs, const char* path, u64* out);
Result fsFsGetFileTimeStampRaw(FsFileSystem* fs, const char* path, FsTimeStampRaw *out);/// 3.0.0+
Result fsFsCleanDirectoryRecursively(FsFileSystem* fs, const char* path);/// 3.0.0+
Result fsFsQueryEntry(FsFileSystem* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryType query_type);/// 4.0.0+
void fsFsClose(FsFileSystem* fs);

/// Uses \ref fsFsQueryEntry to set the archive bit on the specified absolute directory path.
/// This will cause HOS to treat the directory as if it were a file containing the directory's concatenated contents.
Result fsFsSetArchiveBit(FsFileSystem* fs, const char *path);

// IFile
Result fsFileRead(FsFile* f, u64 off, void* buf, size_t len, u32 option, size_t* out);
Result fsFileWrite(FsFile* f, u64 off, const void* buf, size_t len, u32 option);
Result fsFileFlush(FsFile* f);
Result fsFileSetSize(FsFile* f, u64 sz);
Result fsFileGetSize(FsFile* f, u64* out);
Result fsFileOperateRange(FsFile* f, FsOperationId op_id, u64 off, size_t len, FsRangeInfo* out); /// 4.0.0+
void fsFileClose(FsFile* f);

// IDirectory
Result fsDirRead(FsDir* d, u64 inval, size_t* total_entries, size_t max_entries, FsDirectoryEntry *buf);
Result fsDirGetEntryCount(FsDir* d, u64* count);
void fsDirClose(FsDir* d);

// IStorage
Result fsStorageRead(FsStorage* s, u64 off, void* buf, size_t len);
Result fsStorageWrite(FsStorage* s, u64 off, const void* buf, size_t len);
Result fsStorageFlush(FsStorage* s);
Result fsStorageSetSize(FsStorage* s, u64 sz);
Result fsStorageGetSize(FsStorage* s, u64* out);
Result fsStorageOperateRange(FsStorage* s, FsOperationId op_id, u64 off, size_t len, FsRangeInfo* out); /// 4.0.0+
void fsStorageClose(FsStorage* s);

// ISaveDataInfoReader

/// Read FsSaveDataInfo data into the buf array.
Result fsSaveDataIteratorRead(FsSaveDataIterator *s, FsSaveDataInfo* buf, size_t max_entries, size_t* total_entries);
void fsSaveDataIteratorClose(FsSaveDataIterator *s);

// IEventNotifier
Result fsEventNotifierGetEventHandle(FsEventNotifier* e, Handle* out);
void fsEventNotifierClose(FsEventNotifier* e);

// IDeviceOperator
Result fsDeviceOperatorIsSdCardInserted(FsDeviceOperator* d, bool* out);
Result fsDeviceOperatorIsGameCardInserted(FsDeviceOperator* d, bool* out);
Result fsDeviceOperatorGetGameCardHandle(FsDeviceOperator* d, FsGameCardHandle* out);
Result fsDeviceOperatorGetGameCardAttribute(FsDeviceOperator* d, const FsGameCardHandle* handle, u8 *out);
void fsDeviceOperatorClose(FsDeviceOperator* d);
