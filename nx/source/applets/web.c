#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/web.h"

static Result _webHandleExit(AppletHolder* holder, void* reply_buffer, size_t reply_size) {
    Result rc=0;
    size_t transfer_size=0;
    appletHolderJoin(holder);

    LibAppletExitReason reason = appletHolderGetExitReason(holder);

    if (reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
        rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    }

    if (R_SUCCEEDED(rc) && reply_buffer && reply_size) {
        rc = libappletPopOutData(holder, reply_buffer, reply_size, &transfer_size);
        if (R_SUCCEEDED(rc) && transfer_size != reply_size) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    return rc;
}

void webWifiCreate(WebWifiConfig* config, const char* conntest_url, const char* initial_url, u128 userID, u32 unk) {
    memset(config, 0, sizeof(WebWifiConfig));

    if (conntest_url==NULL) conntest_url = initial_url;

    strncpy(config->arg.conntest_url, conntest_url, sizeof(config->arg.conntest_url)-1);
    strncpy(config->arg.initial_url, initial_url, sizeof(config->arg.initial_url)-1);

    config->arg.userID = userID;
    config->arg.unk_x514 = unk;
}

Result webWifiShow(WebWifiConfig* config, WebWifiReturnValue *out) {
    Result rc = 0;
    AppletHolder holder;

    rc = appletCreateLibraryApplet(&holder, AppletId_wifiWebAuth, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, 0);
    rc = libappletArgsPush(&commonargs, &holder);

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(&holder, &config->arg, sizeof(config->arg));

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);

    if (R_SUCCEEDED(rc)) rc = _webHandleExit(&holder, out, sizeof(*out));

    appletHolderClose(&holder);

    return rc;
}
