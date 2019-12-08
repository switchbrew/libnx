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
        if(!accountUidIsValid(&ui->settings.invalid_uid_list[i])) {
            ui->settings.invalid_uid_list[i] = user_id;
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

    // TODO: Official sw supports pushing an optional additional storage from appletCreateTransferMemoryStorage with writable=0 using an input buffer, when that buffer is specified. However, sdknso itself doesn't use this besides a wrapper func. Figure out what this, and implement it?(libappletLaunch could no longer be used with this)

    rc = libappletLaunch(AppletId_playerSelect, &args, arg_ptr, arg_size, &ret, sizeof(ret), &reply_size);
    
    if (R_SUCCEEDED(rc)) {
        rc = ret.res; // Official sw returns this directly.
        if (R_SUCCEEDED(rc) && out_user) *out_user = ret.user_id;
    }

    return rc;
}

static Result _pselUserSelectorCommonInit(PselUiSettings *ui, const PselUserSelectionSettings *settings) {
    Result rc=0;
    bool tmp=0;

    rc = accountIsUserRegistrationRequestPermitted(&tmp);
    if (R_FAILED(rc)) return rc;
    ui->settings.is_permitted = tmp!=0;

    memcpy(&ui->settings.invalid_uid_list, settings->invalid_uid_list, sizeof(settings->invalid_uid_list));
    ui->settings.is_network_service_account_required = settings->is_network_service_account_required;
    ui->settings.is_skip_enabled = settings->is_skip_enabled;

    ui->settings.show_skip_button = settings->show_skip_button;
    ui->settings.additional_select = settings->additional_select;
    if (hosversionAtLeast(6,0,0))
        ui->settings.unk_x97 = settings->is_unqualified_user_selectable ^ 1;

    return rc;
}

static Result _pselShowUserSelectorCommon(PselUiSettings *ui, AccountUid *out_user) {
    Result rc=0;
    bool show_psel=1;

    if (ui->settings.is_skip_enabled!=0) {
        if (accountUidIsValid(&ui->settings.invalid_uid_list[0]) || ui->settings.additional_select)
            rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);

        if (R_SUCCEEDED(rc)) rc = accountTrySelectUserWithoutInteraction(out_user, ui->settings.is_network_service_account_required);
        if (R_SUCCEEDED(rc) && accountUidIsValid(out_user)) show_psel = 0;
    }

    if (R_SUCCEEDED(rc) && show_psel)
        rc = pselUiShow(ui, out_user);

    return rc;
}

Result pselShowUserSelectorForSystem(AccountUid *out_user, const PselUserSelectionSettings *settings, const PselUserSelectionSettingsForSystemService *settings_system) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserSelector);
    if(R_SUCCEEDED(rc)) {
        rc = _pselUserSelectorCommonInit(&ui, settings);
        if(R_SUCCEEDED(rc)) {
            ui.settings.unk_x92 = 1;
            if (hosversionAtLeast(2,0,0)) {
                ui.settings.unk_x96 = settings_system->enable_user_creation_button;
                ui.unk_x98 = settings_system->purpose;
            }
        }

        if(R_SUCCEEDED(rc)) rc = _pselShowUserSelectorCommon(&ui, out_user);
    }
    return rc;
}

Result pselShowUserSelectorForLauncher(AccountUid *out_user, const PselUserSelectionSettings *settings, u64 application_id) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserSelector);
    if(R_SUCCEEDED(rc)) {
        rc = _pselUserSelectorCommonInit(&ui, settings);
        if(R_SUCCEEDED(rc)) {
            ui.settings.application_id = application_id;
            ui.settings.unk_x92 = 1;
            if (hosversionAtLeast(2,0,0))
                ui.settings.unk_x96 = 1;
        }

        if(R_SUCCEEDED(rc)) rc = _pselShowUserSelectorCommon(&ui, out_user);
    }
    return rc;
}

Result pselShowUserSelector(AccountUid *out_user, const PselUserSelectionSettings *settings) {
    PselUiSettings ui;
    Result rc = pselUiCreate(&ui, PselUiMode_UserSelector);
    if(R_SUCCEEDED(rc)) {
        rc = _pselUserSelectorCommonInit(&ui, settings);
        if(R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0)) ui.settings.unk_x96 = 1;

        if(R_SUCCEEDED(rc)) rc = _pselShowUserSelectorCommon(&ui, out_user);
    }
    return rc;
}

Result pselShowUserCreator(void) {
    PselUiSettings ui;
    bool tmp=0;
    Result rc = pselUiCreate(&ui, PselUiMode_UserCreator);
    if(R_SUCCEEDED(rc)) {
        rc = accountIsUserRegistrationRequestPermitted(&tmp);
        if(R_SUCCEEDED(rc) && tmp==0) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);

        if(R_SUCCEEDED(rc)) rc = pselUiShow(&ui, NULL);
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
