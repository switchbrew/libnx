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

static Result _errorShowContext(const void* indata, size_t insize, ErrorContext* ctx) {
    void* ctx_ptr = NULL;
    size_t ctx_size = 0;
    if (hosversionAtLeast(4,0,0)) {
        ctx_ptr = ctx;
        if (ctx_ptr) ctx_size = sizeof(ErrorContext);
    }

    return _errorShow(indata, insize, ctx_ptr, ctx_size);
}

// Backtrace

Result errorResultBacktraceCreate(ErrorResultBacktrace* backtrace, s32 count, Result* entries) {
    if (backtrace==NULL || count < 0 || count > sizeof(backtrace->backtrace)/sizeof(Result))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(backtrace, 0, sizeof(*backtrace));
    backtrace->count = count;
    if (backtrace->count) memcpy(&backtrace->backtrace, entries, count);

    return 0;
}

void errorResultBacktraceClose(ErrorResultBacktrace* backtrace) {
    memset(backtrace, 0, sizeof(*backtrace));
}

Result errorResultBacktraceShow(Result res, ErrorResultBacktrace* backtrace) {
    ErrorCommonArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = 0;
    arg.hdr.jumpFlag = 1;
    arg.hdr.contextFlag = 1;
    arg.res = res;

    return _errorShow(&arg, sizeof(arg), backtrace, sizeof(*backtrace));
}

// Eula

Result errorEulaShow(SetRegion RegionCode) {
    ErrorEulaArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = 3;
    arg.hdr.jumpFlag = 1;
    arg.regionCode = RegionCode;

    return _errorShow(&arg, sizeof(arg), NULL, 0);
}

Result errorSystemUpdateEulaShow(SetRegion RegionCode, ErrorEulaData* eula) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage={0};
    ErrorEulaArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.hdr.type = 8;
    arg.hdr.jumpFlag = 1;
    arg.regionCode = RegionCode;

    rc = _errorAppletCreate(&holder, &arg, sizeof(arg), NULL, 0);

    if (R_SUCCEEDED(rc)) {
        if (R_SUCCEEDED(rc)) rc = appletCreateTransferMemoryStorage(&storage, eula, sizeof(*eula), false);
        if (R_SUCCEEDED(rc)) rc = appletHolderPushInData(&holder, &storage);
    }

    if (R_SUCCEEDED(rc)) rc = _errorRun(&holder);

    appletHolderClose(&holder);
    appletStorageCloseTmem(&storage);

    return rc;
}

// System

Result errorSystemCreate(ErrorSystemConfig* c, const char* dialog_message, const char* fullscreen_message) {
    Result rc=0;

    memset(c, 0, sizeof(*c));
    c->arg.hdr.type = 1;

    strncpy(c->arg.dialogMessage, dialog_message, sizeof(c->arg.dialogMessage)-1);
    if (fullscreen_message) strncpy(c->arg.fullscreenMessage, fullscreen_message, sizeof(c->arg.fullscreenMessage)-1);

    if (hosversionBefore(5,0,0)) {
        rc = setInitialize();
        if (R_SUCCEEDED(rc)) rc = setMakeLanguageCode(SetLanguage_ENUS, &c->arg.languageCode);
        setExit();
    }

    return rc;
}

void errorSystemClose(ErrorSystemConfig* c) {
    memset(c, 0, sizeof(*c));
}

Result errorSystemShow(ErrorSystemConfig* c) {
    return _errorShowContext(&c->arg, sizeof(c->arg), c->arg.hdr.contextFlag!=0 ? &c->ctx : NULL);
}

void errorSystemSetCode(ErrorSystemConfig* c, u32 low, u32 desc) {
    c->arg.errorCode = (u64)low | ((u64)desc<<32);
}

void errorSystemSetResult(ErrorSystemConfig* c, Result res) {
    errorSystemSetCode(c, 2000 + R_MODULE(res), R_DESCRIPTION(res));
}

void errorSystemSetLanguageCode(ErrorSystemConfig* c, u64 LanguageCode) {
    c->arg.languageCode = LanguageCode;
}

void errorSystemSetContext(ErrorSystemConfig* c, ErrorContext* ctx) {
    c->arg.hdr.contextFlag = ctx!=0;
    memset(&c->ctx, 0, sizeof(ErrorContext));
    if (ctx) memcpy(&c->ctx, ctx, sizeof(ErrorContext));
}

// Application

Result errorApplicationCreate(ErrorApplicationConfig* c, const char* dialog_message, const char* fullscreen_message) {
    Result rc=0;

    memset(c, 0, sizeof(*c));
    c->arg.hdr.type = 2;
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

void errorApplicationClose(ErrorApplicationConfig* c) {
    memset(c, 0, sizeof(*c));
}

Result errorApplicationShow(ErrorApplicationConfig* c) {
    return _errorShow(&c->arg, sizeof(c->arg), NULL, 0);
}

void errorApplicationSetNumber(ErrorApplicationConfig* c, u32 errorNumber) {
    c->arg.errorNumber = errorNumber;
}

void errorApplicationSetLanguageCode(ErrorApplicationConfig* c, u64 LanguageCode) {
    c->arg.languageCode = LanguageCode;
}

