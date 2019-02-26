#include <malloc.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/error.h"

void errorCreate(ErrorConfig* c) {
    memset(c->args, 0, 0x1018);

    c->args[0x0] = 0;
    *(u64*) &c->args[0x8] = 2000;
    *(u64*) &c->args[0xC] = 0;
    strcpy((char*) &c->args[0x18], "");
    strcpy((char*) &c->args[0x818], "");
}

void errorClose(ErrorConfig* c) {
    memset(c, 0, sizeof(ErrorConfig));
}

void errorShow(ErrorConfig* c) {
    AppletHolder err;
    AppletStorage errStor;
    LibAppletArgs errArgs;

    appletCreateLibraryApplet(&err, AppletId_error, LibAppletMode_AllForeground);
    libappletArgsCreate(&errArgs, 1);
    libappletArgsPush(&errArgs, &err);

    appletCreateStorage(&errStor, 4120);
    appletStorageWrite(&errStor, 0, c->args, 0x1018);

    appletHolderPushInData(&err, &errStor);

    appletHolderStart(&err);
    appletHolderJoin(&err);
    appletHolderClose(&err);
}

void errorConfigSetMajorCode(ErrorConfig* c, u64 code) {
    *(u64*) &c->args[0x8] = code;
}

void errorConfigSetMinorCode(ErrorConfig* c, u64 code) {
    *(u64*) &c->args[0xC] = code;
}

void errorConfigSetCustomText(ErrorConfig* c, bool customText) {
    c->args[0x0] = customText;
}

void errorConfigSetShortDescription(ErrorConfig* c, const char* str) {
    strcpy((char*) &c->args[0x18], str);
}

void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str) {
    strcpy((char*) &c->args[0x818], str);
}
