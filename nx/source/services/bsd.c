// Copyright 2017 plutoo
#include <switch.h>

static Handle g_bsdHandle = -1;

Result bsdInitialize(TransferMemory* tmem) {
    Result rc = smGetService(&g_bsdHandle, "bsd:s");

    if (R_FAILED(rc)) {
        rc = smGetService(&g_bsdHandle, "bsd:u");
    }

    if (R_SUCCEEDED(rc)) {
        IpcCommand c;
        ipcInitialize(&c);
        ipcSendPid(&c);
        ipcSendHandleCopy(&c, tmem->MemHandle);

        struct {
            u64 magic;
            u64 cmd_id;
            u64 unk[5];
            u64 tmem_sz;
        } *raw;

        raw = ipcPrepareHeader(&c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;
        raw->unk[0] = 0;
        raw->unk[1] = 0;
        raw->unk[2] = 0;
        raw->unk[3] = 0;
        raw->unk[4] = 0;
        raw->tmem_sz = tmem->Size;
    }

    return rc;
}
