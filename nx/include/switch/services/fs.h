// Copyright 2017 plutoo
#pragma once
#include <switch/types.h>
#include <switch/services/sm.h>

// We use wrapped handles for type safety.

#define FS_MAX_PATH 0x301

/// For use with fsMountSaveData().
#define FS_MOUNTSAVEDATA_INVAL_DEFAULT 0x1

/// For use with FsSave.
#define FS_SAVEDATA_CURRENT_TITLEID 0

/// For use with FsSave.
#define FS_SAVEDATA_USERID_COMMONSAVE 0

typedef struct {
    Handle  h;
} FsFileSystem;

typedef struct {
    Handle  h;
} FsFile;

typedef struct {
    Handle  h;
} FsDir;

typedef struct {
    Handle  h;
} FsStorage;

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
    u64 ContentStorageId; ///< ContentStorageId? See FsContentStorageId.
    u64 unk_x28;          ///< 0 for SystemSaveData/SaveData.
    u64 unk_x30;          ///< 0 for SystemSaveData/SaveData.
    u64 unk_x38;          ///< 0 for SystemSaveData/SaveData.
} PACKED FsSave;

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
    FS_CONTENTSTORAGEID_NandSystem = 0,
    FS_CONTENTSTORAGEID_NandUser   = 1,
    FS_CONTENTSTORAGEID_SdCard     = 2,
} FsContentStorageId;

Result fsInitialize(void);
void fsExit(void);

Service* fsGetServiceSession(void);

Result fsMountSdcard(FsFileSystem* out);
Result fsMountSaveData(FsFileSystem* out, u8 inval, FsSave *save);
// todo: Rest of commands here

/// FsFileSystem can be mounted with fs_dev for use with stdio, see fs_dev.h.

/// Wrapper(s) for fsMountSaveData.
/// See FsSave for titleID and userID.
Result fsMount_SaveData(FsFileSystem* out, u64 titleID, u128 userID);

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

// todo: IStorage
