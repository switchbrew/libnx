// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/csrng.h"
#include "services/sm.h"
#include "services/spl.h"

static Service g_csrngSrv;
static u64 g_csrngRefCnt;

Result csrngInitialize(void) {
    atomicIncrement64(&g_csrngRefCnt);
    
    if (serviceIsActive(&g_csrngSrv))
        return 0;
    
    return smGetService(&g_csrngSrv, "csrng");
}

void csrngExit(void) {
    if (atomicDecrement64(&g_csrngRefCnt) == 0)
        serviceClose(&g_csrngSrv);
}

Result csrngGetRandomBytes(void *out, size_t out_size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddRecvBuffer(&c, out, out_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_csrngSrv);

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
