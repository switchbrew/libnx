// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/fs.h"
#include "services/sm.h"
#include "services/fspr.h"

static Service g_fsprSrv;
static u64 g_fsprRefCnt;

/* Default access controls -- these will give full filesystem permissions to the requester. */
static const uint32_t g_fspr_default_fah[] = {0x1, 0xFFFFFFFF, 0xFFFFFFFF, 0x1C, 0, 0x1C, 0};
static const uint32_t g_fspr_default_fac[] = {0x1, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF};

Result fsprInitialize(void) {
    atomicIncrement64(&g_fsprRefCnt);

    if (serviceIsActive(&g_fsprSrv))
        return 0;

    Result rc = smGetService(&g_fsprSrv, "fsp-pr");

    if (R_SUCCEEDED(rc) && hosversionAtLeast(4,0,0)) {
        rc = fsprSetCurrentProcess();
        if (R_FAILED(rc)) {
            fsprExit();
        }
    }

    return rc;
}

void fsprExit(void) {
    if (atomicDecrement64(&g_fsprRefCnt) == 0)
        serviceClose(&g_fsprSrv);
}

Result fsprRegisterProgram(u64 pid, u64 titleID, FsStorageId storageID, const void *fs_access_header, size_t fah_size, const void *fs_access_control, size_t fac_size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    if (fs_access_header == NULL) {
        fs_access_header = g_fspr_default_fah;
        fah_size = sizeof(g_fspr_default_fah);
    }
    if (fs_access_control == NULL) {
        fs_access_control = g_fspr_default_fac;
        fac_size = sizeof(g_fspr_default_fac);
    }
    
    ipcAddSendBuffer(&c, fs_access_header, fah_size, BufferType_Normal);
    ipcAddSendBuffer(&c, fs_access_control, fac_size, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 sid;
        u64 pid;
        u64 tid;
        u64 fah_size;
        u64 fac_size;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->sid = storageID;
    raw->pid = pid;
    raw->tid = titleID;
    raw->fah_size = fah_size;
    raw->fac_size = fac_size;
    
    Result rc = serviceIpcDispatch(&g_fsprSrv);
    
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

Result fsprUnregisterProgram(u64 pid) {
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
    
    Result rc = serviceIpcDispatch(&g_fsprSrv);
    
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

Result fsprSetCurrentProcess(void) {
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
    raw->cmd_id = 2;
    raw->unk = 0;

    Result rc = serviceIpcDispatch(&g_fsprSrv);

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


Result fsprSetEnabledProgramVerification(bool enabled) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u8 enabled;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 256;
    raw->enabled = enabled ? 1 : 0;
    
    Result rc = serviceIpcDispatch(&g_fsprSrv);
    
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
