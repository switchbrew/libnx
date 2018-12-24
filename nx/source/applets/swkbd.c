#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "kernel/detect.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/swkbd.h"
#include "runtime/util/utf.h"

//TODO: InlineKeyboard currently isn't supported.

static void _swkbdConvertToUTF8(char* out, const u16* in, size_t max) {
    if (out==NULL || in==NULL) return;
    out[0] = 0;

    ssize_t units = utf16_to_utf8((uint8_t*)out, in, max);
    if (units < 0) return;
    out[units] = 0;
}

static ssize_t _swkbdConvertToUTF16(u16* out, const char* in, size_t max) {
    if (out==NULL || in==NULL) return 0;
    out[0] = 0;

    ssize_t units = utf8_to_utf16(out, (uint8_t*)in, max);
    if (units < 0 || max<=1) return 0;
    out[units] = 0;

    return units;
}

static ssize_t _swkbdConvertToUTF16ByteSize(u16* out, const char* in, size_t max) {
    return _swkbdConvertToUTF16(out, in, (max/sizeof(u16)) - 1);
}

static void _swkbdConfigClear(SwkbdConfig* c) {
    memset(&c->arg.arg, 0, sizeof(c->arg.arg));
    memset(c->arg.unk_x3e0, 0xff, sizeof(c->arg.unk_x3e0));
}

Result swkbdCreate(SwkbdConfig* c, s32 max_dictwords) {
    Result rc=0;

    memset(c, 0, sizeof(SwkbdConfig));
    _swkbdConfigClear(c);

    c->version=0x5;//1.0.0+ version
    if (kernelAbove500()) {
        c->version = 0x50009;
    }
    else if (kernelAbove400()) {
        c->version = 0x40008;
    }
    else if (kernelAbove300()) {
        c->version = 0x30007;
    }
    else if (kernelAbove200()) {
        c->version = 0x10006;
    }

    c->workbuf_size = 0x1000;
    if (max_dictwords > 0 && max_dictwords <= 0x3e8) c->max_dictwords = max_dictwords;

    if (c->max_dictwords) {
        c->workbuf_size = c->max_dictwords*0x64 + 0x7e8;
        c->workbuf_size = (c->workbuf_size + 0xfff) & ~0xfff;
    }

    c->workbuf = (u8*)memalign(0x1000, c->workbuf_size);
    if (c->workbuf==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    if (R_SUCCEEDED(rc)) memset(c->workbuf, 0, c->workbuf_size);

    return rc;
}

void swkbdClose(SwkbdConfig* c) {
    free(c->workbuf);
    memset(c, 0, sizeof(SwkbdConfig));
}

void swkbdConfigMakePresetDefault(SwkbdConfig* c) {
    _swkbdConfigClear(c);

    c->arg.arg.type = SwkbdType_QWERTY;
    c->arg.arg.initialCursorPos = 1;
    if (c->version < 0x50009) c->arg.arg.textDrawType = SwkbdTextDrawType_Box;//removed with 5.x
    c->arg.arg.returnButtonFlag = 1;
    c->arg.arg.blurBackground = 1;
}

void swkbdConfigMakePresetPassword(SwkbdConfig* c) {
    _swkbdConfigClear(c);

    c->arg.arg.type = SwkbdType_QWERTY;
    c->arg.arg.initialCursorPos = 1;
    c->arg.arg.passwordFlag = 1;
    c->arg.arg.blurBackground = 1;
}

void swkbdConfigMakePresetUserName(SwkbdConfig* c) {
    _swkbdConfigClear(c);

    c->arg.arg.type = SwkbdType_Normal;
    c->arg.arg.keySetDisableBitmask = SwkbdKeyDisableBitmask_UserName;
    c->arg.arg.initialCursorPos = 1;
    c->arg.arg.blurBackground = 1;
}

void swkbdConfigMakePresetDownloadCode(SwkbdConfig* c) {
    _swkbdConfigClear(c);

    c->arg.arg.type = SwkbdType_Normal;
    c->arg.arg.keySetDisableBitmask = SwkbdKeyDisableBitmask_DownloadCode;
    c->arg.arg.initialCursorPos = 1;

    if (c->version >= 0x50009) {//5.x
        c->arg.arg.type = SwkbdType_QWERTY;

        c->arg.arg.stringLenMax = 16;
        c->arg.arg.stringLenMaxExt = 1;
        c->arg.arg.textDrawType = SwkbdTextDrawType_DownloadCode;
    }

    c->arg.arg.blurBackground = 1;

    if (c->version >= 0x50009) {//5.x
        c->arg.unk_x3e0[0] = 0x3;
        c->arg.unk_x3e0[1] = 0x7;
        c->arg.unk_x3e0[2] = 0xb;
    }
}

void swkbdConfigSetOkButtonText(SwkbdConfig* c, const char* str) {
    _swkbdConvertToUTF16ByteSize(c->arg.arg.okButtonText, str, sizeof(c->arg.arg.okButtonText));
}

void swkbdConfigSetLeftOptionalSymbolKey(SwkbdConfig* c, const char* str) {
    _swkbdConvertToUTF16(&c->arg.arg.leftButtonText, str, 1);
}

void swkbdConfigSetRightOptionalSymbolKey(SwkbdConfig* c, const char* str) {
    _swkbdConvertToUTF16(&c->arg.arg.rightButtonText, str, 1);
}

void swkbdConfigSetHeaderText(SwkbdConfig* c, const char* str) {
    _swkbdConvertToUTF16ByteSize(c->arg.arg.headerText, str, sizeof(c->arg.arg.headerText));
}

void swkbdConfigSetSubText(SwkbdConfig* c, const char* str) {
    _swkbdConvertToUTF16ByteSize(c->arg.arg.subText, str, sizeof(c->arg.arg.subText));
}

void swkbdConfigSetGuideText(SwkbdConfig* c, const char* str) {
    _swkbdConvertToUTF16ByteSize(c->arg.arg.guideText, str, sizeof(c->arg.arg.guideText));
}

void swkbdConfigSetInitialText(SwkbdConfig* c, const char* str) {
    c->arg.arg.initialStringOffset = 0;
    c->arg.arg.initialStringSize = 0;

    if (c->workbuf==NULL) return;
    u32 offset=0x14;

    ssize_t units = _swkbdConvertToUTF16ByteSize((u16*)&c->workbuf[offset], str, 0x1f4);
    if (units<=0) return;

    c->arg.arg.initialStringOffset = offset;
    c->arg.arg.initialStringSize = units;
}

void swkbdConfigSetDictionary(SwkbdConfig* c, const SwkbdDictWord *buffer, s32 entries) {
    c->arg.arg.userDicOffset = 0;
    c->arg.arg.userDicEntries = 0;

    if (c->workbuf==NULL) return;
    if (entries < 1 || entries > c->max_dictwords) return;
    u32 offset=0x7e8;

    c->arg.arg.userDicOffset = offset;
    c->arg.arg.userDicEntries = entries;
    memcpy(&c->workbuf[offset], buffer, entries*0x64);
}

void swkbdConfigSetTextCheckCallback(SwkbdConfig* c, SwkbdTextCheckCb cb) {
    c->arg.arg.textCheckFlag = cb!=0;
    c->arg.arg.textCheckCb = cb;
}

static Result _swkbdProcessInteractive(SwkbdConfig* c, AppletHolder* h, uint16_t* strbuf, size_t strbuf_size, char* tmp_string, size_t tmp_string_size) {
    Result rc=0;
    AppletStorage storage;
    u64 strsize=0;
    u32 res=0;

    rc = appletHolderPopInteractiveOutData(h, &storage);
    if (R_FAILED(rc)) return rc;

    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&storage, 0, &strsize, sizeof(strsize));
    if (R_SUCCEEDED(rc) && strsize > strbuf_size) strsize = strbuf_size;
    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&storage, sizeof(strsize), strbuf, strsize);
    appletStorageClose(&storage);

    if (R_SUCCEEDED(rc) && (c->arg.arg.textCheckFlag && c->arg.arg.textCheckCb)) {
        _swkbdConvertToUTF8(tmp_string, strbuf, tmp_string_size-1);

        res = c->arg.arg.textCheckCb(tmp_string, tmp_string_size-1);

        _swkbdConvertToUTF16ByteSize(strbuf, tmp_string, strbuf_size-2);
    }

    if (R_SUCCEEDED(rc)) rc = appletCreateStorage(&storage, sizeof(res)+strbuf_size);
    if (R_SUCCEEDED(rc)) {
        rc = appletStorageWrite(&storage, 0, &res, sizeof(res));
        if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&storage, sizeof(res), strbuf, strbuf_size);

        if (R_FAILED(rc)) appletStorageClose(&storage);
        if (R_SUCCEEDED(rc)) rc = appletHolderPushInteractiveInData(h, &storage);
    }

    return rc;
}

static Result _swkbdProcessOutput(AppletHolder* h, uint16_t* strbuf, size_t strbuf_size, char* out_string, size_t out_string_size) {
    Result rc=0;
    AppletStorage outstorage;
    u32 CloseResult=0;

    rc = appletHolderPopOutData(h, &outstorage);
    if (R_FAILED(rc)) return rc;

    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&outstorage, 0, &CloseResult, sizeof(CloseResult));
    if (R_SUCCEEDED(rc) && CloseResult!=0) rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&outstorage, sizeof(CloseResult), strbuf, strbuf_size);

    if (R_SUCCEEDED(rc)) _swkbdConvertToUTF8(out_string, strbuf, out_string_size-1);

    appletStorageClose(&outstorage);

    return rc;
}

Result swkbdShow(SwkbdConfig* c, char* out_string, size_t out_string_size) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage;
    uint16_t* strbuf = NULL;
    size_t strbuf_size = 0x7D4;

    if (out_string==NULL || out_string_size==0) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(&storage, 0, sizeof(AppletStorage));

    strbuf = (u16*)malloc(strbuf_size+2);
    if (strbuf==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    if (strbuf) memset(strbuf, 0, strbuf_size+2);
    if (R_FAILED(rc)) return rc;

    rc = appletCreateLibraryApplet(&holder, AppletId_swkbd, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) {
        free(strbuf);
        return rc;
    }

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, c->version);
    rc = libappletArgsPush(&commonargs, &holder);

    if (R_SUCCEEDED(rc)) {
        //3.0.0+ has a larger struct.
        if (c->version < 0x30007) rc = libappletPushInData(&holder, &c->arg.arg, sizeof(c->arg.arg));
        if (c->version >= 0x30007) rc = libappletPushInData(&holder, &c->arg, sizeof(c->arg));
    }

    if (R_SUCCEEDED(rc)) {
        if (R_SUCCEEDED(rc)) rc = appletCreateTransferMemoryStorage(&storage, c->workbuf, c->workbuf_size, true);
        appletHolderPushInData(&holder, &storage);
    }

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);

    if (R_SUCCEEDED(rc)) {
        while(appletHolderWaitInteractiveOut(&holder)) {
            _swkbdProcessInteractive(c, &holder, strbuf, strbuf_size, out_string, out_string_size);
        }
    }

    if (R_SUCCEEDED(rc)) {
        appletHolderJoin(&holder);

        LibAppletExitReason reason = appletHolderGetExitReason(&holder);

        if (reason == LibAppletExitReason_Canceled) {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }
        else if (reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }
        else { //success
            memset(out_string, 0, out_string_size);
            rc = _swkbdProcessOutput(&holder, strbuf, strbuf_size, out_string, out_string_size);
        }
    }

    appletHolderClose(&holder);
    appletStorageCloseTmem(&storage);

    free(strbuf);

    return rc;
}

