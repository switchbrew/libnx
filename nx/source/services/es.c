/**
 * @file es.c
 * @brief ETicket service IPC wrapper.
 * @author simontime
 * @copyright libnx Authors
 */

#include "services/es.h"
#include "arm/atomics.h"

static Service g_esSrv;
static u64 g_RefCnt;

Result esInitialize(void) {
    atomicIncrement64(&g_RefCnt);
    
    if (serviceIsActive(&g_esSrv))
        return 0;
    
    return smGetService(&g_esSrv, "es");
}

void esExit(void) {
    if (atomicDecrement64(&g_RefCnt) == 0)
        serviceClose(&g_esSrv);
}

Result esCountCommmonTicket(u32* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof *raw);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;

    Result rc = serviceIpcDispatch(&g_esSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
		ipcParse(&r);
		
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;
}

Result esCountPersonalizedTicket(u32* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof *raw);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&g_esSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
		ipcParse(&r);
		
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;
}
