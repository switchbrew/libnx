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
    libappletPushInData(&err, c, sizeof(ErrorConfig));

    appletHolderStart(&err);
    appletHolderJoin(&err);
    appletHolderClose(&err);
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
