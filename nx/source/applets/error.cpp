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
    c->module = 2000;
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
    strncpy(c->short_description, str, strnlen(str, 0x800));
}

void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str) {
    strncpy(c->detailed_description, str, strnlen(str, 0x800));
}
