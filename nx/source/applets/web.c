#include <string.h>
#include "libapplet_internal.h"
#include "applets/web.h"
#include "runtime/hosversion.h"

static Result _webLaunch(AppletHolder* holder, AppletId id, LibAppletMode mode, u32 version, void* arg, size_t arg_size) {
    Result rc=0;

    rc = appletCreateLibraryApplet(holder, id, mode);
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

    rc = _webLaunch(holder, id, LibAppletMode_AllForeground, version, arg, arg_size);

    if (R_SUCCEEDED(rc)) rc = _webHandleExit(holder, reply_buffer, reply_size);

    appletHolderClose(holder);

    return rc;
}

void webWifiCreate(WebWifiConfig* config, const char* conntest_url, const char* initial_url, Uuid uuid, u32 rev) {
    memset(config, 0, sizeof(*config));

    if (conntest_url) strncpy(config->arg.conntest_url, conntest_url, sizeof(config->arg.conntest_url)-1);
    strncpy(config->arg.initial_url, initial_url, sizeof(config->arg.initial_url)-1);

    config->arg.uuid = uuid;
    config->arg.rev = rev;
}

Result webWifiShow(WebWifiConfig* config, WebWifiReturnValue *out) {
    AppletHolder holder;
    return _webShow(&holder, AppletId_LibraryAppletWifiWebAuth, 0, &config->arg, sizeof(config->arg), out, sizeof(*out));
}

static void _webArgInitialize(WebCommonConfig* config, AppletId appletid, WebShimKind shimKind) {
    memset(config, 0, sizeof(*config));

    WebArgHeader *hdr = (WebArgHeader*)config->arg.data;
    hdr->shimKind = shimKind;
    config->appletid = appletid;

    u32 hosver = hosversionGet();
    if (hosver >= MAKEHOSVERSION(8,0,0))
        config->version = 0x80000;
    else if (hosver >= MAKEHOSVERSION(6,0,0))
        config->version = 0x60000;
    else if (hosver >= MAKEHOSVERSION(5,0,0))
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
    if (argdata) memcpy(argdata, &dataptr[offset], argdata_size);

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

static Result _webConfigSetFloat(WebCommonConfig* config, u16 type, float arg) {
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

static Result _webConfigGetU32(WebCommonConfig* config, u16 type, u32 *arg) {
    return _webConfigGet(config, type, arg, sizeof(*arg));
}

Result webPageCreate(WebCommonConfig* config, const char* url) {
    Result rc=0;
    _webArgInitialize(config, AppletId_LibraryAppletWeb, WebShimKind_Web);

    rc = _webConfigSetU8(config, WebArgType_UnknownD, 1);
    if (R_SUCCEEDED(rc) && config->version < 0x30000) rc = _webConfigSetU8(config, WebArgType_Unknown12, 1); // Removed from user-process init with [3.0.0+].

    if (R_SUCCEEDED(rc)) rc = _webConfigSetUrl(config, url);

    return rc;
}

Result webNewsCreate(WebCommonConfig* config, const char* url) {
    Result rc=0;
    _webArgInitialize(config, AppletId_LibraryAppletWeb, WebShimKind_Web);

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

    _webArgInitialize(config, AppletId_LibraryAppletWeb, WebShimKind_Web);

    rc = _webConfigSetU8(config, WebArgType_UnknownD, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetFlag(config, WebArgType_YouTubeVideoFlag, true);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBootAsMediaPlayer(config, true);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetUrl(config, url);

    return rc;
}

Result webOfflineCreate(WebCommonConfig* config, WebDocumentKind docKind, u64 id, const char* docPath) {
    Result rc=0;

    if (docKind < WebDocumentKind_OfflineHtmlPage || docKind > WebDocumentKind_SystemDataPage)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    _webArgInitialize(config, AppletId_LibraryAppletOfflineWeb, WebShimKind_Offline);

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

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU64(config, docKind != WebDocumentKind_SystemDataPage ? WebArgType_ApplicationId : WebArgType_SystemDataId, id);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetString(config, WebArgType_DocumentPath, docPath, 0xC00);

    return rc;
}

Result webShareCreate(WebCommonConfig* config, WebShareStartPage page) {
    Result rc=0;
    AccountUid uid={0};

    _webArgInitialize(config, AppletId_LibraryAppletLoginShare, WebShimKind_Share);

    rc = webConfigSetLeftStickMode(config, WebLeftStickMode_Cursor);
    if (R_SUCCEEDED(rc)) rc = webConfigSetUid(config, uid);
    if (R_SUCCEEDED(rc)) rc = webConfigSetDisplayUrlKind(config, true);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown14, 1);
    if (R_SUCCEEDED(rc)) rc = _webConfigSetU8(config, WebArgType_Unknown15, 1);
    if (R_SUCCEEDED(rc)) rc = webConfigSetBootDisplayKind(config, WebBootDisplayKind_Unknown3);

    if (R_SUCCEEDED(rc)) rc = _webConfigSetU32(config, WebArgType_ShareStartPage, page);

    return rc;
}

Result webLobbyCreate(WebCommonConfig* config) {
    Result rc=0;
    AccountUid uid={0};
    if (hosversionBefore(2,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    _webArgInitialize(config, AppletId_LibraryAppletLoginShare, WebShimKind_Lobby);

    rc = webConfigSetLeftStickMode(config, WebLeftStickMode_Cursor);
    if (R_SUCCEEDED(rc) && config->version >= 0x30000) rc = webConfigSetPointer(config, false); // Added to user-process init with [3.0.0+].

    if (R_SUCCEEDED(rc)) rc = webConfigSetUid(config, uid);

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

Result webConfigSetUid(WebCommonConfig* config, AccountUid uid) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Share && shim != WebShimKind_Web && shim != WebShimKind_Lobby) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webTLVSet(config, WebArgType_Uid, &uid, sizeof(uid));
}

static Result _webConfigSetAlbumEntryTLV(WebCommonConfig* config, WebArgType type, const CapsAlbumEntry *entry) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return _webTLVSet(config, type, entry, sizeof(*entry));
}

Result webConfigSetAlbumEntry(WebCommonConfig* config, const CapsAlbumEntry *entry) {
    return _webConfigSetAlbumEntryTLV(config, WebArgType_AlbumEntry0, entry);
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
    if (hosversionBefore(2,0,0) || hosversionAtLeast(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_MediaPlayerUserGestureRestriction, flag);
}

Result webConfigSetMediaAutoPlay(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_MediaAutoPlay, flag);
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

static Result _webConfigSetAdditionalMediaDataTLV(WebCommonConfig* config, WebArgType type, const u8* data, size_t size) {
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webTLVWrite(&config->arg, type, data, size, 0x10);
}

Result webConfigSetAdditionalMediaData(WebCommonConfig* config, const u8* data, size_t size) {
    return _webConfigSetAdditionalMediaDataTLV(config, WebArgType_AdditionalMediaData0, data, size);
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

Result webConfigSetMediaPlayerSpeedControl(WebCommonConfig* config, bool flag) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_MediaPlayerSpeedControl, flag);
}

Result webConfigAddAlbumEntryAndMediaData(WebCommonConfig* config, const CapsAlbumEntry *entry, const u8* data, size_t size) {
    Result rc=0;
    u32 i;
    WebArgType album_type, media_type;
    WebArgType album_types[4] = {WebArgType_AlbumEntry0, WebArgType_AlbumEntry1, WebArgType_AlbumEntry2, WebArgType_AlbumEntry3};
    WebArgType media_types[4] = {WebArgType_AdditionalMediaData0, WebArgType_AdditionalMediaData1, WebArgType_AdditionalMediaData2, WebArgType_AdditionalMediaData3};
    if (_webGetShimKind(config) != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    // Find a TLV which is not yet set.
    for(i=0; i<4; i++) {
        album_type = album_types[i];
        media_type = media_types[i];
        rc = _webConfigGet(config, album_type, NULL, sizeof(*entry));
        if (R_FAILED(rc)) break;
    }

    if (R_SUCCEEDED(rc)) return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    rc = _webConfigSetAlbumEntryTLV(config, album_type, entry);
    if (R_SUCCEEDED(rc) && data && size)rc = _webConfigSetAdditionalMediaDataTLV(config, media_type, data, size);

    return rc;
}

Result webConfigSetBootFooterButtonVisible(WebCommonConfig* config, WebFooterButtonId button, bool visible) {
    Result rc=0;
    u32 i=0;
    WebBootFooterButtonEntry entries[0x10];
    size_t total_entries = sizeof(entries)/sizeof(WebBootFooterButtonEntry);
    if (_webGetShimKind(config) != WebShimKind_Offline) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (button==WebFooterButtonId_None || button>=WebFooterButtonId_Max) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(entries, 0, sizeof(entries));
    rc = _webConfigGet(config, WebArgType_BootFooterButton, entries, sizeof(entries));
    if (R_FAILED(rc)) rc = 0;
    else {
        for(i=0; i<total_entries; i++) {
            if (entries[i].id == button || entries[i].id == WebFooterButtonId_None) break;
        }
    }

    if (i>=total_entries) return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    if (entries[i].id == WebFooterButtonId_None) entries[i].id = button;
    entries[i].visible = visible!=0;
    //Official sw accesses unk_x5/unk_x7, but it doesn't set those using any user input.

    return _webTLVSet(config, WebArgType_BootFooterButton, entries, sizeof(entries));
}

Result webConfigSetOverrideWebAudioVolume(WebCommonConfig* config, float value) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFloat(config, WebArgType_OverrideWebAudioVolume, value);
}

Result webConfigSetOverrideMediaAudioVolume(WebCommonConfig* config, float value) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFloat(config, WebArgType_OverrideMediaAudioVolume, value);
}

Result webConfigSetBootMode(WebCommonConfig* config, WebSessionBootMode mode) {
    WebShimKind shim = _webGetShimKind(config);
    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(7,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetU32(config, WebArgType_SessionBootMode, mode);
}

Result webConfigSetMediaPlayerUi(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Offline) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(8,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_MediaPlayerUi, flag);
}

Result webConfigSetTransferMemory(WebCommonConfig* config, bool flag) {
    if (_webGetShimKind(config) != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(11,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _webConfigSetFlag(config, WebArgType_TransferMemory, flag);
}

static void _webConfigInitReply(WebCommonConfig* config, WebCommonReply *out, void** reply, size_t *reply_size) {
    if (out) {
        // ShareApplet on [3.0.0+] uses TLV storage for the reply, while older versions + everything else uses *ReturnValue.
        // Web also uses TLV storage for the reply on [8.0.0+].
        WebShimKind shimKind = _webGetShimKind(config);
        memset(out, 0, sizeof(*out));
        out->shimKind = shimKind;

        if (config->version >= 0x30000 && shimKind == WebShimKind_Share) out->type = true;
        if (config->version >= 0x80000 && shimKind == WebShimKind_Web) out->type = true;

        if (!out->type) {
            *reply = &out->ret;
            *reply_size = sizeof(out->ret);
        }
        else {
            *reply = &out->storage;
            *reply_size = sizeof(out->storage);
        }
    }
}

Result webConfigShow(WebCommonConfig* config, WebCommonReply *out) {
    Result rc=0;
    void* reply = NULL;
    size_t size = 0;
    WebShimKind shimKind = _webGetShimKind(config);

    _webConfigInitReply(config, out, &reply, &size);

    rc = _webShow(&config->holder, config->appletid, config->version, &config->arg, sizeof(config->arg), reply, size);
    if (R_SUCCEEDED(rc) && hosversionAtLeast(10,0,0) && out && (shimKind == WebShimKind_Web || shimKind == WebShimKind_Offline)) {
        WebExitReason reason;
        rc = webReplyGetExitReason(out, &reason);
        if (R_SUCCEEDED(rc) && reason == WebExitReason_UnknownE) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    }
    return rc;
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
    if (reply->shimKind != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (reply->type) {
        return _webTLVRead(&reply->storage, WebReplyType_SharePostResult, sharePostResult, sizeof(*sharePostResult));
    }
    return MAKERESULT(Module_Libnx, LibnxError_BadInput);
}

Result webReplyGetPostServiceName(WebCommonReply *reply, char *outstr, size_t outstr_maxsize, size_t *out_size) {
    if (reply->shimKind != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _webReplyGetString(reply, WebReplyType_PostServiceName, WebReplyType_PostServiceNameSize, outstr, outstr_maxsize, out_size);
}

Result webReplyGetPostId(WebCommonReply *reply, char *outstr, size_t outstr_maxsize, size_t *out_size) {
    if (reply->shimKind != WebShimKind_Share) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _webReplyGetString(reply, WebReplyType_PostId, WebReplyType_PostIdSize, outstr, outstr_maxsize, out_size);
}

Result webReplyGetMediaPlayerAutoClosedByCompletion(WebCommonReply *reply, bool *flag) {
    Result rc=0;
    u8 tmpflag=0;
    if (!reply->type) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (reply->shimKind != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _webTLVRead(&reply->storage, WebReplyType_MediaPlayerAutoClosedByCompletion, &tmpflag, sizeof(tmpflag));
    if (R_SUCCEEDED(rc) && flag) *flag = tmpflag!=0;
    return rc;
}

static s32 _webSessionStorageHandleQueueGetCount(WebSessionStorageHandleQueue *s) {
    if (s->is_full) return s->max_storages;
    return (s->write_pos + s->max_storages - s->read_pos) % s->max_storages;
}

static AppletStorage *_webSessionStorageAt(WebSessionStorageHandleQueue *s, s32 pos) {
    return &s->storages[(s->read_pos + pos) % s->max_storages];
}

static void _webSessionStorageHandleQueueCreate(WebSessionStorageHandleQueue *s) {
    memset(s, 0, sizeof(*s));
    s->max_storages = 0x10;
}

static void _webSessionStorageHandleQueueClear(WebSessionStorageHandleQueue *s) {
    while (_webSessionStorageHandleQueueGetCount(s) >= 1) {
        s->is_full = false;
        appletStorageClose(_webSessionStorageAt(s, 0));
        s->read_pos = (s->read_pos + 1) % s->max_storages;
    }
    s->read_pos = 0;
    s->write_pos = 0;
}

static void _webSessionStorageEnqueue(WebSessionStorageHandleQueue *s, AppletStorage *storage) {
    s->write_pos = (s->write_pos + 1) % s->max_storages;
    s->is_full = s->read_pos==s->write_pos;
    AppletStorage *storageptr = _webSessionStorageAt(s, _webSessionStorageHandleQueueGetCount(s)-1);
    appletStorageClose(storageptr); // sdknso doesn't do this - we will though, since otherwise the overwritten storage will not get closed, when the storage queue is full.
    *storageptr = *storage;
}

static AppletStorage *_webSessionStorageDequeue(WebSessionStorageHandleQueue *s) {
    s->is_full = false;
    AppletStorage *storage = _webSessionStorageAt(s, 0);
    s->read_pos = (s->read_pos+1) % s->max_storages;
    return storage;
}

void webSessionCreate(WebSession *s, WebCommonConfig* config) {
    mutexInit(&s->mutex);
    memset(s->queue, 0, sizeof(s->queue));
    s->config = config;
    _webSessionStorageHandleQueueCreate(&s->storage_queue);
}

void webSessionClose(WebSession *s) {
    _webSessionStorageHandleQueueClear(&s->storage_queue);
    s->config = NULL;
    memset(s->queue, 0, sizeof(s->queue));
}

Result webSessionStart(WebSession *s, Event **out_event) {
    Result rc=0;
    WebShimKind shim = _webGetShimKind(s->config);

    if (shim != WebShimKind_Offline && shim != WebShimKind_Web) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(7,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    _webConfigGetU32(s->config, WebArgType_SessionBootMode, &tmp);
    LibAppletMode mode = tmp == WebSessionBootMode_AllForegroundInitiallyHidden ? LibAppletMode_AllForegroundInitiallyHidden : LibAppletMode_AllForeground;

    rc = _webConfigSetFlag(s->config, WebArgType_SessionFlag, true);

    if (R_SUCCEEDED(rc)) rc = _webLaunch(&s->config->holder, s->config->appletid, mode, s->config->version, &s->config->arg, sizeof(s->config->arg));

    if (R_SUCCEEDED(rc) && out_event) *out_event = appletHolderGetExitEvent(&s->config->holder);

    if (R_FAILED(rc)) appletHolderClose(&s->config->holder);

    return rc;
}

Result webSessionWaitForExit(WebSession *s, WebCommonReply *out) {
    Result rc=0;
    void* reply = NULL;
    size_t size = 0;

    _webConfigInitReply(s->config, out, &reply, &size);

    rc = _webHandleExit(&s->config->holder, reply, size);
    appletHolderClose(&s->config->holder);
    _webSessionStorageHandleQueueClear(&s->storage_queue);
    if (R_SUCCEEDED(rc) && hosversionAtLeast(10,0,0) && out) {
        WebExitReason reason;
        rc = webReplyGetExitReason(out, &reason);
        if (R_SUCCEEDED(rc) && reason == WebExitReason_UnknownE) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    }
    return rc;
}

Result webSessionRequestExit(WebSession *s) {
    mutexLock(&s->mutex);
    Result rc = appletHolderRequestExit(&s->config->holder);
    mutexUnlock(&s->mutex);
    return rc;
}

static Result _webSessionReceiveStorageHandles(WebSession *s) {
    Result rc=0;
    AppletStorage storage;
    Event *event = NULL;
    WebSessionMessageHeader tmphdr={0};
    u8 data[0x20]={0};
    WebSessionMessageHeader *datahdr = (WebSessionMessageHeader*)data;

    rc = appletHolderGetPopInteractiveOutDataEvent(&s->config->holder, &event);
    if (R_FAILED(rc)) return rc;

    while (appletHolderActive(&s->config->holder)) {
        if (appletHolderCheckFinished(&s->config->holder)) return 0;
        if (R_FAILED(eventWait(event, 0))) return 0;
        if (R_FAILED(appletHolderPopInteractiveOutData(&s->config->holder, &storage))) return 0;

        rc = appletStorageRead(&storage, 0, &tmphdr, sizeof(tmphdr));

        if (R_SUCCEEDED(rc)) {
            if (tmphdr.kind == WebSessionReceiveMessageKind_AckBrowserEngine || tmphdr.kind == WebSessionReceiveMessageKind_AckSystemMessage) {
                u32 msgi = tmphdr.kind != WebSessionReceiveMessageKind_AckBrowserEngine;
                if (!s->queue[msgi].count) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
                if (R_SUCCEEDED(rc)) {
                    s->queue[msgi].count--;
                    rc = appletStorageRead(&storage, 0, data, sizeof(data));
                }
                if (R_SUCCEEDED(rc)) s->queue[msgi].cur_size -= *((u32*)(datahdr+1));
                appletStorageClose(&storage);
            }
            else _webSessionStorageEnqueue(&s->storage_queue, &storage);
        }
        if (R_FAILED(rc)) break;
    }

    return rc;
}

static Result _webSessionSendAck(WebSession *s, u32 size) {
    Result rc=0;
    AppletStorage storage;
    u8 data[0x20]={0};
    WebSessionMessageHeader *hdr = (WebSessionMessageHeader*)data;

    hdr->kind = WebSessionSendMessageKind_Ack;
    hdr->size = 0xC;
    *((u32*)(hdr+1)) = size;

    rc = libappletCreateWriteStorage(&storage, data, sizeof(data));
    if (R_SUCCEEDED(rc)) rc = appletHolderPushInteractiveInData(&s->config->holder, &storage);
    if (R_FAILED(rc)) appletStorageClose(&storage);
    return rc;
}

static bool _webSessionCanSend(WebSession *s, const WebSessionMessageHeader *hdr, u64 storage_size) {
    u32 msgi = hdr->kind!=WebSessionSendMessageKind_BrowserEngineContent;

    if (s->queue[msgi].count == 0x10) return false;
    if (s->queue[msgi].cur_size + storage_size > (msgi ? 0x1000 : 0x8000)) return false;
    return true;
}

static Result _webSessionTrySend(WebSession *s, const WebSessionMessageHeader *hdr, const void* content, bool *flag) {
    Result rc=0;
    AppletStorage storage;
    mutexLock(&s->mutex);
    if (appletHolderCheckFinished(&s->config->holder)) {
        *flag = false;
        mutexUnlock(&s->mutex);
        return 0;
    }
    rc = _webSessionReceiveStorageHandles(s);
    if (R_FAILED(rc)) {
        mutexUnlock(&s->mutex);
        return rc;
    }
    u32 msgi = hdr->kind!=WebSessionSendMessageKind_BrowserEngineContent;

    u64 storage_size = hdr->size+sizeof(*hdr);
    if (!_webSessionCanSend(s, hdr, storage_size)) {
        *flag = false;
        mutexUnlock(&s->mutex);
        return 0;
    }

    rc = appletCreateStorage(&storage, storage_size);
    if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&storage, 0, hdr, sizeof(*hdr));
    if (R_SUCCEEDED(rc) && content && hdr->size) rc = appletStorageWrite(&storage, sizeof(*hdr), content, hdr->size);
    if (R_FAILED(rc)) appletStorageClose(&storage);
    if (R_SUCCEEDED(rc)) rc = appletHolderPushInteractiveInData(&s->config->holder, &storage);

    if (R_SUCCEEDED(rc)) {
        s->queue[msgi].count++;
        s->queue[msgi].cur_size+= storage_size;
        *flag = true;
    }

    mutexUnlock(&s->mutex);
    return rc;
}

static Result _webSessionTryReceive(WebSession *s, WebSessionMessageHeader *hdr, void* content, u64 size, WebSessionReceiveMessageKind kind, bool *flag) {
    Result rc=0;
    s64 tmpsize=0;
    WebSessionMessageHeader tmphdr={0};
    AppletStorage *storage = NULL;

    if (hdr==NULL || content==NULL) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    mutexLock(&s->mutex);
    rc = _webSessionReceiveStorageHandles(s);
    if (R_FAILED(rc)) {
        mutexUnlock(&s->mutex);
        return rc;
    }

    while(1) {
        if (_webSessionStorageHandleQueueGetCount(&s->storage_queue) < 1) {
            *flag = false;
            mutexUnlock(&s->mutex);
            return 0;
        }

        storage = _webSessionStorageDequeue(&s->storage_queue);
        rc = appletStorageRead(storage, 0, &tmphdr, sizeof(tmphdr));

        if (R_SUCCEEDED(rc)) {
            rc = appletStorageGetSize(storage, &tmpsize);
            if (R_SUCCEEDED(rc) && tmphdr.size+sizeof(tmphdr) != tmpsize) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
        }

        if (R_SUCCEEDED(rc)) {
            if (tmphdr.kind == kind) break;
            else rc = _webSessionSendAck(s, tmpsize);
        }
        if (R_FAILED(rc)) break;
        appletStorageClose(storage);
    }

    if (R_SUCCEEDED(rc)) {
        *hdr = tmphdr;
        if (tmphdr.size) rc = appletStorageRead(storage, sizeof(tmphdr), content, tmphdr.size < size ? tmphdr.size : size);
    }

    if (R_SUCCEEDED(rc)) rc = _webSessionSendAck(s, tmpsize);
    appletStorageClose(storage);
    if (R_SUCCEEDED(rc)) *flag = true;
    mutexUnlock(&s->mutex);
    return rc;
}

Result webSessionAppear(WebSession *s, bool *flag) {
    Result rc=0;
    bool tmpflag=0;
    WebSessionMessageHeader hdr = {.kind = WebSessionSendMessageKind_SystemMessageAppear, .size = 0};
    do {
        rc = _webSessionTrySend(s, &hdr, NULL, &tmpflag);
        if (R_SUCCEEDED(rc) && tmpflag) {
            *flag = true;
            return rc;
        }
        if (R_FAILED(rc)) return rc;
    } while (appletHolderWaitInteractiveOut(&s->config->holder));
    *flag = false;
    return 0;
}

Result webSessionTrySendContentMessage(WebSession *s, const char *content, u32 size, bool *flag) {
    WebSessionMessageHeader hdr = {.kind = WebSessionSendMessageKind_BrowserEngineContent, .size = size};
    return _webSessionTrySend(s, &hdr, content, flag);
}

Result webSessionTryReceiveContentMessage(WebSession *s, char *content, u64 size, u64 *out_size, bool *flag) {
    Result rc=0;
    bool tmpflag=0;
    WebSessionMessageHeader hdr={0};
    if (content==NULL || out_size==NULL) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    do {
        rc = _webSessionTryReceive(s, &hdr, content, size, WebSessionReceiveMessageKind_BrowserEngineContent, &tmpflag);
        if (R_FAILED(rc) || !tmpflag) {
            *flag = false;
            return rc;
        }
    } while(hdr.kind!=WebSessionReceiveMessageKind_BrowserEngineContent);

    u64 tmp_size = hdr.size;
    *out_size = tmp_size;
    if (tmp_size > size) tmp_size = size;
    if (tmp_size) content[tmp_size-1] = 0; // sdknso doesn't check tmp_size here.

    *flag = true;
    return rc;
}

