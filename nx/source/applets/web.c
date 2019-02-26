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

static void _webTLVWrite(WebCommonTLVStorage *storage, u16 type, const void* argdata, u16 argdata_size, u16 argdata_size_total) {
    size_t i, count, offset;
    u8 *dataptr = storage->data;
    WebArgHeader *hdr = (WebArgHeader*)dataptr;
    WebArgTLV *tlv;
    size_t size = sizeof(storage->data);

    offset = sizeof(WebArgHeader);
    if (size < offset) return;
    if (argdata_size > argdata_size_total) argdata_size = argdata_size_total;

    count = hdr->total_entries;
    tlv = (WebArgTLV*)&dataptr[offset];

    for (i=0; i<count; i++) {
        if (size < offset + sizeof(WebArgTLV)) return;

        tlv = (WebArgTLV*)&dataptr[offset];

        if (tlv->type == type) {
            if (tlv->size != argdata_size_total) return;
            break;
        }

        offset+= sizeof(WebArgTLV) + tlv->size;
        if (size < offset) return;
    }

    if (size < offset + sizeof(WebArgTLV) + argdata_size_total) return;

    tlv = (WebArgTLV*)&dataptr[offset];

    if (tlv->type != type) {
        if (hdr->total_entries == 0xFFFF) return;

        tlv->type = type;
        tlv->size = argdata_size_total;
        hdr->total_entries++;
    }

    offset+= sizeof(WebArgTLV);
    memcpy(&dataptr[offset], argdata, argdata_size);
    if (argdata_size_total != argdata_size) memset(&dataptr[offset+argdata_size], 0, argdata_size_total-argdata_size);
}

static void _webTLVSet(WebCommonTLVStorage *storage, u16 type, const void* argdata, u16 argdata_size) {
    _webTLVWrite(storage, type, argdata, argdata_size, argdata_size);
}

static void _webConfigSetU8(WebCommonConfig* config, u16 type, u8 arg) {
    _webTLVSet(&config->arg, type, &arg, sizeof(arg));
}

static void _webConfigSetFlag(WebCommonConfig* config, u16 type, bool arg) {
    _webConfigSetU8(config, type, arg!=0);
}

/*static void _webConfigSetU32(WebCommonConfig* config, u16 type, u32 arg) {
    _webTLVSet(&config->arg, type, &arg, sizeof(arg));
}*/

static void _webConfigSetString(WebCommonConfig* config, u16 type, const char* str, u16 argdata_size_total) {
    u16 arglen = strlen(str);
    if (arglen >= argdata_size_total) arglen = argdata_size_total-1; //The string must be NUL-terminated.

    _webTLVWrite(&config->arg, type, str, arglen, argdata_size_total);
}

static void _webConfigSetUrl(WebCommonConfig* config, const char* url) {
    _webConfigSetString(config, 0x1, url, 0xc00);
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

void webPageCreate(WebCommonConfig* config, const char* url) {
    _webArgInitialize(config, AppletId_web, WebShimKind_Web);

    _webConfigSetU8(config, 0xD, 1);
    if (config->version < 0x30000) _webConfigSetU8(config, 0x12, 1); // Removed from user-process init with [3.0.0+].

    _webConfigSetUrl(config, url);
}

void webConfigSetWhitelist(WebCommonConfig* config, const char* whitelist) {
    if (_webGetShimKind(config) != WebShimKind_Web) return;
    _webConfigSetString(config, 0xA, whitelist, 0x1000);
}

void webConfigSetDisplayUrlKind(WebCommonConfig* config, bool kind) {
    _webConfigSetFlag(config, 0x1F, kind);
}

Result webConfigShow(WebCommonConfig* config, WebCommonReturnValue *out) {
    return _webShow(config->appletid, config->version, &config->arg, sizeof(config->arg), out, sizeof(*out));
}

