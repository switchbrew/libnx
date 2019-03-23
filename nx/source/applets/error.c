#include <malloc.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/error.h"

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

void errorCreate(ErrorConfig* c) {
    memset(c, 0, sizeof(ErrorConfig));
    c->custom_text = false;
    c->module = 2000;
}

void errorClose(ErrorConfig* c) {
    memset(c, 0, sizeof(ErrorConfig));
}

Result errorShow(ErrorConfig* c) {
    return _errorShow(c, sizeof(ErrorConfig), NULL, 0);
}

void errorConfigSetModule(ErrorConfig* c, u32 code) {
    c->module = code;
}

void errorConfigSetDescription(ErrorConfig* c, u32 code) {
    c->description = code;
}

void errorConfigSetCustomText(ErrorConfig* c, bool custom_text) {
    c->custom_text = custom_text;
}

void errorConfigSetShortDescription(ErrorConfig* c, const char* str) {
    strncpy(c->short_description, str, sizeof(c->short_description) - 1);
}

void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str) {
    strncpy(c->detailed_description, str, sizeof(c->detailed_description) - 1);
}
