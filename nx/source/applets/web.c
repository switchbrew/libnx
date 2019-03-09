#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/caps.h"
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

static Result _webShow(AppletHolder *holder, AppletId id, u32 version, void* arg, size_t arg_size, void* reply_buffer, size_t reply_size) {
    Result rc = 0;

    rc = _webLaunch(holder, id, version, arg, arg_size);

    if (R_SUCCEEDED(rc)) rc = _webHandleExit(holder, reply_buffer, reply_size);

    appletHolderClose(holder);

    return rc;
}

void webWifiCreate(WebWifiConfig* config, const char* conntest_url, const char* initial_url, u128 uuid, u32 rev) {
    memset(config, 0, sizeof(*config));

    if (conntest_url) strncpy(config->arg.conntest_url, conntest_url, sizeof(config->arg.conntest_url)-1);
    strncpy(config->arg.initial_url, initial_url, sizeof(config->arg.initial_url)-1);

    config->arg.uuid = uuid;
    config->arg.rev = rev;
}

Result webWifiShow(WebWifiConfig* config, WebWifiReturnValue *out) {
    AppletHolder holder;
    return _webShow(&holder, AppletId_wifiWebAuth, 0, &config->arg, sizeof(config->arg), out, sizeof(*out));
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

static Result _webTLVRead(WebCommonTLVStorage *storage, u16 type, void* argdata, u16 argdata_size) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    size_t i, count, offset;
    u8 *dataptr = storage->data;
    WebArgHeader *hdr = (WebArgHeader*)dataptr;
    WebArgTLV *tlv;
    size_t size = sizeof(storage->data);

    offset = sizeof(WebArgHeader);
    if (size < offset) return rc;

    count = hdr->total_entries;
    tlv = (WebArgTLV*)&dataptr[offset];

    for (i=0; i<count; i++) {
        if (size < offset + sizeof(WebArgTLV)) return rc;

        tlv = (WebArgTLV*)&dataptr[offset];

        if (tlv->type == type) {
            if (tlv->size != argdata_size) return rc;
            break;
        }

        offset+= sizeof(WebArgTLV) + tlv->size;
        if (size < offset) return rc;
    }

    if (i==count) return MAKERESULT(Module_Libnx, LibnxError_NotFound);
    if (size < offset + sizeof(WebArgTLV) + argdata_size) return rc;

    offset+= sizeof(WebArgTLV);
    memcpy(argdata, &dataptr[offset], argdata_size);

    return 0;
}

static Result _webTLVReadVarSize(WebCommonTLVStorage *storage, u16 type, void* argdata, size_t argdata_size) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    size_t i, count, offset;
    u8 *dataptr = storage->data;
    WebArgHeader *hdr = (WebArgHeader*)dataptr;
    WebArgTLV *tlv;
    size_t size = sizeof(storage->data);

    offset = sizeof(WebArgHeader);
    if (size < offset) return rc;

    count = hdr->total_entries;
    tlv = (WebArgTLV*)&dataptr[offset];

    for (i=0; i<count; i++) {
        if (size < offset + sizeof(WebArgTLV)) return rc;

        tlv = (WebArgTLV*)&dataptr[offset];

        if (tlv->type == type) {
            if (argdata_size > tlv->size) argdata_size = tlv->size;
            break;
        }

        offset+= sizeof(WebArgTLV) + tlv->size;
        if (size < offset) return rc;
    }

    if (i==count) return MAKERESULT(Module_Libnx, LibnxError_NotFound);
    if (size < offset + sizeof(WebArgTLV) + argdata_size) return rc;

    offset+= sizeof(WebArgTLV);
    memcpy(argdata, &dataptr[offset], argdata_size);

    return 0;
}

static Result _webTLVSet(WebCommonConfig* config, u16 type, const void* argdata, u16 argdata_size) {
    return _webTLVWrite(&config->arg, type, argdata, argdata_size, argdata_size);
}

static Result _webConfigGet(WebCommonConfig* config, u16 type, void* argdata, u16 argdata_size) {
    return _webTLVRead(&config->arg, type, argdata, argdata_size);
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

static Result _webConfigSetU64(WebCommonConfig* config, u16 type, u64 arg) {
    return _webTLVSet(config, type, &arg, sizeof(arg));
}

static Result _webConfigSetString(WebCommonConfig* config, u16 type, const char* str, u16 argdata_size_total) {
    u16 arglen = strlen(str);
    if (arglen >= argdata_size_total) arglen = argdata_size_total-1; //The string must be NUL-terminated.

    return _webTLVWrite(&config->arg, type, str, arglen, argdata_size_total);
}

static Result _webConfigSetUrl(WebCommonConfig* config, const char* url) {
    return _webConfigSetString(config, WebArgType_Url, url, 0xC00);
}

static Result _webConfigGetU8(WebCommonConfig* config, u16 type, u8 *arg) {
    return _webConfigGet(config, type, arg, sizeof(*arg));
}

static Result _webConfigGetFlag(WebCommonConfig* config, u16 type, bool *arg) {
    u8 tmpdata=0;
    Result rc = _webConfigGetU8(config, type, &tmpdata);
    *arg = tmpdata!=0;
    return rc;
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

Result webOfflineCreate(WebCommonConfig* config, WebDocumentKind docKind, u64 titleID, const char* docPath) {
    Result rc=0;

    if (docKind < WebDocumentKind_OfflineHtmlPage || docKind > WebDocumentKind_SystemDataPage)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    _webArgInitialize(config, AppletId_offlineWeb, WebShimKind_Offline);

    rc = webConfigSetLeftStickMode(config, WebLeftStickMode_Cursor);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetFlag(config, WebArgType_BootAsMediaPlayerInverted, false);
    if (R_SUCCEEDED(rc)) rc = webConfigSetPointer(config, docKind == WebDocumentKind_OfflineHtmlPage);

    if (docKind == WebDocumentKind_ApplicationLegalInformation || docKind == WebDocumentKind_SystemDataPage) {
        webConfigSetFooter(config, true);
        webConfigSetBackgroundKind(config, WebBackgroundKind_Default);
    }

    if (R_SUCCEEDED(rc) && docKind == WebDocumentKind_SystemDataPage) rc = webConfigSetBootDisplayKind(config, WebBootDisplayKind_White);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown14, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown15, 1);

    if (R_SUCCEEDED(rc) && docKind == WebDocumentKind_ApplicationLegalInformation) rc = webConfigSetBootDisplayKind(config, WebBootDisplayKind_White);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_UnknownC, 1);

    if (R_SUCCEEDED(rc) && docKind == WebDocumentKind_ApplicationLegalInformation) rc = webConfigSetEcClientCert(config, true);

    if (R_SUCCEEDED(rc) && docKind == WebDocumentKind_OfflineHtmlPage && config->version < 0x30000) rc = _webConfigSetU8(config, WebArgType_Unknown12, 1); // Removed from user-process init with [3.0.0+].

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU32(config, WebArgType_DocumentKind, docKind);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU64(config, docKind != WebDocumentKind_SystemDataPage ? WebArgType_ApplicationId : WebArgType_SystemDataId, titleID);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetString(config, WebArgType_DocumentPath, docPath, 0xC00);

    return rc;
}

Result webShareCreate(WebCommonConfig* config, WebShareStartPage page) {
    Result rc=0;

    _webArgInitialize(config, AppletId_loginShare, WebShimKind_Share);

    rc = webConfigSetLeftStickMode(config, WebLeftStickMode_Cursor);
    if (R_SUCCEEDED(rc)) rc = webConfigSetUserID(config, 0);
    if (R_SUCCEEDED(rc)) rc = webConfigSetDisplayUrlKind(config, true);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown14, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown15, 1);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBootDisplayKind(config, WebBootDisplayKind_Unknown3);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU32(config, WebArgType_ShareStartPage, page);

    return rc;
}

Result webLobbyCreate(WebCommonConfig* config) {
    Result rc=0;
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    _webArgInitialize(config, AppletId_loginShare, WebShimKind_Lobby);

    rc = webConfigSetLeftStickMode(config, WebLeftStickMode_Cursor);
    if (R_SUCCEEDED(rc) && config->version >= 0x30000) rc = webConfigSetPointer(config, false); // Added to user-process init with [3.0.0+].

    if (R_SUCCEEDED(rc)) rc = webConfigSetUserID(config, 0);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown14, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown15, 1);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBootDisplayKind(config, WebBootDisplayKind_Unknown4);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBackgroundKind(config, WebBackgroundKind_Unknown2);
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

Result webConfigSetAlbumEntry(WebCommonConfig* config, CapsAlbumEntry *entry) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webTLVSet(config, WebArgType_AlbumEntry, entry, sizeof(*entry));
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

Result webConfigSetPlayReport(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Offline) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetFlag(config, WebArgType_PlayReport, flag);
}

Result webConfigSetBootDisplayKind(WebCommonConfig* config, WebBootDisplayKind kind) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Share && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetU32(config, WebArgType_BootDisplayKind, kind);
}

Result webConfigSetBackgroundKind(WebCommonConfig* config, WebBackgroundKind kind) {
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

Result webConfigSetLeftStickMode(WebCommonConfig* config, WebLeftStickMode mode) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Share && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
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
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Share && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webConfigSetFlag(config, WebArgType_DisplayUrlKind, kind);
}

Result webConfigSetBootAsMediaPlayer(WebCommonConfig* config, bool flag) {
    Result rc=0;
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    rc = _webConfigSetFlag(config, WebArgType_BootAsMediaPlayer, flag);

    if (R_SUCCEEDED(rc) && config->version >= 0x30000) {//Check if the NewsFlag is set on [3.0.0+], and set WebArgType_BootAsMediaPlayerInverted if so.
        bool tmpval=false;
        Result tmprc = _webConfigGetFlag(config, WebArgType_NewsFlag, &tmpval);
        if (R_SUCCEEDED(tmprc) && tmpval) rc = _webConfigSetFlag(config, WebArgType_BootAsMediaPlayerInverted, !flag);
    }

    return rc;
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

Result webConfigSetApplicationAlbumEntry(WebCommonConfig* config, CapsApplicationAlbumEntry *entry) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webTLVSet(config, WebArgType_ApplicationAlbumEntry, entry, sizeof(*entry));
}

Result webConfigSetJsExtension(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_JsExtension, flag);
}

Result webConfigSetAdditionalCommentText(WebCommonConfig* config, const char* str) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetString(config, WebArgType_AdditionalCommentText, str, 0x100);
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

Result webConfigSetAdditionalMediaData(WebCommonConfig* config, const u8* data, size_t size) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webTLVWrite(&config->arg, WebArgType_AdditionalMediaData, data, size, 0x10);
}

Result webConfigSetMediaPlayerAutoClose(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
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
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetU32(config, WebArgType_FooterFixedKind, kind);
}

Result webConfigSetPageFade(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_PageFade, flag);
}

Result webConfigSetMediaCreatorApplicationRatingAge(WebCommonConfig* config, const s8 *data) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webTLVSet(config, WebArgType_MediaCreatorApplicationRatingAge, data, 0x20);
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

Result webConfigShow(WebCommonConfig* config, WebCommonReply *out) {
    void* reply = NULL;
    size_t size = 0;

    if (out) {
        // ShareApplet on [3.0.0+] uses TLV storage for the reply, while older versions + everything else uses *ReturnValue.
        memset(out, 0, sizeof(*out));
        if (config->version >= 0x30000 && _webGetShimKind(config) == WebShimKind_Share) out->type = true;

        if (!out->type) {
            reply = &out->ret;
            size = sizeof(out->ret);
        }
        else {
            reply = &out->storage;
            size = sizeof(out->storage);
        }
    }

    return _webShow(&config->holder, config->appletid, config->version, &config->arg, sizeof(config->arg), reply, size);
}

Result webConfigRequestExit(WebCommonConfig* config) {
    return appletHolderRequestExit(&config->holder);
}

// For strings only available via TLVs.
static Result _webReplyGetString(WebCommonReply *reply, WebReplyType str_type, WebReplyType strsize_type, char *outstr, size_t outstr_maxsize, size_t *out_size) {
    Result rc=0;

    if (outstr && outstr_maxsize <= 1) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (outstr) memset(outstr, 0, outstr_maxsize);

    if (!reply->type) {
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }
    else {
        if (outstr) rc = _webTLVReadVarSize(&reply->storage, str_type, outstr, outstr_maxsize-1);
        if (R_SUCCEEDED(rc) && out_size) rc = _webTLVRead(&reply->storage, strsize_type, out_size, sizeof(*out_size));
    }
    return rc;
}

Result webReplyGetExitReason(WebCommonReply *reply, WebExitReason *exitReason) {
    if (!reply->type) {
        *exitReason = reply->ret.exitReason;
    }
    else {
        return _webTLVRead(&reply->storage, WebReplyType_ExitReason, exitReason, sizeof(*exitReason));
    }
    return 0;
}

Result webReplyGetLastUrl(WebCommonReply *reply, char *outstr, size_t outstr_maxsize, size_t *out_size) {
    Result rc=0;

    if (outstr && outstr_maxsize <= 1) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (outstr) memset(outstr, 0, outstr_maxsize);

    if (!reply->type) {
        if (outstr) strncpy(outstr, reply->ret.lastUrl, outstr_maxsize-1);
        if (out_size) *out_size = reply->ret.lastUrlSize;
    }
    else {
        if (outstr) rc = _webTLVReadVarSize(&reply->storage, WebReplyType_LastUrl, outstr, outstr_maxsize-1);
        if (R_SUCCEEDED(rc) && out_size) rc = _webTLVRead(&reply->storage, WebReplyType_LastUrlSize, out_size, sizeof(*out_size));
    }
    return rc;
}

Result webReplyGetSharePostResult(WebCommonReply *reply, u32 *sharePostResult) {
    if (reply->type) {
        return _webTLVRead(&reply->storage, WebReplyType_SharePostResult, sharePostResult, sizeof(*sharePostResult));
    }
    return MAKERESULT(Module_Libnx, LibnxError_BadInput);
}

Result webReplyGetPostServiceName(WebCommonReply *reply, char *outstr, size_t outstr_maxsize, size_t *out_size) {
    return _webReplyGetString(reply, WebReplyType_PostServiceName, WebReplyType_PostServiceNameSize, outstr, outstr_maxsize, out_size);
}

Result webReplyGetPostId(WebCommonReply *reply, char *outstr, size_t outstr_maxsize, size_t *out_size) {
    return _webReplyGetString(reply, WebReplyType_PostId, WebReplyType_PostIdSize, outstr, outstr_maxsize, out_size);
}

