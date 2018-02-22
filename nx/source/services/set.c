// Copyright 2018 plutoo
#include "types.h"
#include "result.h"
#include "ipc.h"
#include "services/set.h"
#include "services/sm.h"

static Service g_setsysSrv;

Result setsysInitialize(void)
{
    if (serviceIsActive(&g_setsysSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    return smGetService(&g_setsysSrv, "set:sys");
}

void setsysExit(void)
{
    serviceClose(&g_setsysSrv);
}

Result setsysGetColorSetId(ColorSetId* out)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 color_set;
        } *resp = r.Raw;

        *out = resp->color_set;
        rc = resp->result;
    }

    return rc;

}
