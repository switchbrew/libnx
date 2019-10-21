// Copyright 2017 plutoo
#include <string.h>
#include "service_guard.h"
#include "kernel/mutex.h"
#include "kernel/condvar.h"
#include "runtime/hosversion.h"
#include "services/fs.h"
#include "services/acc.h"

#define FS_MAX_SESSIONS 8

__attribute__((weak)) u32 __nx_fs_num_sessions = 3;

static Service g_fsSrv;

static Handle g_fsSessions[FS_MAX_SESSIONS];
static u32 g_fsSessionFreeMask;
static Mutex g_fsSessionMutex;
static CondVar g_fsSessionCondVar;
static bool g_fsSessionWaiting;

static __thread u32 g_fsPriority = FsPriority_Normal;

static int _fsGetSessionSlot(void)
{
    mutexLock(&g_fsSessionMutex);
    int slot;
    for (;;) {
        slot = __builtin_ffs(g_fsSessionFreeMask)-1;
        if (slot >= 0) break;
        g_fsSessionWaiting = true;
        condvarWait(&g_fsSessionCondVar, &g_fsSessionMutex);
    }
    g_fsSessionFreeMask &= ~(1U << slot);
    mutexUnlock(&g_fsSessionMutex);
    return slot;
}

static void _fsPutSessionSlot(int slot)
{
    mutexLock(&g_fsSessionMutex);
    g_fsSessionFreeMask |= 1U << slot;
    if (g_fsSessionWaiting) {
        g_fsSessionWaiting = false;
        condvarWakeOne(&g_fsSessionCondVar);
    }
    mutexUnlock(&g_fsSessionMutex);
}

NX_INLINE bool _fsObjectIsChild(Service* s)
{
    return s->session == g_fsSrv.session;
}

static void _fsObjectClose(Service* s)
{
    if (!_fsObjectIsChild(s)) {
        serviceClose(s);
    }
    else {
        int slot = _fsGetSessionSlot();
        uint32_t object_id = serviceGetObjectId(s);
        serviceAssumeDomain(s);
        cmifMakeCloseRequest(armGetTls(), object_id);
        svcSendSyncRequest(g_fsSessions[slot]);
        _fsPutSessionSlot(slot);
    }
}

NX_INLINE Result _fsObjectDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
) {
    int slot = -1;
    if (_fsObjectIsChild(s)) {
        slot = _fsGetSessionSlot();
        if (slot < 0) __builtin_unreachable();
        disp.target_session = g_fsSessions[slot];
        serviceAssumeDomain(s);
    }

    disp.context = g_fsPriority;
    Result rc = serviceDispatchImpl(s, request_id, in_data, in_data_size, out_data, out_data_size, disp);

    if (slot >= 0) {
        _fsPutSessionSlot(slot);
    }

    return rc;
}

#define _fsObjectDispatch(_s,_rid,...) \
    _fsObjectDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _fsObjectDispatchIn(_s,_rid,_in,...) \
    _fsObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _fsObjectDispatchOut(_s,_rid,_out,...) \
    _fsObjectDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _fsObjectDispatchInOut(_s,_rid,_in,_out,...) \
    _fsObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

NX_GENERATE_SERVICE_GUARD(fs);

Result _fsInitialize(void)
{
    if (__nx_fs_num_sessions < 1 || __nx_fs_num_sessions > FS_MAX_SESSIONS)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    Result rc = smGetService(&g_fsSrv, "fsp-srv");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_fsSrv);
    }

    if (R_SUCCEEDED(rc)) {
        u64 pid_placeholder = 0;
        serviceAssumeDomain(&g_fsSrv);
        rc = serviceDispatchIn(&g_fsSrv, 1, pid_placeholder, .in_send_pid = true);
    }

    if (R_SUCCEEDED(rc)) {
        g_fsSessionFreeMask = (1U << __nx_fs_num_sessions) - 1U;
        g_fsSessions[0] = g_fsSrv.session;
    }

    for (u32 i = 1; R_SUCCEEDED(rc) && i < __nx_fs_num_sessions; i ++) {
        rc = cmifCloneCurrentObject(g_fsSessions[0], &g_fsSessions[i]);
    }

    return rc;
}

void _fsCleanup(void)
{
    // Close extra sessions
    g_fsSessions[0] = INVALID_HANDLE;
    for (u32 i = 1; i < __nx_fs_num_sessions; i ++) {
        if (g_fsSessions[i] != INVALID_HANDLE) {
            cmifMakeCloseRequest(armGetTls(), 0);
            svcSendSyncRequest(g_fsSessions[i]);
            g_fsSessions[i] = INVALID_HANDLE;
        }
    }

    // We can't assume g_fsSrv is a domain here because serviceConvertToDomain might have failed
    serviceClose(&g_fsSrv);
}

Service* fsGetServiceSession(void) {
    return &g_fsSrv;
}

void fsSetPriority(FsPriority prio)
{
    if (hosversionAtLeast(5,0,0))
        g_fsPriority = prio;
}

//-----------------------------------------------------------------------------
// IFileSystemProxy
//-----------------------------------------------------------------------------

Result fsOpenBisStorage(FsStorage* out, FsBisStorageId partitionId) {
    return _fsObjectDispatchIn(&g_fsSrv, 12, partitionId,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenBisFileSystem(FsFileSystem* out, FsBisStorageId partitionId, const char* string) {
    char tmpstr[FS_MAX_PATH] = {0};
    strncpy(tmpstr, string, sizeof(tmpstr)-1);

    return _fsObjectDispatchIn(&g_fsSrv, 11, partitionId,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { tmpstr, sizeof(tmpstr) } },
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsCreateSaveDataFileSystemBySystemSaveDataId(const FsSave* save, const FsSaveCreate* create) {
    const struct {
        FsSave save;
        FsSaveCreate create;
    } in = { *save, *create };

    return _fsObjectDispatchIn(&g_fsSrv, 23, in);
}

Result fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId saveDataSpaceId, u64 saveID) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 saveDataSpaceId;
        u64 saveID;
    } in = { (u8)saveDataSpaceId, saveID };

    return _fsObjectDispatchIn(&g_fsSrv, 25, in);
}

Result fsOpenSdCardFileSystem(FsFileSystem* out) {
    return _fsObjectDispatch(&g_fsSrv, 18,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenSaveDataFileSystem(FsFileSystem* out, FsSaveDataSpaceId saveDataSpaceId, const FsSave *save) {
    const struct {
        u8 saveDataSpaceId;
        FsSave save;
    } in = { (u8)saveDataSpaceId, *save };

    return _fsObjectDispatchIn(&g_fsSrv, 51, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenSaveDataFileSystemBySystemSaveDataId(FsFileSystem* out, FsSaveDataSpaceId saveDataSpaceId, const FsSave *save) {
    const struct {
        u8 saveDataSpaceId;
        FsSave save;
    } in = { (u8)saveDataSpaceId, *save };

    return _fsObjectDispatchIn(&g_fsSrv, 52, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

static Result _fsOpenSaveDataInfoReader(FsSaveDataInfoReader* out) {
    return _fsObjectDispatch(&g_fsSrv, 60,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

static Result _fsOpenSaveDataInfoReaderBySaveDataSpaceId(FsSaveDataInfoReader* out, FsSaveDataSpaceId saveDataSpaceId) {
    u8 in = (u8)saveDataSpaceId;
    return _fsObjectDispatchIn(&g_fsSrv, 61, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );

}

Result fsOpenSaveDataInfoReader(FsSaveDataInfoReader* out, FsSaveDataSpaceId saveDataSpaceId) {
    if (saveDataSpaceId == FsSaveDataSpaceId_All) {
        return _fsOpenSaveDataInfoReader(out);
    } else {
        return _fsOpenSaveDataInfoReaderBySaveDataSpaceId(out, saveDataSpaceId);
    }
}

Result fsOpenContentStorageFileSystem(FsFileSystem* out, FsContentStorageId content_storage_id) {
    return _fsObjectDispatchIn(&g_fsSrv, 110, content_storage_id,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenCustomStorageFileSystem(FsFileSystem* out, FsCustomStorageId custom_storage_id) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _fsObjectDispatchIn(&g_fsSrv, 130, custom_storage_id,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenDataStorageByCurrentProcess(FsStorage* out) {
    return _fsObjectDispatch(&g_fsSrv, 200,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenDataStorageByDataId(FsStorage* out, u64 dataId, FsStorageId storageId) {
    const struct {
        FsStorageId storage_id;
        u64 data_id;
    } in = { storageId, dataId };

    return _fsObjectDispatchIn(&g_fsSrv, 202, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenDeviceOperator(FsDeviceOperator* out) {
    return _fsObjectDispatch(&g_fsSrv, 400,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenSdCardDetectionEventNotifier(FsEventNotifier* out) {
    return _fsObjectDispatch(&g_fsSrv, 500,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsGetRightsIdByPath(const char* path, FsRightsId* out_rights_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);

    return _fsObjectDispatchOut(&g_fsSrv, 609, *out_rights_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { send_path, sizeof(send_path) } },
    );
}

Result fsGetRightsIdAndKeyGenerationByPath(const char* path, u8* out_key_generation, FsRightsId* out_rights_id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);

    struct {
        u8 key_generation;
        u8 padding[0x7];
        FsRightsId rights_id;
    } out;

    Result rc = _fsObjectDispatchOut(&g_fsSrv, 610, out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { send_path, sizeof(send_path) } },
    );

    if (R_SUCCEEDED(rc)) {
        if (out_key_generation) *out_key_generation = out.key_generation;
        if (out_rights_id) *out_rights_id = out.rights_id;
    }

    return rc;
}

Result fsDisableAutoSaveDataCreation(void) {
    return _fsObjectDispatch(&g_fsSrv, 1003);
}

Result fsIsExFatSupported(bool* out)
{
    if (hosversionBefore(2,0,0)) {
        *out = false;
        return 0;
    }

    return _fsObjectDispatchOut(&g_fsSrv, 27, *out);
}

Result fsOpenGameCardFileSystem(FsFileSystem* out, const FsGameCardHandle* handle, FsGameCardPartiton partition) {
    const struct {
        FsGameCardHandle handle;
        FsGameCardPartiton partition;
    } in = { *handle, partition };

    return _fsObjectDispatchIn(&g_fsSrv, 31, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(void* buf, size_t len, FsSaveDataSpaceId saveDataSpaceId, u64 saveID) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 saveDataSpaceId;
        u64 saveID;
    } in = { (u8)saveDataSpaceId, saveID };

    return _fsObjectDispatchIn(&g_fsSrv, 57, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, len } },
    );
}

Result fsReadSaveDataFileSystemExtraData(void* buf, size_t len, u64 saveID) {
    return _fsObjectDispatchIn(&g_fsSrv, 58, saveID,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, len } },
    );
}

Result fsWriteSaveDataFileSystemExtraData(const void* buf, size_t len, FsSaveDataSpaceId saveDataSpaceId, u64 saveID) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 saveDataSpaceId;
        u64 saveID;
    } in = { (u8)saveDataSpaceId, saveID };

    return _fsObjectDispatchIn(&g_fsSrv, 59, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, len } },
    );
}

Result fsExtendSaveDataFileSystem(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, s64 dataSize, s64 journalSize) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 saveDataSpaceId;
        u64 saveID;
        s64 dataSize;
        s64 journalSize;
    } in = { (u8)saveDataSpaceId, saveID, dataSize, journalSize };

    return _fsObjectDispatchIn(&g_fsSrv, 32, in);
}

Result fsSetGlobalAccessLogMode(u32 mode) {
    return _fsObjectDispatchIn(&g_fsSrv, 1004, mode);
}

Result fsGetGlobalAccessLogMode(u32* out_mode) {
    return _fsObjectDispatchOut(&g_fsSrv, 1005, *out_mode);
}

// Wrapper(s) for fsCreateSaveDataFileSystemBySystemSaveDataId.
Result fsCreate_SystemSaveDataWithOwner(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, AccountUid *userID, u64 ownerId, u64 size, u64 journalSize, u32 flags) {
    FsSave save = {
        .userID = *userID,
        .saveID = saveID,
    };
    FsSaveCreate create = {
        .size = size,
        .journalSize = journalSize,
        .blockSize = 0x4000,
        .ownerId = ownerId,
        .flags = flags,
        .saveDataSpaceId = saveDataSpaceId,
    };

    return fsCreateSaveDataFileSystemBySystemSaveDataId(&save, &create);
}

Result fsCreate_SystemSaveData(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, u64 size, u64 journalSize, u32 flags) {
    return fsCreate_SystemSaveDataWithOwner(saveDataSpaceId, saveID, 0, 0, size, journalSize, flags);
}

// Wrapper(s) for fsOpenSaveDataFileSystem.
Result fsOpen_SaveData(FsFileSystem* out, u64 titleID, AccountUid *userID) {
    FsSave save;

    memset(&save, 0, sizeof(save));
    save.titleID = titleID;
    save.userID = *userID;
    save.saveDataType = FsSaveDataType_SaveData;

    return fsOpenSaveDataFileSystem(out, FsSaveDataSpaceId_NandUser, &save);
}

Result fsOpen_SystemSaveData(FsFileSystem* out, u64 saveID) {
    FsSave save;

    memset(&save, 0, sizeof(save));
    save.saveID = saveID;
    save.saveDataType = FsSaveDataType_SystemSaveData;

    return fsOpenSaveDataFileSystemBySystemSaveDataId(out, FsSaveDataSpaceId_NandSystem, &save);
}

Result fsOpenFileSystem(FsFileSystem* out, FsFileSystemType fsType, const char* contentPath) {
    return fsOpenFileSystemWithId(out, 0, fsType, contentPath);
}

static Result _fsOpenFileSystemWithId(FsFileSystem* out, u64 titleId, FsFileSystemType fsType, const char* contentPath) {
    const struct {
        FsFileSystemType fsType;
        u64 titleId;
    } in = { fsType, titleId };

    return _fsObjectDispatchIn(&g_fsSrv, 8, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { contentPath, FS_MAX_PATH } },
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

static Result _fsOpenFileSystem(FsFileSystem* out, FsFileSystemType fsType, const char* contentPath) {
    return _fsObjectDispatchIn(&g_fsSrv, 0, fsType,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { contentPath, FS_MAX_PATH } },
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsOpenFileSystemWithId(FsFileSystem* out, u64 titleId, FsFileSystemType fsType, const char* contentPath) {
    char sendStr[FS_MAX_PATH] = {0};
    strncpy(sendStr, contentPath, sizeof(sendStr)-1);

    if (hosversionAtLeast(2,0,0))
        return _fsOpenFileSystemWithId(out, titleId, fsType, sendStr);
    else
        return _fsOpenFileSystem(out, fsType, sendStr);
}

Result fsOpenFileSystemWithPatch(FsFileSystem* out, u64 titleId, FsFileSystemType fsType) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        FsFileSystemType fsType;
        u64 titleId;
    } in = { fsType, titleId };

    return _fsObjectDispatchIn(&g_fsSrv, 7, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

//-----------------------------------------------------------------------------
// IFileSystem
//-----------------------------------------------------------------------------

Result fsFsCreateFile(FsFileSystem* fs, const char* path, u64 size, u32 option) {
    const struct {
        u32 option;
        u64 size;
    } in = { option, size };

    return _fsObjectDispatchIn(&fs->s, 0, in,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { path, FS_MAX_PATH } },
    );
}

static Result _fsFsCmdWithInPath(FsFileSystem* fs, const char* path, u32 cmd_id) {
    return _fsObjectDispatch(&fs->s, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { path, FS_MAX_PATH } },
    );
}

Result fsFsDeleteFile(FsFileSystem* fs, const char* path) {
    return _fsFsCmdWithInPath(fs, path, 1);
}

Result fsFsCreateDirectory(FsFileSystem* fs, const char* path) {
    return _fsFsCmdWithInPath(fs, path, 2);
}

Result fsFsDeleteDirectory(FsFileSystem* fs, const char* path) {
    return _fsFsCmdWithInPath(fs, path, 3);
}

Result fsFsDeleteDirectoryRecursively(FsFileSystem* fs, const char* path) {
    return _fsFsCmdWithInPath(fs, path, 4);
}

static Result _fsFsCmdWithTwoInPaths(FsFileSystem* fs, const char* cur_path, const char* new_path, u32 cmd_id) {
    return _fsObjectDispatch(&fs->s, cmd_id,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { cur_path, FS_MAX_PATH },
            { new_path, FS_MAX_PATH },
        },
    );
}

Result fsFsRenameFile(FsFileSystem* fs, const char* cur_path, const char* new_path) {
    return _fsFsCmdWithTwoInPaths(fs, cur_path, new_path, 5);
}

Result fsFsRenameDirectory(FsFileSystem* fs, const char* cur_path, const char* new_path) {
    return _fsFsCmdWithTwoInPaths(fs, cur_path, new_path, 6);
}

Result fsFsGetEntryType(FsFileSystem* fs, const char* path, FsDirEntryType* out) {
    return _fsObjectDispatchOut(&fs->s, 7, *out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { path, FS_MAX_PATH } },
    );
}

static Result _fsFsOpenCommon(FsFileSystem* fs, const char* path, u32 flags, Service* out, u32 cmd_id) {
    return _fsObjectDispatchIn(&fs->s, cmd_id, flags,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { path, FS_MAX_PATH } },
        .out_num_objects = 1,
        .out_objects = out,
    );
}

Result fsFsOpenFile(FsFileSystem* fs, const char* path, u32 mode, FsFile* out) {
    return _fsFsOpenCommon(fs, path, mode, &out->s, 8);
}

Result fsFsOpenDirectory(FsFileSystem* fs, const char* path, u32 mode, FsDir* out) {
    return _fsFsOpenCommon(fs, path, mode, &out->s, 9);
}

Result fsFsCommit(FsFileSystem* fs) {
    return _fsObjectDispatch(&fs->s, 10);
}

static Result _fsFsCmdWithInPathAndOutU64(FsFileSystem* fs, const char* path, u64* out, u32 cmd_id) {
    return _fsObjectDispatchOut(&fs->s, cmd_id, *out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { path, FS_MAX_PATH } },
    );
}

Result fsFsGetFreeSpace(FsFileSystem* fs, const char* path, u64* out) {
    return _fsFsCmdWithInPathAndOutU64(fs, path, out, 11);
}

Result fsFsGetTotalSpace(FsFileSystem* fs, const char* path, u64* out) {
    return _fsFsCmdWithInPathAndOutU64(fs, path, out, 12);
}

Result fsFsCleanDirectoryRecursively(FsFileSystem* fs, const char* path) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _fsFsCmdWithInPath(fs, path, 13);
}

Result fsFsGetFileTimeStampRaw(FsFileSystem* fs, const char* path, FsTimeStampRaw *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _fsObjectDispatchOut(&fs->s, 14, *out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { path, FS_MAX_PATH } },
    );
}

Result fsFsQueryEntry(FsFileSystem* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryType query_type) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, sizeof(send_path)-1);

    return _fsObjectDispatchIn(&fs->s, 15, query_type,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer  | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In  | SfBufferAttr_HipcMapTransferAllowsNonSecure,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure,
        },
        .buffers = {
            { send_path, sizeof(send_path) },
            { in,        in_size           },
            { out,       out_size          },
        },
    );
}

Result fsFsSetArchiveBit(FsFileSystem* fs, const char *path) {
    return fsFsQueryEntry(fs, NULL, 0, NULL, 0, path, FsFileSystemQueryType_SetArchiveBit);
}

void fsFsClose(FsFileSystem* fs) {
    _fsObjectClose(&fs->s);
}

//-----------------------------------------------------------------------------
// IFile
//-----------------------------------------------------------------------------

Result fsFileRead(FsFile* f, u64 off, void* buf, u64 read_size, u32 option, u64* bytes_read) {
    const struct {
        u32 option;
        u64 offset;
        u64 read_size;
    } in = { option, off, read_size };

    return _fsObjectDispatchInOut(&f->s, 0, in, *bytes_read,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buf, read_size } },
    );
}

Result fsFileWrite(FsFile* f, u64 off, const void* buf, u64 write_size, u32 option) {
    const struct {
        u32 option;
        u64 offset;
        u64 write_size;
    } in = { option, off, write_size };

    return _fsObjectDispatchIn(&f->s, 1, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buf, write_size } },
    );
}

Result fsFileFlush(FsFile* f) {
    return _fsObjectDispatch(&f->s, 2);
}

Result fsFileSetSize(FsFile* f, u64 sz) {
    return _fsObjectDispatchIn(&f->s, 3, sz);
}

Result fsFileGetSize(FsFile* f, u64* out) {
    return _fsObjectDispatchOut(&f->s, 4, *out);
}

Result fsFileOperateRange(FsFile* f, FsOperationId op_id, u64 off, u64 len, FsRangeInfo* out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 op_id;
        u64 off;
        u64 len;
    } in = { op_id, off, len };

    return _fsObjectDispatchInOut(&f->s, 5, in, *out);
}

void fsFileClose(FsFile* f) {
    _fsObjectClose(&f->s);
}

// IDirectory implementation
void fsDirClose(FsDir* d) {
    _fsObjectClose(&d->s);
}

Result fsDirRead(FsDir* d, u64 inval, u64* total_entries, size_t max_entries, FsDirectoryEntry *buf) {
    return _fsObjectDispatchInOut(&d->s, 0, inval, *total_entries,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, sizeof(FsDirectoryEntry)*max_entries } },
    );
}

Result fsDirGetEntryCount(FsDir* d, u64* count) {
    return _fsObjectDispatchOut(&d->s, 1, *count);
}

//-----------------------------------------------------------------------------
// IStorage
//-----------------------------------------------------------------------------

Result fsStorageRead(FsStorage* s, u64 off, void* buf, u64 read_size) {
    const struct {
        u64 offset;
        u64 read_size;
    } in = { off, read_size };

    return _fsObjectDispatchIn(&s->s, 0, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buf, read_size } },
    );
}

Result fsStorageWrite(FsStorage* s, u64 off, const void* buf, u64 write_size) {
    const struct {
        u64 offset;
        u64 write_size;
    } in = { off, write_size };

    return _fsObjectDispatchIn(&s->s, 1, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buf, write_size } },
    );
}

Result fsStorageFlush(FsStorage* s) {
    return _fsObjectDispatch(&s->s, 2);
}

Result fsStorageSetSize(FsStorage* s, u64 sz) {
    return _fsObjectDispatchIn(&s->s, 3, sz);
}

Result fsStorageGetSize(FsStorage* s, u64* out) {
    return _fsObjectDispatchOut(&s->s, 4, *out);
}

Result fsStorageOperateRange(FsStorage* s, FsOperationId op_id, u64 off, u64 len, FsRangeInfo* out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 op_id;
        u64 off;
        u64 len;
    } in = { op_id, off, len };

    return _fsObjectDispatchInOut(&s->s, 5, in, *out);
}

void fsStorageClose(FsStorage* s) {
    _fsObjectClose(&s->s);
}

//-----------------------------------------------------------------------------
// ISaveDataInfoReader
//-----------------------------------------------------------------------------

// Actually called ReadSaveDataInfo
Result fsSaveDataInfoReaderRead(FsSaveDataInfoReader *s, FsSaveDataInfo* buf, size_t max_entries, u64* total_entries) {
    return _fsObjectDispatchOut(&s->s, 0, *total_entries,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, sizeof(FsSaveDataInfo)*max_entries } },
    );
}

void fsSaveDataInfoReaderClose(FsSaveDataInfoReader* s) {
    _fsObjectClose(&s->s);
}

//-----------------------------------------------------------------------------
// IEventNotifier
//-----------------------------------------------------------------------------

Result fsEventNotifierGetEventHandle(FsEventNotifier* e, Event* out, bool autoclear) {
    Handle event = INVALID_HANDLE;
    Result rc = _fsObjectDispatch(&e->s, 0,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out, event, autoclear);

    return rc;
}

void fsEventNotifierClose(FsEventNotifier* e) {
    _fsObjectClose(&e->s);
}

//-----------------------------------------------------------------------------
// IDeviceOperator
//-----------------------------------------------------------------------------

Result fsDeviceOperatorIsSdCardInserted(FsDeviceOperator* d, bool* out) {
    return _fsObjectDispatchOut(&d->s, 0, *out);
}

Result fsDeviceOperatorIsGameCardInserted(FsDeviceOperator* d, bool* out) {
    return _fsObjectDispatchOut(&d->s, 200, *out);
}

Result fsDeviceOperatorGetGameCardHandle(FsDeviceOperator* d, FsGameCardHandle* out) {
    return _fsObjectDispatchOut(&d->s, 202, *out);
}

Result fsDeviceOperatorGetGameCardAttribute(FsDeviceOperator* d, const FsGameCardHandle* handle, u8 *out) {
    return _fsObjectDispatchInOut(&d->s, 205, *handle, *out);
}

void fsDeviceOperatorClose(FsDeviceOperator* d) {
    _fsObjectClose(&d->s);
}
