// Copyright 2017 plutoo
#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "types.h"
#include "result.h"
#include "kernel/detect.h"
#include "kernel/svc.h"
#include "sf/service.h"
#include "services/fatal.h"
#include "services/sm.h"

static void _fatalImpl(u32 cmd_id, Result err, FatalType type, FatalContext *ctx) {
    Result rc = 0;

    //Only [3.0.0+] supports FatalType_ErrorScreen, when specified on pre-3.0.0 use FatalType_ErrorReportAndErrorScreen instead.
    if (type == FatalType_ErrorScreen && !kernelAbove300()) type = FatalType_ErrorReportAndErrorScreen;

    if (detectDebugger()) {
        svcBreak(0x80000000, err, 0);
    }

    Handle session;
    rc = smInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = smGetServiceOriginal(&session, smEncodeName("fatal:u"));
        smExit();
    }

    if (R_SUCCEEDED(rc)) {
        const struct {
            u32 result;
            u32 type;
            u64 pid_placeholder;
        } in = { err, type };

        Service s;
        serviceCreate(&s, session);
        serviceDispatchIn(&s, cmd_id, in,
            .buffer_attrs = { ctx ? (SfBufferAttr_In | SfBufferAttr_HipcMapAlias) : 0U },
            .buffers      = { { ctx, sizeof(*ctx) } },
            .in_send_pid  = true,
        );
        serviceClose(&s);
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

void NORETURN fatalSimple(Result err) {
    /* By default, do not generate an error report. */
    fatalWithType(err, FatalType_ErrorScreen);
    svcExitProcess();
    __builtin_unreachable();
}

void fatalWithType(Result err, FatalType type) {
    _fatalImpl(1, err, type, NULL);
}

void fatalWithContext(Result err, FatalType type, FatalContext *ctx) {
    _fatalImpl(2, err, type, ctx);
}
