#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "services/set.h"
#include "applets/libapplet.h"
#include "applets/psel.h"
#include "runtime/hosversion.h"

Result pselUiCreate(PselUiSettings *ui, PselUiMode mode) {
    memset(ui, 0, sizeof(PselUiSettings));
    ui->mode = mode;
    return 0;
}

void pselUiAddInvalidUser(PselUiSettings *ui, u128 user_id) {
    int i;
    for(i = 0; i < ACC_USER_LIST_SIZE; i++) {

        if(ui->invalidUserList[i] == 0) {
            __builtin_memcpy(&ui->invalidUserList[i], &user_id, sizeof(user_id));
            break;
        }
    }
}

void pselUiSetAllowUserCreation(PselUiSettings *ui, bool flag) {
    if(ui->mode == PselUiMode_SelectUser) {
        ui->allowUserCreation = flag;
    }
}

void pselUiSetNetworkServiceRequired(PselUiSettings *ui, bool flag) {
    ui->networkServiceRequired = flag;
}

void pselUiSetSkipEnabled(PselUiSettings *ui, bool flag) {
    ui->skipEnabled = flag;
}

static u32 _pselGetLaVersion() {
    u32 ver = 0;
    u32 hosver = hosversionGet();
    if(hosver >= MAKEHOSVERSION(9,0,0)) {
        ver = 0x20000;
    }
    return ver;
}

Result pselUiShow(PselUiSettings *ui, u128 *out_user) {
    Result rc = 0;
    LibAppletArgs args;
    u32 la_ver = _pselGetLaVersion();
    libappletArgsCreate(&args, la_ver);
    PselResult res;
    size_t reply_size;

    rc = libappletLaunch(AppletId_playerSelect, &args, ui, sizeof(PselUiSettings), &res, sizeof(res), &reply_size);
    
    if (R_SUCCEEDED(rc)) {
        if (R_FAILED(res.result)) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        if (R_SUCCEEDED(rc)) *out_user = res.userId;
    }

    return rc;
}
