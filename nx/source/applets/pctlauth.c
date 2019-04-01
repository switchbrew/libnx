#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/pctlauth.h"
#include "runtime/hosversion.h"

static Result _pctlauthShow(PctlAuthArg* arg) {
    Result rc=0;
    Result tmpres=0;
    size_t out_reply_size=0;
    u32 ver=1;
    LibAppletArgs commonargs;

    if (hosversionAtLeast(4,0,0)) ver=2;
    libappletArgsCreate(&commonargs, ver);

    rc = libappletLaunch(AppletId_auth, &commonargs, arg, sizeof(*arg), &tmpres, sizeof(tmpres), &out_reply_size);
    if (R_SUCCEEDED(rc)) {
        if (out_reply_size != sizeof(tmpres)) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        if (R_SUCCEEDED(rc)) rc = tmpres;
    }

    return rc;
}

Result pctlauthShow(bool flag) {
    PctlAuthArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.type = PctlAuthType_Show;
    arg.arg0 = flag!=0;

    return _pctlauthShow(&arg);
}

Result pctlauthShowEx(u8 arg0, u8 arg1, u8 arg2) {
    PctlAuthArg arg;

    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    memset(&arg, 0, sizeof(arg));
    arg.type = PctlAuthType_Show;
    arg.arg0 = arg0;
    arg.arg1 = arg1;
    arg.arg2 = arg2;

    return _pctlauthShow(&arg);
}

Result pctlauthShowForConfiguration(void) {
    return pctlauthShowEx(1, 0, 1);
}

Result pctlauthRegisterPasscode(void) {
    PctlAuthArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.type = PctlAuthType_RegisterPasscode;

    return _pctlauthShow(&arg);
}

Result pctlauthChangePasscode(void) {
    PctlAuthArg arg;

    memset(&arg, 0, sizeof(arg));
    arg.type = PctlAuthType_ChangePasscode;

    return _pctlauthShow(&arg);
}


