#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/sm.h"
#include "services/ns.h"

static Service g_nsAppManSrv, g_nsGetterSrv, g_nsvmSrv, g_nsdevSrv;
static u64 g_nsRefCnt, g_nsvmRefCnt, g_nsdevRefCnt;

static Result _nsGetInterface(Service* srv_out, u64 cmd_id);

Result nsInitialize(void)
{
    Result rc=0;

    atomicIncrement64(&g_nsRefCnt);

    if (serviceIsActive(&g_nsGetterSrv) || serviceIsActive(&g_nsAppManSrv))
        return 0;

    if(hosversionBefore(3,0,0))
        return smGetService(&g_nsAppManSrv, "ns:am");

    rc = smGetService(&g_nsGetterSrv, "ns:am2");//TODO: Support the other services?(Only useful when ns:am2 isn't accessible)
    if (R_FAILED(rc)) return rc;

    rc = _nsGetInterface(&g_nsAppManSrv, 7996);

    if (R_FAILED(rc)) serviceClose(&g_nsGetterSrv);

    return rc;
}

void nsExit(void)
{
    if (atomicDecrement64(&g_nsRefCnt) == 0) {
        serviceClose(&g_nsAppManSrv);
        if(hosversionBefore(3,0,0)) return;

        serviceClose(&g_nsGetterSrv);
    }
}

Result nsdevInitialize(void) {
    atomicIncrement64(&g_nsdevRefCnt);

    if (serviceIsActive(&g_nsdevSrv))
        return 0;

    return smGetService(&g_nsdevSrv, "ns:dev");
}

void nsdevExit(void) {
    if (atomicDecrement64(&g_nsdevRefCnt) == 0)
        serviceClose(&g_nsdevSrv);
}

static Result _nsGetInterface(Service* srv_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_nsGetterSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(srv_out, r.Handles[0]);
        }
    }

    return rc;
}

Result nsListApplicationRecord(NsApplicationRecord* buffer, size_t size, size_t entry_offset, size_t* out_entrycount)
{
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 entry_offset;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->entry_offset = entry_offset;

    Result rc = serviceIpcDispatch(&g_nsAppManSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 entry_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out_entrycount) *out_entrycount = resp->entry_count;
    }

    return rc;
}

Result nsListApplicationContentMetaStatus(u64 titleID, u32 index, NsApplicationContentMetaStatus* buffer, size_t size, size_t* out_entrycount)
{
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 index;
        u64 titleID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 601;
    raw->index = index;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_nsAppManSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 entry_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out_entrycount) *out_entrycount = resp->entry_count;
    }

    return rc;
}

Result nsGetApplicationControlData(u8 flag, u64 titleID, NsApplicationControlData* buffer, size_t size, size_t* actual_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
        u64 titleID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 400;
    raw->flag = flag;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_nsAppManSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 actual_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && actual_size) *actual_size = resp->actual_size;
    }

    return rc;
}

Result nsGetTotalSpaceSize(FsStorageId storage_id, u64 *size)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 storage_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 47;
    raw->storage_id = storage_id;

    Result rc = serviceIpcDispatch(&g_nsAppManSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

Result nsGetFreeSpaceSize(FsStorageId storage_id, u64 *size)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 storage_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 48;
    raw->storage_id = storage_id;

    Result rc = serviceIpcDispatch(&g_nsAppManSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

Result nsvmInitialize(void)
{
    if (hosversionBefore(3,0,0))
        return 0;

    atomicIncrement64(&g_nsvmRefCnt);

    if (serviceIsActive(&g_nsvmSrv))
        return 0;

    return smGetService(&g_nsvmSrv, "ns:vm");
}

void nsvmExit(void)
{
    if (hosversionBefore(3,0,0))
        return;

    if (atomicDecrement64(&g_nsvmRefCnt) == 0) {
        serviceClose(&g_nsvmSrv);
    }
}

Result nsvmNeedsUpdateVulnerability(bool *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1200;

    Result rc;

    if (hosversionAtLeast(3,0,0))
        rc = serviceIpcDispatch(&g_nsvmSrv);
    else
        rc = serviceIpcDispatch(&g_nsAppManSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result nsvmGetSafeSystemVersion(u16 *out)
{
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1202;

    Result rc = serviceIpcDispatch(&g_nsvmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u16 out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result nsdevLaunchProgram(u64* out_pid, const NsLaunchProperties* properties, u32 flags) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
        u32 pad;
        NsLaunchProperties properties;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->flags = flags;
    raw->pad = 0;
    raw->properties = *properties;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_pid) *out_pid = resp->pid;
        }
    }

    return rc;
}

Result nsdevTerminateProcess(u64 pid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nsdevTerminateProgram(u64 tid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 tid;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->tid = tid;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nsdevGetShellEvent(Event* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(out, r.Handles[0], true);
        }
    }

    return rc;
}

Result nsdevGetShellEventInfo(NsShellEventInfo* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 event;
            u32 pad;
            u64 process_id;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) {
                out->event = (NsShellEvent)resp->event;
                out->process_id = resp->process_id;
            }
        }
    }

    return rc;
}

Result nsdevTerminateApplication(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nsdevPrepareLaunchProgramFromHost(NsLaunchProperties* out, const char* path, size_t path_len) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, path, path_len, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            NsLaunchProperties properties;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->properties;
        }
    }

    return rc;
}

Result nsdevLaunchApplication(u64* out_pid, u64 app_title_id, u32 flags) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
        u32 pad;
        u64 app_title_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->flags = flags;
    raw->pad = 0;
    raw->app_title_id = app_title_id;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_pid) *out_pid = resp->pid;
        }
    }

    return rc;
}

Result nsdevLaunchApplicationWithStorageId(u64* out_pid, u64 app_title_id, u32 flags, u8 app_storage_id, u8 patch_storage_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 app_storage_id;
        u8 patch_storage_id;
        u16 pad;
        u32 flags;
        u64 app_title_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->app_storage_id = app_storage_id;
    raw->patch_storage_id = patch_storage_id;
    raw->pad = 0;
    raw->flags = flags;
    raw->app_title_id = app_title_id;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_pid) *out_pid = resp->pid;
        }
    }

    return rc;
}

Result nsdevIsSystemMemoryResourceLimitBoosted(bool* out) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 boosted;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->boosted != 0;
        }
    }

    return rc;
}

Result nsdevGetRunningApplicationProcessId(u64* out_pid) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_pid) *out_pid = resp->pid;
        }
    }

    return rc;
}

Result nsdevSetCurrentApplicationRightsEnvironmentCanBeActive(bool can_be_active) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 can_be_active;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nsdevSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->can_be_active = can_be_active ? 1 : 0;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nsdevSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
