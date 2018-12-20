#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/swkbd.h"

void swkbdCreate(SwkbdConfig* c) {
    memset(c, 0, sizeof(SwkbdConfig));
}

Result swkbdShow(SwkbdConfig* c) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage;
    u8 *workbuf = NULL;//TODO
    size_t workbuf_size = 0x1000;//TODO

    memset(&storage, 0, sizeof(AppletStorage));

    rc = appletCreateLibraryApplet(&holder, AppletId_swkbd, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, 0x5);//1.0.0+ version
    rc = libappletArgsPush(&commonargs, &holder);

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(&holder, &c->arg, sizeof(c->arg));

    if (R_SUCCEEDED(rc)) {
        if (R_SUCCEEDED(rc)) rc = appletCreateTransferMemoryStorage(&storage, workbuf, workbuf_size, true);
        appletHolderPushInData(&holder, &storage);
    }

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);

    if (R_SUCCEEDED(rc)) {
        while(appletHolderWaitInteractiveOut(&holder)) {
            //TODO: Handle Interactive data here.
        }
    }

    if (R_SUCCEEDED(rc)) {
        appletHolderJoin(&holder);

        LibAppletExitReason reason = appletHolderGetExitReason(&holder);

        if (reason == LibAppletExitReason_Canceled) {
            rc = 0x29f;//TODO: Official sw returns this, replace it with something else.
        }
        else if (reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            //TODO: Official sw asserts here - return a proper error here.
            return -1;
        }
        else { //success
            //TODO: Process the output storage here. When the output CloseResult indicates failure, official sw returns same error as above.
        }
    }

    appletHolderClose(&holder);
    appletStorageCloseTmem(&storage);

    return rc;
}

