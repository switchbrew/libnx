// Copyright 2017 plutoo
#include <switch.h>

static Handle g_fsHandle = INVALID_HANDLE;

Result fsInitialize() {
    if(g_fsHandle != INVALID_HANDLE)return 0;
    Result rc = smGetService(&g_fsHandle, "fsp-srv");

    if (R_SUCCEEDED(rc)) {
        IpcCommand c;
        ipcInitialize(&c);
        ipcSendPid(&c);

        struct {
            u64 magic;
            u64 cmd_id;
            u64 unk;
        } *raw;

        raw = ipcPrepareHeader(&c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 1;
        raw->unk = 0;

        rc = ipcDispatch(g_fsHandle);

        if (R_SUCCEEDED(rc)) {
            IpcCommandResponse r;
            ipcParseResponse(&r);

            struct {
                u64 magic;
                u64 result;
            } *resp = r.Raw;

            rc = resp->result;
        }
    }

    return rc;
}

void fsExit(void) {
    if(g_fsHandle == INVALID_HANDLE)return;
    svcCloseHandle(g_fsHandle);
    g_fsHandle = INVALID_HANDLE;
}

Handle fsGetServiceSession(void) {
    return g_fsHandle;
}

Result fsMountSdcard(FsFileSystem* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;

    Result rc = ipcDispatch(g_fsHandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            out->h = r.Handles[0];
        }
    }

    return rc;
}

// IFileSystem impl
Result fsFsCreateFile(FsFileSystem* fs, const char* path, size_t size, int flags) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 zero;
        u64 size;
        u32 flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->zero = 0;
    raw->size = size;
    raw->flags = flags;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsDeleteFile(FsFileSystem* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsCreateDirectory(FsFileSystem* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsDeleteDirectory(FsFileSystem* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsDeleteDirectoryRecursively(FsFileSystem* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsRenameFile(FsFileSystem* fs, const char* path0, const char* path1) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path0, FS_MAX_PATH, 0);
    ipcAddSendStatic(&c, path1, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsRenameDirectory(FsFileSystem* fs, const char* path0, const char* path1) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path0, FS_MAX_PATH, 0);
    ipcAddSendStatic(&c, path1, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsGetEntryType(FsFileSystem* fs, const char* path, FsEntryType* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 type;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->type;
        }
    }

    return rc;
}

Result fsFsOpenFile(FsFileSystem* fs, const char* path, int flags, FsFile* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->flags = flags;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            out->h = r.Handles[0];
        }
    }

    return rc;
}

Result fsFsOpenDirectory(FsFileSystem* fs, const char* path, int flags, FsDir* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->flags = flags;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            out->h = r.Handles[0];
        }
    }

    return rc;
}

Result fsFsCommit(FsFileSystem* fs) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsGetFreeSpace(FsFileSystem* fs, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 space;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->space;
        }
    }

    return rc;
}

Result fsFsGetTotalSpace(FsFileSystem* fs, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = ipcDispatch(fs->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 space;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->space;
        }
    }

    return rc;
}

void fsFsClose(FsFileSystem* fs) {
    svcCloseHandle(fs->h);
}

// IFile implementation
Result fsFileRead(FsFile* f, u64 off, void* buf, size_t len, size_t* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 zero;
        u64 offset;
        u64 read_size;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->zero = 0;
    raw->offset = off;
    raw->read_size = len;

    Result rc = ipcDispatch(f->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 bytes_read;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->bytes_read;
        }
    }

    return rc;
}

Result fsFileWrite(FsFile* f, u64 off, const void* buf, size_t len, size_t* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 zero;
        u64 offset;
        u64 write_size;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->zero = 0;
    raw->offset = off;
    raw->write_size = len;

    Result rc = ipcDispatch(f->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 bytes_written;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->bytes_written;
        }
    }

    return rc;
}

Result fsFileFlush(FsFile* f) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = ipcDispatch(f->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFileSetSize(FsFile* f, u64 sz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 size;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->size = sz;

    Result rc = ipcDispatch(f->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFileGetSize(FsFile* f, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = ipcDispatch(f->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->size;
    }

    return rc;
}

void fsFileClose(FsFile* f) {
    svcCloseHandle(f->h);
}

// IDirectory implementation
void fsDirClose(FsDir* d) {
    svcCloseHandle(d->h);
}

Result fsDirRead(FsDir* d, u64 inval, size_t* total_entries, size_t max_entries, FsDirectoryEntry *buf) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, sizeof(FsDirectoryEntry)*max_entries, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->inval = inval;

    Result rc = ipcDispatch(d->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (total_entries) *total_entries = resp->total_entries;
        }
    }

    return rc;
}

Result fsDirGetEntryCount(FsDir* d, u64* count) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = ipcDispatch(d->h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 count;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && count) *count = resp->count;
    }

    return rc;
}

