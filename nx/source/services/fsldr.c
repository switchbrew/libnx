// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/fs.h"
#include "services/sm.h"
#include "services/fsldr.h"

static Service g_fsldrSrv;
static u64 g_fsldrRefCnt;

Result fsldrSetCurrentProcess(void);

Result fsldrInitialize(void) {
    atomicIncrement64(&g_fsldrRefCnt);

    if (serviceIsActive(&g_fsldrSrv))
        return 0;

    Result rc = smGetService(&g_fsldrSrv, "fsp-ldr");
    
    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_fsldrSrv);
    }

    if (R_SUCCEEDED(rc) && hosversionAtLeast(4,0,0)) {
        rc = fsldrSetCurrentProcess();
    }

    return rc;

}

void fsldrExit(void) {
    if (atomicDecrement64(&g_fsldrRefCnt) == 0)
        serviceClose(&g_fsldrSrv);
}

Result fsldrOpenCodeFileSystem(u64 tid, const char *path, FsFileSystem* out) {
    char send_path[FS_MAX_PATH+1] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, FS_MAX_PATH, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 tid;
    } *raw;
    
    raw = serviceIpcPrepareHeader(&g_fsldrSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->tid = tid;
    
    strncpy(send_path, path, FS_MAX_PATH);
    Result rc = serviceIpcDispatch(&g_fsldrSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsldrSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_fsldrSrv, &r, 0);
        }
    }
    
    return rc;
}

Result fsldrIsArchivedProgram(u64 pid, bool *out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;
    
    raw = serviceIpcPrepareHeader(&g_fsldrSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = pid;
    
    Result rc = serviceIpcDispatch(&g_fsldrSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 is_archived;
        } *resp;

        serviceIpcParse(&g_fsldrSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out = resp->is_archived != 0;
        }
    }
    
    return rc;
}

Result fsldrSetCurrentProcess(void) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 unk;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsldrSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->unk = 0;

    Result rc = serviceIpcDispatch(&g_fsldrSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsldrSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }
    
    return rc;
}

