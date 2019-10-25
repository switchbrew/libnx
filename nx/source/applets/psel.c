#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "services/acc.h"
#include "applets/libapplet.h"
#include "applets/psel.h"
#include "runtime/hosversion.h"

Result pselUiCreate(PselUiSettings *ui, PselUiMode mode) {
    memset(ui, 0, sizeof(PselUiSettings));
    ui->mode = mode;
    return 0;
}

void pselUiAddInvalidUser(PselUiSettings *ui, AccountUid *user_id) {
    int i;
    for(i = 0; i < ACC_USER_LIST_SIZE; i++) {

        if(!accountUidIsValid(&ui->invalidUserList[i])) {
            __builtin_memcpy(&ui->invalidUserList[i], user_id, sizeof(AccountUid));
            break;
        }
    }
}

static u32 _pselGetLaVersion() {
    u32 ver = 0;
    u32 hosver = hosversionGet();
    if(hosver >= MAKEHOSVERSION(9,0,0)) {
        ver = 0x20000;
    }
    return ver;
}

Result pselUiShow(PselUiSettings *ui, AccountUid *out_user) {
    Result rc = 0;
    LibAppletArgs args;
    u32 la_ver = _pselGetLaVersion();
    libappletArgsCreate(&args, la_ver);
    PselResult res;
    size_t reply_size;

    rc = libappletLaunch(AppletId_playerSelect, &args, ui, sizeof(PselUiSettings), &res, sizeof(res), &reply_size);
    
    if (R_SUCCEEDED(rc)) {
        if (res.result != 0) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        if (R_SUCCEEDED(rc)) *out_user = res.userId;
    }

    return rc;
}
