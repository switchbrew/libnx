#include <malloc.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/error.h"

void errorCreate(ErrorConfig* c) {
    memset(c, 0, sizeof(ErrorConfig));
    c->custom_text = false;
    c->major_code = 2000;
    c->minor_code = 0;
    strcpy((char*) &c->short_description, "");
    strcpy((char*) &c->detailed_description, "");
}

void errorClose(ErrorConfig* c) {
    memset(c, 0, sizeof(ErrorConfig));
}

void errorShow(ErrorConfig* c) {
    AppletHolder err;
    AppletStorage errStor;
    LibAppletArgs errArgs;

    u8 args[0x1018] = {0};
    memcpy(&args, c, 0x1018);

    appletCreateLibraryApplet(&err, AppletId_error, LibAppletMode_AllForeground);
    libappletArgsCreate(&errArgs, 1);
    libappletArgsPush(&errArgs, &err);

    appletCreateStorage(&errStor, 4120);
    appletStorageWrite(&errStor, 0, args, 0x1018);

    appletHolderPushInData(&err, &errStor);

    appletHolderStart(&err);
    appletHolderJoin(&err);
    appletHolderClose(&err);
}

void errorConfigSetMajorCode(ErrorConfig* c, u32 code) {
    c->major_code = code;
}

void errorConfigSetMinorCode(ErrorConfig* c, u32 code) {
    c->minor_code = code;
}

void errorConfigSetCustomText(ErrorConfig* c, bool customText) {
    c->custom_text = customText;
}

void errorConfigSetShortDescription(ErrorConfig* c, const char* str) {
    strcpy((char*) &c->short_description, str);
}

void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str) {
    strcpy((char*) &c->detailed_description, str);
}
