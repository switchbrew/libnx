#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "services/set.h"
#include "applets/libapplet.h"
#include "applets/error.h"
#include "runtime/hosversion.h"

static Result _errorAppletCreate(AppletHolder* holder, const void* indata, size_t insize, const void* indata2, size_t insize2) {
    Result rc=0;
    LibAppletArgs commonargs;

    rc = appletCreateLibraryApplet(holder, AppletId_error, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    libappletArgsCreate(&commonargs, 0);
    if (R_SUCCEEDED(rc)) rc = libappletArgsPush(&commonargs, holder);
    if (R_SUCCEEDED(rc)) rc = libappletPushInData(holder, indata, insize);
    if (R_SUCCEEDED(rc) && indata2 && insize2) rc = libappletPushInData(holder, indata2, insize2);

    return rc;
}

static Result _errorRun(AppletHolder* holder) {
    Result rc=0;
    u8 reply[2]={0};
    size_t transfer_size = 0;
    u8 status=0;//Official sw doesn't use the output status.

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(holder);

    if (R_SUCCEEDED(rc)) {
        appletHolderJoin(holder);
        LibAppletExitReason reason = appletHolderGetExitReason(holder);

        if (reason == LibAppletExitReason_Normal) {
            if (R_SUCCEEDED(libappletPopOutData(holder, reply, sizeof(reply), &transfer_size))) {
                if (transfer_size != sizeof(reply)) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
                if (R_SUCCEEDED(rc)) status = reply[1];
            }
        }
        else if (reason == LibAppletExitReason_Canceled) {
            status = 0;
        }
        else {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }

        if (R_SUCCEEDED(rc) && status!=0) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    }

    return rc;
}

static Result _errorShow(const void* indata, size_t insize, const void* indata2, size_t insize2) {
    Result rc=0;
    AppletHolder holder;

    rc = _errorAppletCreate(&holder, indata, insize, indata2, insize2);
    if (R_SUCCEEDED(rc)) rc = _errorRun(&holder);

    appletHolderClose(&holder);

    return rc;
}

static Result _errorShowContext(const void* indata, size_t insize, const ErrorContext* ctx) {
    const void* ctx_ptr = NULL;
    size_t ctx_size = 0;
    if (hosversionAtLeast(4,0,0)) {
        ctx_ptr = ctx;
        if (ctx_ptr) ctx_size = sizeof(ErrorContext);
    }

    return _errorShow(indata, insize, ctx_ptr, ctx_size);
}

// {Result/Code}Show

Result errorResultShow(Result res, bool jumpFlag, const ErrorContext* ctx) {
    if (!jumpFlag) ctx = NULL;
    ErrorCommonArg arg_common;
    ErrorPctlArg arg_pctl;
    ErrorCommonHeader *hdr = NULL;
    bool flag = hosversionAtLeast(4,0,0) && ctx!=NULL;
    bool argtype;
    u32 tmp = R_DESCRIPTION(res);

    argtype = R_MODULE(res)==142 && tmp >= 100 && tmp <= 100+19;
    if (!argtype) hdr = &arg_common.hdr;
    if (argtype) hdr = &arg_pctl.hdr;

    memset(&arg_common, 0, sizeof(arg_common));
    memset(&arg_pctl, 0, sizeof(arg_pctl));

    hdr->type = !argtype ? ErrorType_Normal : ErrorType_Pctl;
    hdr->jumpFlag = jumpFlag!=0;
    if (flag) hdr->contextFlag2 = 1;

    if (!argtype) {
        arg_common.res = res;
        return _errorShowContext(&arg_common, sizeof(arg_common), ctx);
    }
    else {
        arg_pctl.res = res;
        return _errorShowContext(&arg_pctl, sizeof(arg_pctl), ctx);
    }
}

Result errorCodeShow(ErrorCode errorCode, bool jumpFlag, const ErrorContext* ctx) {
    if (!jumpFlag) ctx = NULL;
    bool flag = hosversionAtLeast(4,0,0) && ctx!=NULL;
    ErrorCommonArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = ErrorType_Normal;
    arg.hdr.jumpFlag = jumpFlag!=0;
    if (flag) arg.hdr.contextFlag2 = 1;
    arg.hdr.resultFlag = 1;
    arg.errorCode = errorCode;

    return _errorShowContext(&arg, sizeof(arg), ctx);
}

// Backtrace

Result errorResultBacktraceCreate(ErrorResultBacktrace* backtrace, s32 count, const Result* entries) {
    if (backtrace==NULL || count < 0 || count > sizeof(backtrace->backtrace)/sizeof(Result))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(backtrace, 0, sizeof(*backtrace));
    backtrace->count = count;
    if (backtrace->count) memcpy(&backtrace->backtrace, entries, count);

    return 0;
}

Result errorResultBacktraceShow(Result res, const ErrorResultBacktrace* backtrace) {
    ErrorCommonArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = ErrorType_Normal;
    arg.hdr.jumpFlag = 1;
    arg.hdr.contextFlag = 1;
    arg.res = res;

    return _errorShow(&arg, sizeof(arg), backtrace, sizeof(*backtrace));
}

// Eula

Result errorEulaShow(SetRegion RegionCode) {
    ErrorEulaArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = ErrorType_Eula;
    arg.hdr.jumpFlag = 1;
    arg.regionCode = RegionCode;

    return _errorShow(&arg, sizeof(arg), NULL, 0);
}

Result errorSystemUpdateEulaShow(SetRegion RegionCode, const ErrorEulaData* eula) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage={0};
    ErrorEulaArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = ErrorType_SystemUpdateEula;
    arg.hdr.jumpFlag = 1;
    arg.regionCode = RegionCode;

    rc = _errorAppletCreate(&holder, &arg, sizeof(arg), NULL, 0);

    if (R_SUCCEEDED(rc)) {
        if (R_SUCCEEDED(rc)) rc = appletCreateTransferMemoryStorage(&storage, (void*)eula, sizeof(*eula), false);
        if (R_SUCCEEDED(rc)) rc = appletHolderPushInData(&holder, &storage);
    }

    if (R_SUCCEEDED(rc)) rc = _errorRun(&holder);

    appletHolderClose(&holder);
    appletStorageCloseTmem(&storage);

    return rc;
}

// Record

Result errorCodeRecordShow(ErrorCode errorCode, u64 timestamp) {
    ErrorRecordArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = ErrorType_Record;
    arg.hdr.jumpFlag = 1;
    arg.errorCode = errorCode;
    arg.timestamp = timestamp;

    return _errorShow(&arg, sizeof(arg), NULL, 0);
}

// System

Result errorSystemCreate(ErrorSystemConfig* c, const char* dialog_message, const char* fullscreen_message) {
    Result rc=0;

    memset(c, 0, sizeof(*c));
    c->arg.hdr.type = ErrorType_System;

    strncpy(c->arg.dialogMessage, dialog_message, sizeof(c->arg.dialogMessage)-1);
    if (fullscreen_message) strncpy(c->arg.fullscreenMessage, fullscreen_message, sizeof(c->arg.fullscreenMessage)-1);

    if (hosversionBefore(5,0,0)) {
        rc = setInitialize();
        if (R_SUCCEEDED(rc)) rc = setMakeLanguageCode(SetLanguage_ENUS, &c->arg.languageCode);
        setExit();
    }

    return rc;
}

Result errorSystemShow(ErrorSystemConfig* c) {
    return _errorShowContext(&c->arg, sizeof(c->arg), c->arg.hdr.contextFlag!=0 ? &c->ctx : NULL);
}

void errorSystemSetContext(ErrorSystemConfig* c, const ErrorContext* ctx) {
    if (hosversionBefore(4,0,0)) return;

    c->arg.hdr.contextFlag = ctx!=0;
    memset(&c->ctx, 0, sizeof(ErrorContext));
    if (ctx) memcpy(&c->ctx, ctx, sizeof(ErrorContext));
}

// Application

Result errorApplicationCreate(ErrorApplicationConfig* c, const char* dialog_message, const char* fullscreen_message) {
    Result rc=0;

    memset(c, 0, sizeof(*c));
    c->arg.hdr.type = ErrorType_Application;
    c->arg.hdr.jumpFlag = 1;

    strncpy(c->arg.dialogMessage, dialog_message, sizeof(c->arg.dialogMessage)-1);
    if (fullscreen_message) strncpy(c->arg.fullscreenMessage, fullscreen_message, sizeof(c->arg.fullscreenMessage)-1);

    if (hosversionBefore(5,0,0)) {
        rc = setInitialize();
        if (R_SUCCEEDED(rc)) rc = setMakeLanguageCode(SetLanguage_ENUS, &c->arg.languageCode);
        setExit();
    }

    return rc;
}

Result errorApplicationShow(ErrorApplicationConfig* c) {
    return _errorShow(&c->arg, sizeof(c->arg), NULL, 0);
}

