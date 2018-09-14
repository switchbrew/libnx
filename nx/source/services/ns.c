#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
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

    if(!kernelAbove300())
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
        if(!kernelAbove300()) return;

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
    if (!kernelAbove300())
        return 0;

    atomicIncrement64(&g_nsvmRefCnt);

    if (serviceIsActive(&g_nsvmSrv))
        return 0;

    return smGetService(&g_nsvmSrv, "ns:vm");
}

void nsvmExit(void)
{
    if (!kernelAbove300())
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
    
    if (kernelAbove300())
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
    if (!kernelAbove300())
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

Result nsdevTerminateProcess(u64 pid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

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

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->tid = tid;

    Result rc = serviceIpcDispatch(&g_nsdevSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
