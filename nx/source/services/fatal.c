// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "ipc.h"
#include "services/fatal.h"
#include "services/sm.h"
#include "kernel/detect.h"
#include "kernel/svc.h"

void fatalSimple(Result err) {
    Result rc = 0;

    if (detectDebugger()) {
        svcBreak(0x80000000, err, 0);
    }

    if (!smHasInitialized()) {
        rc = smInitialize();
    }

    if (R_SUCCEEDED(rc)) {
        Handle srv;
        rc = smGetServiceOriginal(&srv, smEncodeName("fatal:u"));

        if (R_SUCCEEDED(rc)) {
            IpcCommand c;
            ipcInitialize(&c);
            ipcSendPid(&c);

            struct {
                u64 magic;
                u64 cmd_id;
                u64 result;
                u64 unknown;
            } *raw;

            raw = ipcPrepareHeader(&c, sizeof(*raw));

            raw->magic = SFCI_MAGIC;
            raw->cmd_id = 1;
            raw->result = err;
            raw->unknown = 0;

            ipcDispatch(srv);
        }
    }

    ((void(*)())0xBADC0DE)();
    __builtin_unreachable();
}
