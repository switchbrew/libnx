#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/web.h"

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

static void _webArgInitialize(WebCommonTLVStorage *storage, WebShimKind shimKind) {
    WebArgHeader *hdr = (WebArgHeader*)storage->data;

    memset(storage, 0, sizeof(*storage));
    hdr->shimKind = shimKind;
}

static void _webTLVWrite(WebCommonTLVStorage *storage, u16 type, const void* argdata, u16 argdata_size) {
    size_t i, count, offset;
    u8 *dataptr = storage->data;
    WebArgHeader *hdr = (WebArgHeader*)dataptr;
    WebArgTLV *tlv;
    size_t size = sizeof(storage->data);

    offset = sizeof(WebArgHeader);
    if (size < offset) return;

    count = hdr->total_entries;
    tlv = (WebArgTLV*)&dataptr[offset];

    for (i=0; i<count; i++) {
        if (size < offset + sizeof(WebArgTLV)) return;

        tlv = (WebArgTLV*)&dataptr[offset];

        if (tlv->type == type) {
            if (tlv->size != argdata_size) return;
            break;
        }

        offset+= sizeof(WebArgTLV) + tlv->size;
        if (size < offset) return;
    }

    if (size < offset + sizeof(WebArgTLV) + argdata_size) return;

    tlv = (WebArgTLV*)&dataptr[offset];

    if (tlv->type != type) {
        if (hdr->total_entries == 0xFFFF) return;

        tlv->type = type;
        tlv->size = argdata_size;
        hdr->total_entries++;
    }

    offset+= sizeof(WebArgTLV);
    memcpy(&dataptr[offset], argdata, argdata_size);
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

void webPageCreate(WebPageConfig* config, const char* url) {
    char tmpurl[0xc00];

    _webArgInitialize(&config->arg, WebShimKind_Web);

    u8 tmpval=1;
    _webTLVWrite(&config->arg, 0xD, &tmpval, sizeof(tmpval));
    _webTLVWrite(&config->arg, 0x12, &tmpval, sizeof(tmpval)); // Removed from user-process init with [3.0.0+].

    memset(tmpurl, 0, sizeof(tmpurl));
    strncpy(tmpurl, url, sizeof(tmpurl)-1);
    _webTLVWrite(&config->arg, 0x1, tmpurl, sizeof(tmpurl));
}

Result webPageShow(WebPageConfig* config, WebCommonReturnValue *out) {
    return _webShow(AppletId_web, 0x20000, &config->arg, sizeof(config->arg), out, sizeof(*out));
}

