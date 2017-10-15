// Copyright 2017 plutoo

// We use wrapped handles for type safety.

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

typedef enum {
    ENTRYTYPE_FILE=0,
    ENTRYTYPE_DIR =1
} FsEntryType;

#define FS_MAX_PATH 0x301

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
Result fsFileWrite(FsFile* f, u64 off, void* buf, size_t len, size_t* out);
Result fsFileFlush(FsFile* f);
Result fsFileSetSize(FsFile* f, u64 sz);
Result fsFileGetSize(FsFile* f, u64* out);
void fsFileClose(FsFile* f);

// todo: IDirectory
// todo: IStorage
