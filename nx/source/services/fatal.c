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

    //Only 3.0.0+ supports FatalType_ErrorScreen, when specified on pre-3.0.0 use FatalType_ErrorReportAndErrorScreen instead.
    if (type == FatalType_ErrorScreen && !kernelAbove300()) type = FatalType_ErrorReportAndErrorScreen;

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
                u32 result;
                u32 type;
                u64 pid_placeholder;
            } *raw;

            raw = ipcPrepareHeader(&c, sizeof(*raw));

            raw->magic = SFCI_MAGIC;
            raw->cmd_id = 1;
            raw->result = err;
            raw->type = type;
            raw->pid_placeholder = 0; // Overwritten by fatal with PID descriptor.

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
