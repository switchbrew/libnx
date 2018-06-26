// Copyright 2017 plutoo
#include "types.h"
#include "result.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "kernel/svc.h"
#include "services/fatal.h"
#include "services/sm.h"

void NORETURN fatalSimple(Result err) {
    /* By default, do not generate an error report. */
    fatalWithType(err, FatalType_ErrorScreen);
    svcExitProcess();
    __builtin_unreachable();
}

void fatalWithType(Result err, FatalType type) {
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
                u64 type;
            } *raw;

            raw = ipcPrepareHeader(&c, sizeof(*raw));

            raw->magic = SFCI_MAGIC;
            raw->cmd_id = 1;
            raw->result = err;
            raw->type = type;

            ipcDispatch(srv);
        }
    }
    
    switch (type) {
        case FatalType_ErrorReport:
            break;
        case FatalType_ErrorReportAndErrorScreen:
        case FatalType_ErrorScreen:
        default:
            svcExitProcess();
            __builtin_unreachable();
    }
}
