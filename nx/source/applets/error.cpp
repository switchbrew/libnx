#include <malloc.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/error.h"

void errorCreate(ErrorConfig* c) {
    c->major_code = 2000;
    c->minor_code = 0;
    c->custom_text = false;

    c->detailed_description = (char*) malloc(sizeof(char) * (strlen("") + 1));
    strcpy(c->detailed_description, "");

    c->short_description = (char*) malloc(sizeof(char) * (strlen("") + 1));
    strcpy(c->short_description, "");
}

void errorClose(ErrorConfig* c) {
    free(c->short_description);
    free(c->detailed_description);
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

    u8 args[0x1018] = {0};
    args[0] = (c->custom_text ? 1 : 0);

    *(u64*) &args[8] = c->major_code;
    *(u64*) &args[0xC] = c->minor_code;
    strcpy((char*) &args[0x18], c->short_description);
    strcpy((char*) &args[0x818], c->detailed_description);
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
    free(c->short_description);
    c->short_description = (char*) malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(c->short_description, str);
}

void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str) {
    free(c->detailed_description);
    c->detailed_description = (char*) malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(c->detailed_description, str);
}
