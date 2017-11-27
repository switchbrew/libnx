// Copyright 2017 plutoo

// We use wrapped handles for type safety.

#define FS_MAX_PATH 0x301

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
	u32 attributes;       ///< Attributes.
	u64 fileSize;         ///< File size.
} FsDirectoryEntry;

typedef enum {
    ENTRYTYPE_FILE=0,
    ENTRYTYPE_DIR =1
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

/// Attribute flags.
typedef enum
{
	FS_ATTRIBUTE_FILE = BIT(0),  ///< File.
} FsAttribute;

Result fsInitialize();
void fsExit(void);

Handle fsGetServiceSession(void);

Result fsMountSdcard(FsFileSystem* out);
// todo: Rest of commands here

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
Result fsFsGetFreeSpace(FsFileSystem* fs, u64* out);
Result fsFsGetTotalSpace(FsFileSystem* fs, u64* out);
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
