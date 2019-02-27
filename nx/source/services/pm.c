// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/pm.h"
#include "services/sm.h"

static Service g_pmdmntSrv, g_pmshellSrv, g_pminfoSrv;
static u64 g_pmdmntRefCnt, g_pmshellRefCnt, g_pminfoRefCnt;

Result pmdmntInitialize(void)
{
    atomicIncrement64(&g_pmdmntRefCnt);

    if (serviceIsActive(&g_pmdmntSrv))
        return 0;

    return smGetService(&g_pmdmntSrv, "pm:dmnt");
}

void pmdmntExit(void)
{
    if (atomicDecrement64(&g_pmdmntRefCnt) == 0) {
        serviceClose(&g_pmdmntSrv);
    }
}

Service* pmdmntGetServiceSession(void) {
    return &g_pmdmntSrv;
}

Result pminfoInitialize(void)
{
    atomicIncrement64(&g_pminfoRefCnt);

    if (serviceIsActive(&g_pminfoSrv))
        return 0;

    return smGetService(&g_pminfoSrv, "pm:info");
}

void pminfoExit(void)
{
    if (atomicDecrement64(&g_pminfoRefCnt) == 0) {
        serviceClose(&g_pminfoSrv);
    }
}

Result pmshellInitialize(void)
{
    atomicIncrement64(&g_pmshellRefCnt);

    if (serviceIsActive(&g_pmshellSrv))
        return 0;

    return smGetService(&g_pmshellSrv, "pm:shell");
}

void pmshellExit(void)
{
    if (atomicDecrement64(&g_pmshellRefCnt) == 0) {
        serviceClose(&g_pmshellSrv);
    }
}

Result pmdmntGetDebugProcesses(u32* out_count, u64* out_pids, size_t max_pids) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, out_pids, sizeof(*out_pids) * max_pids, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 0 : 1;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out_count;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_count) *out_count = resp->out_count;
        }
    }

    return rc;
}

Result pmdmntStartProcess(u64 pid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 1 : 2;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

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

Result pmdmntGetTitlePid(u64* pid_out, u64 title_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 title_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 2 : 3;
    raw->title_id = title_id;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *pid_out = resp->pid;
        }
    }

    return rc;
}

Result pmdmntEnableDebugForTitleId(Handle* handle_out, u64 title_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 title_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 3 : 4;
    raw->title_id = title_id;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

Result pminfoGetTitleId(u64* title_id_out, u64 pid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_pminfoSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 title_id;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *title_id_out = resp->title_id;
        }
    }
    return rc;
}

Result pmdmntGetApplicationPid(u64* pid_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 4 : 5;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *pid_out = resp->pid;
        }
    }

    return rc;
}

Result pmdmntEnableDebugForApplication(Handle* handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 5 : 6;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

Result pmdmntDisableDebug(void) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&g_pmdmntSrv);

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

Result pmshellLaunchProcess(u32 launch_flags, u64 titleID, u64 storageID, u64 *pid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 launch_flags;
        u64 titleID;
        u64 storageID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->launch_flags = launch_flags;
    raw->titleID = titleID;
    raw->storageID = storageID;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && pid) *pid = resp->pid;
    }

    return rc;
}

Result pmshellTerminateProcessByProcessId(u64 processID) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 processID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->processID = processID;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

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

Result pmshellTerminateProcessByTitleId(u64 titleID) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 titleID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->titleID = titleID;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

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

Result pmshellGetProcessEvent(Event* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);
        
        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(out, r.Handles[0], true);
        }
    }

    return rc;
}

Result pmshellGetProcessEventInfo(PmProcessEventInfo* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);
        
        struct {
            u64 magic;
            u64 result;
            u32 event;
            u32 pad;
            u64 process_id;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) {
                out->event = (PmProcessEvent)resp->event;
                out->process_id = resp->process_id;
            }
        }
    }

    return rc;
}

Result pmshellFinalizeDeadProcess(u64 pid) {
    if (hosversionAtLeast(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

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

Result pmshellClearProcessExceptionOccurred(u64 pid) {
    if (hosversionAtLeast(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

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

Result pmshellNotifyBootFinished(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 5 : 7;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

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

Result pmshellGetApplicationPid(u64* pid_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 6 : 8;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *pid_out = resp->pid;
        }
    }

    return rc;
}

Result pmshellBoostSystemMemoryResourceLimit(u64 boost_size) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 boost_size;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(5,0,0) ? 7 : 9;
    raw->boost_size = boost_size;

    Result rc = serviceIpcDispatch(&g_pmshellSrv);

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
