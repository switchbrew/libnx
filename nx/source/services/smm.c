// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/fs.h"
#include "services/sm.h"
#include "services/smm.h"

static Service g_smManagerSrv;
static u64 g_smManagerRefcnt;

Result smManagerInitialize(void) {
    atomicIncrement64(&g_smManagerRefcnt);

    if (serviceIsActive(&g_smManagerSrv))
        return 0;

    return smGetService(&g_smManagerSrv, "sm:m");
}

void smManagerExit(void) {
    if (atomicDecrement64(&g_smManagerRefcnt) == 0)
        serviceClose(&g_smManagerSrv);    
}

Result smManagerRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, acid_sac, acid_sac_size, BufferType_Normal);
    ipcAddSendBuffer(&c, aci0_sac, aci0_sac_size, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->pid = pid;
    
    Result rc = serviceIpcDispatch(&g_smManagerSrv);
    
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

Result smManagerUnregisterProcess(u64 pid) {
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
    
    Result rc = serviceIpcDispatch(&g_smManagerSrv);
    
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
