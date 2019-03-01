#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/web.h"
#include "runtime/hosversion.h"

static Result _webLaunch(AppletHolder* holder, AppletId id, u32 version, void* arg, size_t arg_size) {
    Result rc=0;

    rc = appletCreateLibraryApplet(holder, id, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, version);
    rc = libappletArgsPush(&commonargs, holder);

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(holder, arg, arg_size);

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(holder);

    return rc;
}

static Result _webHandleExit(AppletHolder* holder, void* reply_buffer, size_t reply_size) {
    Result rc=0;
    size_t transfer_size=0;
    appletHolderJoin(holder);

    LibAppletExitReason reason = appletHolderGetExitReason(holder);

    if (reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
        rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    }

    if (R_SUCCEEDED(rc) && reply_buffer && reply_size) {
        rc = libappletPopOutData(holder, reply_buffer, reply_size, &transfer_size);
        if (R_SUCCEEDED(rc) && transfer_size != reply_size) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    return rc;
}

static Result _webShow(AppletId id, u32 version, void* arg, size_t arg_size, void* reply_buffer, size_t reply_size) {
    Result rc = 0;
    AppletHolder holder;

    rc = _webLaunch(&holder, id, version, arg, arg_size);

    if (R_SUCCEEDED(rc)) rc = _webHandleExit(&holder, reply_buffer, reply_size);

    appletHolderClose(&holder);

    return rc;
}

void webWifiCreate(WebWifiConfig* config, const char* conntest_url, const char* initial_url, u128 userID, u32 unk) {
    memset(config, 0, sizeof(*config));

    if (conntest_url==NULL) conntest_url = initial_url;

    strncpy(config->arg.conntest_url, conntest_url, sizeof(config->arg.conntest_url)-1);
    strncpy(config->arg.initial_url, initial_url, sizeof(config->arg.initial_url)-1);

    config->arg.userID = userID;
    config->arg.unk_x514 = unk;
}

Result webWifiShow(WebWifiConfig* config, WebWifiReturnValue *out) {
    return _webShow(AppletId_wifiWebAuth, 0, &config->arg, sizeof(config->arg), out, sizeof(*out));
}

static void _webArgInitialize(WebCommonConfig* config, AppletId appletid, WebShimKind shimKind) {
    memset(config, 0, sizeof(*config));

    WebArgHeader *hdr = (WebArgHeader*)config->arg.data;
    hdr->shimKind = shimKind;
    config->appletid = appletid;

    u32 hosver = hosversionGet();
    if (hosver >= MAKEHOSVERSION(5,0,0))
        config->version = 0x50000;
    else if (hosver >= MAKEHOSVERSION(3,0,0))
        config->version = 0x30000;
    else
        config->version = 0x20000; // [1.0.0+] version
}

WebShimKind _webGetShimKind(WebCommonConfig* config) {
    WebArgHeader *hdr = (WebArgHeader*)config->arg.data;
    return hdr->shimKind;
}

static Result _webTLVWrite(WebCommonTLVStorage *storage, u16 type, const void* argdata, u16 argdata_size, u16 argdata_size_total) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    size_t i, count, offset;
    u8 *dataptr = storage->data;
    WebArgHeader *hdr = (WebArgHeader*)dataptr;
    WebArgTLV *tlv;
    size_t size = sizeof(storage->data);

    offset = sizeof(WebArgHeader);
    if (size < offset) return rc;
    if (argdata_size > argdata_size_total) argdata_size = argdata_size_total;

    count = hdr->total_entries;
    tlv = (WebArgTLV*)&dataptr[offset];

    for (i=0; i<count; i++) {
        if (size < offset + sizeof(WebArgTLV)) return rc;

        tlv = (WebArgTLV*)&dataptr[offset];

        if (tlv->type == type) {
            if (tlv->size != argdata_size_total) return rc;
            break;
        }

        offset+= sizeof(WebArgTLV) + tlv->size;
        if (size < offset) return rc;
    }

    if (size < offset + sizeof(WebArgTLV) + argdata_size_total) return rc;

    tlv = (WebArgTLV*)&dataptr[offset];

    if (tlv->type != type) {
        if (hdr->total_entries == 0xFFFF) return rc;

        tlv->type = type;
        tlv->size = argdata_size_total;
        hdr->total_entries++;
    }

    offset+= sizeof(WebArgTLV);
    memcpy(&dataptr[offset], argdata, argdata_size);
    if (argdata_size_total != argdata_size) memset(&dataptr[offset+argdata_size], 0, argdata_size_total-argdata_size);

    return 0;
}

static Result _webTLVSet(WebCommonConfig* config, u16 type, const void* argdata, u16 argdata_size) {
    return _webTLVWrite(&config->arg, type, argdata, argdata_size, argdata_size);
}

static Result _webConfigSetU8(WebCommonConfig* config, u16 type, u8 arg) {
    return _webTLVSet(config, type, &arg, sizeof(arg));
}

static Result _webConfigSetFlag(WebCommonConfig* config, u16 type, bool arg) {
    return _webConfigSetU8(config, type, arg!=0);
}

static Result _webConfigSetU32(WebCommonConfig* config, u16 type, u32 arg) {
    return _webTLVSet(config, type, &arg, sizeof(arg));
}

static Result _webConfigSetString(WebCommonConfig* config, u16 type, const char* str, u16 argdata_size_total) {
    u16 arglen = strlen(str);
    if (arglen >= argdata_size_total) arglen = argdata_size_total-1; //The string must be NUL-terminated.

    return _webTLVWrite(&config->arg, type, str, arglen, argdata_size_total);
}

static Result _webConfigSetUrl(WebCommonConfig* config, const char* url) {
    return _webConfigSetString(config, WebArgType_Url, url, 0xc00);
}

Result webPageCreate(WebCommonConfig* config, const char* url) {
    Result rc=0;
    _webArgInitialize(config, AppletId_web, WebShimKind_Web);

    rc = _webConfigSetU8(config, WebArgType_UnknownD, 1);
    if (R_SUCCEEDED(rc) && config->version < 0x30000) rc = _webConfigSetU8(config, WebArgType_Unknown12, 1); // Removed from user-process init with [3.0.0+].

    if (R_SUCCEEDED(rc)) rc = _webConfigSetUrl(config, url);

    return rc;
}

Result webNewsCreate(WebCommonConfig* config, const char* url) {
    Result rc=0;
    _webArgInitialize(config, AppletId_web, WebShimKind_Web);

    rc = _webConfigSetU8(config, WebArgType_UnknownD, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetFlag(config, WebArgType_NewsFlag, true);
    if (R_SUCCEEDED(rc)) rc = webConfigSetEcClientCert(config, true);
    if (R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0)) rc = webConfigSetShopJump(config, true); // Check version so that rc isn't set to an error on pre-2.0.0.

    if (R_SUCCEEDED(rc)) rc = _webConfigSetUrl(config, url);

    return rc;
}

Result webYouTubeVideoCreate(WebCommonConfig* config, const char* url) {
    Result rc=0;
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    _webArgInitialize(config, AppletId_web, WebShimKind_Web);

    rc = _webConfigSetU8(config, WebArgType_UnknownD, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetFlag(config, WebArgType_YouTubeVideoFlag, true);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBootAsMediaPlayer(config, true);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetUrl(config, url);

    return rc;
}

Result webLobbyCreate(WebCommonConfig* config) {
    Result rc=0;
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    _webArgInitialize(config, AppletId_loginShare, WebShimKind_Lobby);

    rc = webConfigSetLeftStickMode(config, 1);
    if (R_SUCCEEDED(rc) && config->version >= 0x30000) rc = webConfigSetPointer(config, false); // Added to user-process init with [3.0.0+].

    if (R_SUCCEEDED(rc)) rc = webConfigSetUserID(config, 0);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown14, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown15, 1);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBootDisplayKind(config, WebBootDisplayKind_Unknown4);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBackgroundKind(config, 2);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetFlag(config, WebArgType_BootAsMediaPlayerInverted, false);

    return rc;
}

Result webConfigSetCallbackUrl(WebCommonConfig* config, const char* url) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Share && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetString(config, WebArgType_CallbackUrl, url, 0x400);
}

Result webConfigSetCallbackableUrl(WebCommonConfig* config, const char* url) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetString(config, WebArgType_CallbackableUrl, url, 0x400);
}

Result webConfigSetWhitelist(WebCommonConfig* config, const char* whitelist) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetString(config, WebArgType_Whitelist, whitelist, 0x1000);
}

Result webConfigSetUserID(WebCommonConfig* config, u128 userID) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Share && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webTLVSet(config, WebArgType_UserID, &userID, sizeof(userID));
}

Result webConfigSetScreenShot(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetFlag(config, WebArgType_ScreenShot, flag);
}

Result webConfigSetEcClientCert(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetFlag(config, WebArgType_EcClientCert, flag);
}

Result webConfigSetBootDisplayKind(WebCommonConfig* config, u32 kind) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetU32(config, WebArgType_BootDisplayKind, kind);
}

Result webConfigSetBackgroundKind(WebCommonConfig* config, u32 kind) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetU32(config, WebArgType_BackgroundKind, kind);
}

Result webConfigSetFooter(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetFlag(config, WebArgType_Footer, flag);
}

Result webConfigSetPointer(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetFlag(config, WebArgType_Pointer, flag);
}

Result webConfigSetLeftStickMode(WebCommonConfig* config, u32 mode) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetU32(config, WebArgType_LeftStickMode, mode);
}

Result webConfigSetKeyRepeatFrame(WebCommonConfig* config, s32 inval0, s32 inval1) {
    Result rc=0;
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    rc = _webConfigSetU32(config, WebArgType_KeyRepeatFrame0, inval0);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetU32(config, WebArgType_KeyRepeatFrame1, inval1);
    return rc;
}

Result webConfigSetDisplayUrlKind(WebCommonConfig* config, bool kind) {
    return _webConfigSetFlag(config, WebArgType_DisplayUrlKind, kind);
}

Result webConfigSetBootAsMediaPlayer(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_BootAsMediaPlayer, flag);
    //TODO: Need to check for News somehow and set WebArgType_BootAsMediaPlayerInverted.
}

Result webConfigSetShopJump(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_ShopJump, flag);
}

Result webConfigSetMediaPlayerUserGestureRestriction(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_MediaPlayerUserGestureRestriction, flag);
}

Result webConfigSetLobbyParameter(WebCommonConfig* config, const char* str) {
    if (_webGetShimKind(config) != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetString(config, WebArgType_LobbyParameter, str, 0x100);
}

Result webConfigSetJsExtension(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_JsExtension, flag);
}

Result webConfigSetTouchEnabledOnContents(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_TouchEnabledOnContents, flag);
}

Result webConfigSetUserAgentAdditionalString(WebCommonConfig* config, const char* str) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetString(config, WebArgType_UserAgentAdditionalString, str, 0x80);
}

Result webConfigSetMediaPlayerAutoClose(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_MediaPlayerAutoClose, flag);
}

Result webConfigSetPageCache(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_PageCache, flag);
}

Result webConfigSetWebAudio(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_WebAudio, flag);
}

Result webConfigSetFooterFixedKind(WebCommonConfig* config, u32 kind) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetU32(config, WebArgType_FooterFixedKind, kind);
}

Result webConfigSetPageFade(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_PageFade, flag);
}

Result webConfigSetBootLoadingIcon(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Offline) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_BootLoadingIcon, flag);
}

Result webConfigSetPageScrollIndicator(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_PageScrollIndicator, flag);
}

Result webConfigShow(WebCommonConfig* config, WebCommonReturnValue *out) {
    return _webShow(config->appletid, config->version, &config->arg, sizeof(config->arg), out, sizeof(*out));
}

