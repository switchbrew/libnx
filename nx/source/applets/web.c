#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/web.h"

static void _webWifiUrlCreate(WebWifiPageArgUrl* argUrl, const char* url) {
    strncpy(argUrl->url, url, sizeof(argUrl->url)-1);
}

void webWifiCreate(WebWifiConfig* config, const char* url) {
    memset(config, 0, sizeof(WebWifiConfig));
    _webWifiUrlCreate(&config->arg.url1, url);
    _webWifiUrlCreate(&config->arg.url2, url);
}

Result webWifiShow(WebWifiConfig* config) {
    Result rc = 0;
    AppletHolder holder;

    rc = appletCreateLibraryApplet(&holder, AppletId_wifiWebAuth, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, 0);
    rc = libappletArgsPush(&commonargs, &holder);

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(&holder, &config->arg, sizeof(config->arg));

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);

    if (R_SUCCEEDED(rc)) {
        appletHolderJoin(&holder);

        LibAppletExitReason reason = appletHolderGetExitReason(&holder);

        if (reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }
    }

    appletHolderClose(&holder);

    return rc;
}
