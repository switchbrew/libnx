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

Result errorSystemCreate(ErrorSystemConfig* c, const char* dialog_message, const char* fullscreen_message) {
    Result rc=0;

    memset(c, 0, sizeof(ErrorSystemConfig));
    c->arg.hdr.type = 1;

    strncpy(c->arg.dialogMessage, dialog_message, sizeof(c->arg.dialogMessage)-1);
    strncpy(c->arg.fullscreenMessage, fullscreen_message, sizeof(c->arg.fullscreenMessage)-1);

    if (hosversionBefore(5,0,0)) {
        rc = setInitialize();
        if (R_SUCCEEDED(rc)) rc = setMakeLanguageCode(SetLanguage_ENUS, &c->arg.languageCode);
        setExit();
    }

    return rc;
}

void errorSystemClose(ErrorSystemConfig* c) {
    memset(c, 0, sizeof(ErrorSystemConfig));
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

