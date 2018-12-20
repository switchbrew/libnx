#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/counter.h"
#include "services/applet.h"
#include "applets/libapplet.h"

void libappletArgsCreate(LibAppletArgs* a, u32 version) {
    memset(a, 0, sizeof(LibAppletArgs));

    a->CommonArgs_version = 1;
    a->CommonArgs_size = sizeof(LibAppletArgs);

    a->LaVersion = version;
    a->ExpectedThemeColor = appletGetThemeColorType();
}

void libappletArgsSetPlayStartupSound(LibAppletArgs* a, bool flag) {
    a->PlayStartupSound = flag!=0;
}

Result libappletArgsPush(LibAppletArgs* a, AppletHolder *h) {
    Result rc=0;
    AppletStorage storage;

    //Official sw stores the header in LibAppletArgs seperately (first 8-bytes), but here we're including it with the LibAppletCommonArguments struct.
    //Official sw uses appletStorageWrite twice, for writing the header then the rest of the struct.

    a->tick = armGetSystemTick();

    rc = appletCreateStorage(&storage, sizeof(LibAppletArgs));
    if (R_FAILED(rc)) return rc;

    rc = appletStorageWrite(&storage, 0, a, sizeof(LibAppletArgs));
    if (R_FAILED(rc)) {
        appletStorageClose(&storage);
        return rc;
    }

    return appletHolderPushInData(h, &storage);
}

