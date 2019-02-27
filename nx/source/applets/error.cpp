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
    LibAppletArgs errArgs;

    appletCreateLibraryApplet(&err, AppletId_error, LibAppletMode_AllForeground);
    libappletArgsCreate(&errArgs, 1);
    libappletArgsPush(&errArgs, &err);
    libappletPushInData(&err, c, 0x1018);

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

void errorConfigSetCustomText(ErrorConfig* c, bool custom_text) {
    c->custom_text = custom_text;
}

void errorConfigSetShortDescription(ErrorConfig* c, const char* str) {
    &c->short_description = str;
}

void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str) {
    &c->detailed_description = str;
}
