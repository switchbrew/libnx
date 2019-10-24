#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "types.h"
#include "result.h"
#include "kernel/detect.h"
#include "kernel/svc.h"
#include "sf/service.h"
#include "services/fatal.h"
#include "services/sm.h"

static void _fatalCmd(Result err, FatalPolicy type, FatalCpuContext *ctx, u32 cmd_id) {
    Result rc = 0;

    //Only [3.0.0+] supports FatalPolicy_ErrorScreen, when specified on pre-3.0.0 use FatalPolicy_ErrorReportAndErrorScreen instead.
    if (type == FatalPolicy_ErrorScreen && !kernelAbove300()) type = FatalPolicy_ErrorReportAndErrorScreen;

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
        case FatalPolicy_ErrorReport:
            break;
        case FatalPolicy_ErrorReportAndErrorScreen:
        case FatalPolicy_ErrorScreen:
        default:
            svcExitProcess();
            __builtin_unreachable();
    }
}

void NORETURN fatalThrow(Result err) {
    // By default, do not generate an error report.
    fatalThrowWithPolicy(err, FatalPolicy_ErrorScreen);
    svcExitProcess();
    __builtin_unreachable();
}

void fatalThrowWithPolicy(Result err, FatalPolicy type) {
    _fatalCmd(err, type, NULL, 1);
}

void fatalThrowWithContext(Result err, FatalPolicy type, FatalCpuContext *ctx) {
    _fatalCmd(err, type, ctx, 2);
}

void NORETURN fatalSimple(Result) __attribute__((alias("fatalThrow")));
