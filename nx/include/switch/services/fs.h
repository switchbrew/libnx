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
    u64 titleID;          ///< titleID of the savedata to access when accessing other titles' savedata via SaveData, otherwise FS_SAVEDATA_CURRENT_TITLEID.
    u128 userID;          ///< userID of the user-specific savedata to access, otherwise FS_SAVEDATA_USERID_COMMONSAVE. See account.h.
    u64 saveID;           ///< saveID, 0 for SaveData.
    u64 SaveDataType;     ///< See \ref FsSaveDataType.
    u64 unk_x28;          ///< 0 for SystemSaveData/SaveData.
    u64 unk_x30;          ///< 0 for SystemSaveData/SaveData.
    u64 unk_x38;          ///< 0 for SystemSaveData/SaveData.
} PACKED FsSave;

typedef struct
{
    u64 saveID_unk;
    u8 SaveDataSpaceId; ///< See \ref FsSaveDataSpaceId.
    u8 SaveDataType;    ///< See \ref FsSaveDataType.
    u8 pad[6];
    u128 userID;        ///< See userID for \ref FsSave.
    u64 saveID;         ///< See saveID for \ref FsSave.
    u64 titleID;        ///< titleID for FsSaveDataType_SaveData.
    u64 size;           ///< Raw saveimage size.
    u8 unk_x38[0x28];   ///< Unknown. Usually zeros?
} PACKED FsSaveDataInfo;

typedef enum {
    ENTRYTYPE_DIR = 0,
    ENTRYTYPE_FILE = 1
} FsEntryType;

typedef enum
{
    FS_OPEN_READ   = BIT(0), ///< Open for reading.
    FS_OPEN_WRITE  = BIT(1), ///< Open for writing.
    FS_OPEN_APPEND = BIT(2), ///< Append file.
} FsFileFlags;

/// For use with fsFsOpenDirectory.
typedef enum
{
    FS_DIROPEN_DIRECTORY   = BIT(0), ///< Enable reading directory entries.
    FS_DIROPEN_FILE  = BIT(1),       ///< Enable reading file entries.
} FsDirectoryFlags;

typedef enum
{
    FsStorageId_None =       0,
    FsStorageId_Host =       1,
    FsStorageId_GameCard =   2,
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

    FsSaveDataSpaceId_All = -1,             ///< Pseudo value for fsOpenSaveDataIterator().
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

Result fsInitialize(void);
void fsExit(void);

Service* fsGetServiceSession(void);

/// Do not call this directly, see fs_dev.h.
Result fsMountSdcard(FsFileSystem* out);

Result fsMountSaveData(FsFileSystem* out, u8 inval, FsSave *save);
Result fsMountSystemSaveData(FsFileSystem* out, u8 inval, FsSave *save);
Result fsOpenSaveDataIterator(FsSaveDataIterator* out, s32 SaveDataSpaceId);
Result fsOpenDataStorageByCurrentProcess(FsStorage* out);
Result fsOpenDeviceOperator(FsDeviceOperator* out);
Result fsOpenSdCardDetectionEventNotifier(FsEventNotifier* out);
// todo: Rest of commands here

/// FsFileSystem can be mounted with fs_dev for use with stdio, see fs_dev.h.

/// Wrapper(s) for fsMountSaveData.
/// See FsSave for titleID and userID.
Result fsMount_SaveData(FsFileSystem* out, u64 titleID, u128 userID);

/// Wrapper for fsMountSystemSaveData.
/// WARNING: You can brick when writing to SystemSaveData, if the data is corrupted etc.
Result fsMount_SystemSaveData(FsFileSystem* out, u64 saveID);

// IFileSystem
Result fsFsCreateFile(FsFileSystem* fs, const char* path, size_t size, int flags);
Result fsFsDeleteFile(FsFileSystem* fs, const char* path);
Result fsFsCreateDirectory(FsFileSystem* fs, const char* path);
Result fsFsDeleteDirectory(FsFileSystem* fs, const char* path);
Result fsFsDeleteDirectoryRecursively(FsFileSystem* fs, const char* path);
Result fsFsRenameFile(FsFileSystem* fs, const char* path0, const char* path1);
Result fsFsRenameDirectory(FsFileSystem* fs, const char* path0, const char* path1);
Result fsFsGetEntryType(FsFileSystem* fs, const char* path, FsEntryType* out);
Result fsFsOpenFile(FsFileSystem* fs, const char* path, int flags, FsFile* out);
Result fsFsOpenDirectory(FsFileSystem* fs, const char* path, int flags, FsDir* out);
Result fsFsCommit(FsFileSystem* fs);
Result fsFsGetFreeSpace(FsFileSystem* fs, const char* path, u64* out);
Result fsFsGetTotalSpace(FsFileSystem* fs, const char* path, u64* out);
void fsFsClose(FsFileSystem* fs);

// IFile
Result fsFileRead(FsFile* f, u64 off, void* buf, size_t len, size_t* out);
Result fsFileWrite(FsFile* f, u64 off, const void* buf, size_t len);
Result fsFileFlush(FsFile* f);
Result fsFileSetSize(FsFile* f, u64 sz);
Result fsFileGetSize(FsFile* f, u64* out);
void fsFileClose(FsFile* f);

// IDirectory
Result fsDirRead(FsDir* d, u64 inval, size_t* total_entries, size_t max_entries, FsDirectoryEntry *buf);
Result fsDirGetEntryCount(FsDir* d, u64* count);
void fsDirClose(FsDir* d);

// IStorage
Result fsStorageRead(FsStorage* s, u64 off, void* buf, size_t len);
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
void fsDeviceOperatorClose(FsDeviceOperator* d);
