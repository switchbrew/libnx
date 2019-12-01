#include <string.h>
#include "libapplet_internal.h"
#include "applets/psel.h"
#include "runtime/hosversion.h"

Result pselUiCreate(PselUiSettings *ui, PselUiMode mode) {
    memset(ui, 0, sizeof(PselUiSettings));
    ui->settings.mode = mode;
    return 0;
}

void pselUiAddUser(PselUiSettings *ui, AccountUid user_id) {
    for(u32 i=0; i<ACC_USER_LIST_SIZE; i++) {
        if(!accountUidIsValid(&ui->settings.user_list[i])) {
            ui->settings.user_list[i] = user_id;
            break;
        }
    }
}

static u32 _pselGetLaVersion() {
    u32 ver = 0x1; // [1.0.0]
    if (hosversionAtLeast(6,0,0))
        ver = 0x20000;
    else if (hosversionAtLeast(2,0,0))
        ver = 0x10000;
    return ver;
}

Result pselUiShow(PselUiSettings *ui, AccountUid *out_user) {
    Result rc = 0;
    LibAppletArgs args;
    u32 la_ver = _pselGetLaVersion();
    libappletArgsCreate(&args, la_ver);
    PselUiReturnArg ret;
    size_t reply_size;
    const void* arg_ptr = ui;
    size_t arg_size = sizeof(*ui);

    if (la_ver == 0x1) { // [1.0.0]
        arg_ptr = &ui->settings;
        arg_size = sizeof(ui->settings);
    }

    rc = libappletLaunch(AppletId_playerSelect, &args, arg_ptr, arg_size, &ret, sizeof(ret), &reply_size);
    
    if (R_SUCCEEDED(rc)) {
        rc = ret.res; // Official sw returns this directly.
        if (R_SUCCEEDED(rc) && out_user) *out_user = ret.user_id;
    }

    return rc;
}

Result pselShowUserSelector(AccountUid *out_user) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserSelector);
    // TODO: This is missing the rest of the UiSettings setup done by official sw.
    if(R_SUCCEEDED(rc)) {
        rc = pselUiShow(&ui, out_user);
    }
    return rc;
}

Result pselShowUserCreator(void) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserCreator);
    if(R_SUCCEEDED(rc)) {
        rc = pselUiShow(&ui, NULL);
    }
    return rc;
}

Result pselShowUserIconEditor(AccountUid user) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserIconEditor);
    if(R_SUCCEEDED(rc)) {
        pselUiAddUser(&ui, user);
        rc = pselUiShow(&ui, NULL);
    }
    return rc;
}

Result pselShowUserNicknameEditor(AccountUid user) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserNicknameEditor);
    if(R_SUCCEEDED(rc)) {
        pselUiAddUser(&ui, user);
        rc = pselUiShow(&ui, NULL);
    }
    return rc;
}

Result pselShowUserCreatorForStarter(void) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserCreatorForStarter);
    if(R_SUCCEEDED(rc)) {
        rc = pselUiShow(&ui, NULL);
    }
    return rc;
}

Result pselShowNintendoAccountNnidLinker(AccountUid user) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_NintendoAccountNnidLinker);
    if(R_SUCCEEDED(rc)) {
        pselUiAddUser(&ui, user);
        rc = pselUiShow(&ui, NULL);
    }
    return rc;
}
