// Copyright 2017 plutoo
#include <string.h>
#include "service_guard.h"
#include "kernel/mutex.h"
#include "kernel/condvar.h"
#include "runtime/hosversion.h"
#include "services/fs.h"

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

Result fsOpenBisStorage(FsStorage* out, FsBisStorageId partitionId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PartitionId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->PartitionId = partitionId;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenBisFileSystem(FsFileSystem* out, FsBisStorageId partitionId, const char* string) {
    IpcCommand c;
    ipcInitialize(&c);

    char tmpstr[FS_MAX_PATH] = {0};
    strncpy(tmpstr, string, sizeof(tmpstr)-1);
    ipcAddSendStatic(&c, tmpstr, sizeof(tmpstr), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PartitionId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->PartitionId = partitionId;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsCreateSaveDataFileSystemBySystemSaveDataId(const FsSave* save, const FsSaveCreate* create) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        FsSave save;
        FsSaveCreate create;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;
    memcpy(&raw->save, save, sizeof(FsSave));
    memcpy(&raw->create, create, sizeof(FsSaveCreate));

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId saveDataSpaceId, u64 saveID) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 saveDataSpaceId;
        u64 saveID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 25;
    raw->saveDataSpaceId = saveDataSpaceId;
    raw->saveID = saveID;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsMountSdcard(FsFileSystem* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsMountSaveData(FsFileSystem* out, u8 inval, FsSave *save) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;//Actually u8.
        FsSave save;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 51;
    raw->inval = (u64)inval;
    memcpy(&raw->save, save, sizeof(FsSave));

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsMountSystemSaveData(FsFileSystem* out, u8 inval, FsSave *save) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;//Actually u8.
        FsSave save;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 52;
    raw->inval = (u64)inval;
    memcpy(&raw->save, save, sizeof(FsSave));

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenSaveDataIterator(FsSaveDataIterator* out, s32 saveDataSpaceId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    struct {
        u64 magic;
        u64 cmd_id;
        u8 saveDataSpaceId;
    } *raw2;

    if (saveDataSpaceId == FsSaveDataSpaceId_All) {
        raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 60;
    }
    else {
        raw2 = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw2));

        raw2->magic = SFCI_MAGIC;
        raw2->cmd_id = 61;
        raw2->saveDataSpaceId = saveDataSpaceId;
    }

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenContentStorageFileSystem(FsFileSystem* out, FsContentStorageId content_storage_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 content_storage_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 110;
    raw->content_storage_id = content_storage_id;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenCustomStorageFileSystem(FsFileSystem* out, FsCustomStorageId custom_storage_id) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 custom_storage_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 130;
    raw->custom_storage_id = custom_storage_id;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenDataStorageByCurrentProcess(FsStorage* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 200;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenDataStorageByDataId(FsStorage* out, u64 dataId, FsStorageId storageId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 storage_id;
        u64 data_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;
    raw->storage_id = storageId;
    raw->data_id = dataId;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenDeviceOperator(FsDeviceOperator* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 400;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenSdCardDetectionEventNotifier(FsEventNotifier* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 500;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsGetRightsIdByPath(const char* path, FsRightsId* out_rights_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 609;

    strncpy(send_path, path, FS_MAX_PATH-1);
    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsRightsId rights_id;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_rights_id) *out_rights_id = resp->rights_id;
        }
    }

    return rc;
}

Result fsGetRightsIdAndKeyGenerationByPath(const char* path, u8* out_key_generation, FsRightsId* out_rights_id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 610;

    strncpy(send_path, path, FS_MAX_PATH-1);
    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 key_generation;
            u8 padding[0x7];
            FsRightsId rights_id;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_key_generation) *out_key_generation = resp->key_generation;
            if (out_rights_id) *out_rights_id = resp->rights_id;
        }
    }

    return rc;
}

Result fsDisableAutoSaveDataCreation(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1003;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsIsExFatSupported(bool* out)
{
    if (hosversionBefore(2,0,0)) {
        *out = false;
        return 0;
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 27;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8  exfat;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->exfat;
        }
    }

    return rc;
}

Result fsOpenGameCardFileSystem(FsFileSystem* out, const FsGameCardHandle* handle, FsGameCardPartiton partition) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 handle;
        u32 partition;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 31;
    raw->handle = handle->value;
    raw->partition = partition;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(void* buf, size_t len, FsSaveDataSpaceId saveDataSpaceId, u64 saveID) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 saveDataSpaceId;
        u64 saveID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 57;
    raw->saveDataSpaceId = saveDataSpaceId;
    raw->saveID = saveID;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsReadSaveDataFileSystemExtraData(void* buf, size_t len, u64 saveID) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 saveID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 58;
    raw->saveID = saveID;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsWriteSaveDataFileSystemExtraData(const void* buf, size_t len, FsSaveDataSpaceId saveDataSpaceId, u64 saveID) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 saveDataSpaceId;
        u64 saveID;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 59;
    raw->saveDataSpaceId = saveDataSpaceId;
    raw->saveID = saveID;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsExtendSaveDataFileSystem(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, s64 dataSize, s64 journalSize) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 saveDataSpaceId;
        u64 saveID;
        s64 dataSize;
        s64 journalSize;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 32;
    raw->saveDataSpaceId = saveDataSpaceId;
    raw->saveID = saveID;
    raw->dataSize = dataSize;
    raw->saveID = saveID;
    raw->journalSize = journalSize;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsSetGlobalAccessLogMode(u32 mode) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 mode;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1004;
    raw->mode = mode;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsGetGlobalAccessLogMode(u32* out_mode) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1005;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 mode;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_mode = resp->mode;
        }
    }

    return rc;
}

// Wrapper(s) for fsCreateSaveDataFileSystemBySystemSaveDataId.
Result fsCreate_SystemSaveDataWithOwner(FsSaveDataSpaceId saveDataSpaceId, u64 saveID, u128 userID, u64 ownerId, u64 size, u64 journalSize, u32 flags) {
    FsSave save = {
        .userID = userID,
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

// Wrapper(s) for fsMountSaveData.
Result fsMount_SaveData(FsFileSystem* out, u64 titleID, u128 userID) {
    FsSave save;

    memset(&save, 0, sizeof(save));
    save.titleID = titleID;
    save.userID = userID;
    save.saveDataType = FsSaveDataType_SaveData;

    return fsMountSaveData(out, FsSaveDataSpaceId_NandUser, &save);
}

Result fsMount_SystemSaveData(FsFileSystem* out, u64 saveID) {
    FsSave save;

    memset(&save, 0, sizeof(save));
    save.saveID = saveID;
    save.saveDataType = FsSaveDataType_SystemSaveData;

    return fsMountSystemSaveData(out, FsSaveDataSpaceId_NandSystem, &save);
}

Result fsOpenFileSystem(FsFileSystem* out, FsFileSystemType fsType, const char* contentPath) {
    return fsOpenFileSystemWithId(out, 0, fsType, contentPath);
}

Result fsOpenFileSystemWithId(FsFileSystem* out, u64 titleId, FsFileSystemType fsType, const char* contentPath) {
    char sendStr[FS_MAX_PATH] = {0};
    strncpy(sendStr, contentPath, sizeof(sendStr)-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, sendStr, sizeof(sendStr), 0);

    if (hosversionAtLeast(2,0,0)) {
        struct {
            u64 magic;
            u64 cmd_id;
            u32 fsType;
            u64 titleId;
        } *raw;

        raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 8;
        raw->fsType = fsType;
        raw->titleId = titleId;
    }
    else {
        struct {
            u64 magic;
            u64 cmd_id;
            u32 fsType;
        } *raw;

        raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;
        raw->fsType = fsType;
    }

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

Result fsOpenFileSystemWithPatch(FsFileSystem* out, u64 titleId, FsFileSystemType fsType) {
    if (hosversionBefore(2,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 fsType;
        u64 titleId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;
    raw->fsType = fsType;
    raw->titleId = titleId;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsSrv, &r, 0);
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
        u32 flags;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->flags = flags;
    raw->size = size;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsRenameFile(FsFileSystem* fs, const char* cur_path, const char* new_path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, cur_path, FS_MAX_PATH, 0);
    ipcAddSendStatic(&c, new_path, FS_MAX_PATH, 1);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsRenameDirectory(FsFileSystem* fs, const char* cur_path, const char* new_path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, cur_path, FS_MAX_PATH, 0);
    ipcAddSendStatic(&c, new_path, FS_MAX_PATH, 1);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 type;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &fs->s, &r, 0);
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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &fs->s, &r, 0);
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

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsGetFreeSpace(FsFileSystem* fs, const char* path, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 space;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->space;
        }
    }

    return rc;
}

Result fsFsGetTotalSpace(FsFileSystem* fs, const char* path, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 space;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->space;
        }
    }

    return rc;
}

Result fsFsCleanDirectoryRecursively(FsFileSystem* fs, const char* path) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsGetFileTimeStampRaw(FsFileSystem* fs, const char* path, FsTimeStampRaw *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, sizeof(send_path)-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, sizeof(send_path), 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsTimeStampRaw out;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result fsFsQueryEntry(FsFileSystem* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryType query_type) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, sizeof(send_path)-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, sizeof(send_path), 0);
    ipcAddSendBuffer(&c, in, in_size, BufferType_Type1);
    ipcAddRecvBuffer(&c, out, out_size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 query_type;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
    raw->query_type = query_type;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsSetArchiveBit(FsFileSystem* fs, const char *path) {
    return fsFsQueryEntry(fs, NULL, 0, NULL, 0, path, FsFileSystemQueryType_SetArchiveBit);
}

void fsFsClose(FsFileSystem* fs) {
    _fsObjectClose(&fs->s);
}

// IFile implementation
Result fsFileRead(FsFile* f, u64 off, void* buf, size_t len, u32 option, size_t* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 option;
        u64 offset;
        u64 read_size;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->option = option;
    raw->offset = off;
    raw->read_size = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 bytes_read;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->bytes_read;
        }
    }

    return rc;
}

Result fsFileWrite(FsFile* f, u64 off, const void* buf, size_t len, u32 option) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 option;
        u64 offset;
        u64 write_size;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->option = option;
    raw->offset = off;
    raw->write_size = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
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

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->size = sz;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->size;
    }

    return rc;
}

Result fsFileOperateRange(FsFile* f, FsOperationId op_id, u64 off, size_t len, FsRangeInfo* out) {
    if (hosversionBefore(4,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 op_id;
        u64 off;
        u64 len;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->op_id = op_id;
    raw->off = off;
    raw->len = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsRangeInfo range_info;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->range_info;
    }

    return rc;

}

void fsFileClose(FsFile* f) {
    _fsObjectClose(&f->s);
}

// IDirectory implementation
void fsDirClose(FsDir* d) {
    _fsObjectClose(&d->s);
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

    raw = serviceIpcPrepareHeader(&d->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(&d->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&d->s, &r, sizeof(*resp));
        resp = r.Raw;

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

    raw = serviceIpcPrepareHeader(&d->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&d->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 count;
        } *resp;

        serviceIpcParse(&d->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && count) *count = resp->count;
    }

    return rc;
}

// IStorage implementation
Result fsStorageRead(FsStorage* s, u64 off, void* buf, size_t len) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
        u64 read_size;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->offset = off;
    raw->read_size = len;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsStorageWrite(FsStorage* s, u64 off, const void* buf, size_t len) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
        u64 write_size;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->offset = off;
    raw->write_size = len;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsStorageFlush(FsStorage* s) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsStorageSetSize(FsStorage* s, u64 sz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->size = sz;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsStorageGetSize(FsStorage* s, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->size;
    }

    return rc;
}

Result fsStorageOperateRange(FsStorage* s, FsOperationId op_id, u64 off, size_t len, FsRangeInfo* out) {
    if (hosversionBefore(4,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 op_id;
        u64 off;
        u64 len;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->op_id = op_id;
    raw->off = off;
    raw->len = len;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsRangeInfo range_info;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->range_info;
    }

    return rc;
}

void fsStorageClose(FsStorage* s) {
    _fsObjectClose(&s->s);
}

// ISaveDataInfoReader
Result fsSaveDataIteratorRead(FsSaveDataIterator *s, FsSaveDataInfo* buf, size_t max_entries, size_t* total_entries) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, sizeof(FsSaveDataInfo)*max_entries, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (total_entries) *total_entries = resp->total_entries;
        }
    }

    return rc;
}

void fsSaveDataIteratorClose(FsSaveDataIterator* s) {
    _fsObjectClose(&s->s);
}

// IEventNotifier
Result fsEventNotifierGetEventHandle(FsEventNotifier* e, Handle* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&e->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&e->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&e->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = r.Handles[0];
        }
    }

    return rc;
}

void fsEventNotifierClose(FsEventNotifier* e) {
    _fsObjectClose(&e->s);
}

// IDeviceOperator
static Result _fsDeviceOperatorCheckInserted(FsDeviceOperator* d, u32 cmd_id, bool* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&d->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&d->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 is_inserted;
        } *resp;

        serviceIpcParse(&d->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->is_inserted != 0;
        }
    }

    return rc;
}

Result fsDeviceOperatorIsSdCardInserted(FsDeviceOperator* d, bool* out) {
    return _fsDeviceOperatorCheckInserted(d, 0, out);
}

Result fsDeviceOperatorIsGameCardInserted(FsDeviceOperator* d, bool* out) {
    return _fsDeviceOperatorCheckInserted(d, 200, out);
}

Result fsDeviceOperatorGetGameCardHandle(FsDeviceOperator* d, FsGameCardHandle* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&d->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;

    Result rc = serviceIpcDispatch(&d->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 handle;
        } *resp;

        serviceIpcParse(&d->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            out->value = resp->handle;
        }
    }

    return rc;
}

Result fsDeviceOperatorGetGameCardAttribute(FsDeviceOperator* d, const FsGameCardHandle* handle, u8 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 handle;
    } *raw;

    raw = serviceIpcPrepareHeader(&d->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 205;
    raw->handle = handle->value;

    Result rc = serviceIpcDispatch(&d->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 attr;
        } *resp;

        serviceIpcParse(&d->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->attr;
        }
    }

    return rc;
}

void fsDeviceOperatorClose(FsDeviceOperator* d) {
    _fsObjectClose(&d->s);
}
