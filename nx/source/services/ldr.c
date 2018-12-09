// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/ldr.h"
#include "services/fs.h"
#include "services/sm.h"

static Service g_shellSrv, g_dmntSrv, g_pmSrv;
static u64 g_shellRefCnt, g_dmntRefCnt, g_pmRefCnt;

/* Service init/exit helpers. */
static Result _ldrSrvInitialize(Service* srv, u64 *refcnt, const char *name) {
    atomicIncrement64(refcnt);
    
    if (serviceIsActive(srv))
        return 0;
    
    return smGetService(srv, name);
}

static void _ldrSrvExit(Service* srv, u64 *refcnt) {
    if (atomicDecrement64(refcnt) == 0)
        serviceClose(srv);
}

Result ldrShellInitialize(void) {
    return _ldrSrvInitialize(&g_shellSrv, &g_shellRefCnt, "ldr:shel");
}

void ldrShellExit(void) {
    return _ldrSrvExit(&g_shellSrv, &g_shellRefCnt);
}

Result ldrDmntInitialize(void) {
    return _ldrSrvInitialize(&g_dmntSrv, &g_dmntRefCnt, "ldr:dmnt");
}

void ldrDmntExit(void) {
    return _ldrSrvExit(&g_dmntSrv, &g_dmntRefCnt);
}

Result ldrPmInitialize(void) {
    return _ldrSrvInitialize(&g_pmSrv, &g_pmRefCnt, "ldr:pm");
}

void ldrPmExit(void) {
    return _ldrSrvExit(&g_pmSrv, &g_pmRefCnt);
}

static Result _ldrAddTitleToLaunchQueue(Service* srv, u64 tid, const void *args, size_t args_size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, args, args_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 tid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->tid = tid;

    Result rc = serviceIpcDispatch(srv);

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

static Result _ldrClearLaunchQueue(Service* srv) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(srv);

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

Result ldrShellAddTitleToLaunchQueue(u64 tid, const void *args, size_t args_size) {
    return _ldrAddTitleToLaunchQueue(&g_shellSrv, tid, args, args_size);
}

Result ldrShellClearLaunchQueue(void) {
    return _ldrClearLaunchQueue(&g_shellSrv);
}

Result ldrDmntAddTitleToLaunchQueue(u64 tid, const void *args, size_t args_size) {
    return _ldrAddTitleToLaunchQueue(&g_dmntSrv, tid, args, args_size);
}

Result ldrDmntClearLaunchQueue(void) {
    return _ldrClearLaunchQueue(&g_dmntSrv);
}

Result ldrDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t out_size, u32 *num_out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddRecvStatic(&c, out_module_infos, out_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_dmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 num_out;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && num_out != NULL) {
            *num_out = resp->num_out;
        }
    }

    return rc;
}

Result ldrPmCreateProcess(u64 flags, u64 launch_index, Handle reslimit_h, Handle *out_process_h) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcSendHandleCopy(&c, reslimit_h);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 flags;
        u64 launch_index;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->flags = flags;
    raw->launch_index = launch_index;

    Result rc = serviceIpcDispatch(&g_pmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_process_h = r.Handles[0];
        }
    }

    return rc;
}

Result ldrPmGetProgramInfo(u64 title_id, FsStorageId storage_id, LoaderProgramInfo *out_program_info) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddRecvStatic(&c, out_program_info, sizeof(LoaderProgramInfo), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 title_id;
        u64 storage_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->title_id = title_id;
    raw->storage_id = storage_id;

    Result rc = serviceIpcDispatch(&g_pmSrv);

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

Result ldrPmRegisterTitle(u64 title_id, FsStorageId storage_id, u64 *out_index) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 title_id;
        u64 storage_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->title_id = title_id;
    raw->storage_id = storage_id;

    Result rc = serviceIpcDispatch(&g_pmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 index;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *out_index = resp->index;
        }
    }

    return rc;
}

Result ldrPmUnregisterTitle(u64 launch_index) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 launch_index;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->launch_index = launch_index;

    Result rc = serviceIpcDispatch(&g_pmSrv);

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
