#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "services/set.h"
#include "applets/libapplet.h"
#include "applets/psel.h"
#include "runtime/hosversion.h"

Result pselConfigCreate(PselConfig *c, PselMode mode) {
    memset(c, 0, sizeof(PselConfig));
    c->config.mode = mode;
    return 0;
}

Result pselConfigShow(PselConfig *c, u128 *out_uid) {
    Result rc = 0;
    LibAppletArgs args;
    libappletArgsCreate(&args, 0);
    PselResult res;
    size_t reply_size;

    rc = libappletLaunch(AppletId_playerSelect, &args, &c->config, sizeof(c->config), &res, sizeof(res), &reply_size);
    
    if (R_SUCCEEDED(rc)) {
        if (R_FAILED(res.result)) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        if (R_SUCCEEDED(rc)) *out_uid = res.userId;
    }

    return rc;
}
