#include <string.h>
#include <malloc.h>
#include <math.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/swkbd.h"
#include "runtime/hosversion.h"
#include "runtime/util/utf.h"

static Result _swkbdGetReplies(SwkbdInline* s);

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

/// Clamp a float to the range 0.0f..1.0.f.
static void _swkbdClampFloat(float *val) {
    float tmpval = *val;

    tmpval = fminf(tmpval, 1.0f);
    tmpval = fmaxf(tmpval, 0.0f);

    *val = tmpval;
}

static void _swkbdConfigClear(SwkbdConfig* c) {
    memset(&c->arg.arg, 0, sizeof(c->arg.arg));
    memset(c->arg.unk_x3e0, 0xff, sizeof(c->arg.unk_x3e0));
}

static void _swkbdInitVersion(u32* version) {
    u32 hosver = hosversionGet();
    if (hosver >= MAKEHOSVERSION(5,0,0))
        *version = 0x50009;
    else if (hosver >= MAKEHOSVERSION(4,0,0))
        *version = 0x40008;
    else if (hosver >= MAKEHOSVERSION(3,0,0))
        *version = 0x30007;
    else if (hosver >= MAKEHOSVERSION(2,0,0))
        *version = 0x10006;
    else
        *version=0x5;//1.0.0+ version
}

Result swkbdCreate(SwkbdConfig* c, s32 max_dictwords) {
    Result rc=0;

    memset(c, 0, sizeof(SwkbdConfig));
    _swkbdConfigClear(c);

    _swkbdInitVersion(&c->version);

    c->workbuf_size = 0x1000;
    if (max_dictwords > 0 && max_dictwords <= 0x3e8) c->max_dictwords = max_dictwords;

    if (c->max_dictwords) {
        c->workbuf_size = c->max_dictwords*sizeof(SwkbdDictWord) + 0x7e8;
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
    memcpy(&c->workbuf[offset], buffer, entries*sizeof(SwkbdDictWord));
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
        if (R_SUCCEEDED(rc)) rc = appletHolderPushInData(&holder, &storage);
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

static Result _swkbdSendRequest(SwkbdInline* s, u32 RequestCommand, const void* buffer, size_t size) {
    Result rc=0;
    AppletStorage storage;

    rc = appletCreateStorage(&storage, size+sizeof(u32));
    if (R_FAILED(rc)) return rc;

    rc = appletStorageWrite(&storage, 0, &RequestCommand, sizeof(RequestCommand));
    if (R_SUCCEEDED(rc) && buffer!=NULL) rc = appletStorageWrite(&storage, sizeof(RequestCommand), buffer, size);
    if (R_FAILED(rc)) {
        appletStorageClose(&storage);
        return rc;
    }

    return appletHolderPushInteractiveInData(&s->holder, &storage);
}

Result swkbdInlineCreate(SwkbdInline* s) {
    Result rc=0;

    memset(s, 0, sizeof(SwkbdInline));

    _swkbdInitVersion(&s->version);

    //swkbd-inline is only available on 2.0.0+.
    if (s->version < 0x10006) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    s->calcArg.unk_x0 = 0x30000;
    s->calcArg.size = sizeof(s->calcArg);

    //if (s->version >= 0x50009) s->calcArg.initArg.unk_x5 = 0x1;//Set in a separate init func by official sw on 5.0.0+.

    s->calcArg.volume = 1.0f;
    s->calcArg.appearArg.type = SwkbdType_QWERTY;
    s->calcArg.unk_x6 = 1;
    s->calcArg.unk_x7 = 1;
    s->calcArg.appearArg.unk_x20 = -1;
    s->calcArg.appearArg.unk_x24 = -1;
    s->calcArg.appearArg.unk_x30 = 1;

    s->calcArg.enableBackspace = 1;
    s->calcArg.unk_x45f[0] = 1;
    s->calcArg.footerScalable = 1;
    s->calcArg.inputModeFadeType = 1;

    s->calcArg.keytopScaleX = 1.0f;
    s->calcArg.keytopScaleY = 1.0f;
    s->calcArg.keytopBgAlpha = 1.0f;
    s->calcArg.footerBgAlpha = 1.0f;
    s->calcArg.balloonScale = 1.0f;
    s->calcArg.unk_x48c = 1.0f;

    swkbdInlineSetUtf8Mode(s, true);

    s->interactive_tmpbuf_size = 0x1000;
    s->interactive_tmpbuf = (u8*)malloc(s->interactive_tmpbuf_size);
    if (s->interactive_tmpbuf==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    if (R_SUCCEEDED(rc)) memset(s->interactive_tmpbuf, 0, s->interactive_tmpbuf_size);

    if (R_SUCCEEDED(rc)) {
        s->interactive_strbuf_size = 0x1001;
        s->interactive_strbuf = (char*)malloc(s->interactive_strbuf_size);
        if (s->interactive_strbuf==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        if (R_SUCCEEDED(rc)) memset(s->interactive_strbuf, 0, s->interactive_strbuf_size);

        if (R_FAILED(rc)) {
            free(s->interactive_tmpbuf);
            s->interactive_tmpbuf = NULL;
        }
    }

    if (R_FAILED(rc)) {
        s->interactive_tmpbuf_size = 0;
        s->interactive_strbuf_size = 0;
    }

    return rc;
}

Result swkbdInlineClose(SwkbdInline* s) {
    Result rc=0;

    if (appletHolderActive(&s->holder))
    {
        _swkbdSendRequest(s, SwkbdRequestCommand_Finalize, NULL, 0);//Finalize cmd

        int cnt=0;
        while (s->dicCustomInitialized && cnt<9) {
            rc = _swkbdGetReplies(s);
            if (R_FAILED(rc)) break;

            if (s->dicCustomInitialized) {
                cnt++;
                svcSleepThread(100000000ULL);
            }
        }

        appletHolderJoin(&s->holder);

        if (R_SUCCEEDED(rc)) {
            LibAppletExitReason reason = appletHolderGetExitReason(&s->holder);

            if (reason == LibAppletExitReason_Canceled) {
                rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
            }
            else if (reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
                rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
            }
        }

        appletHolderClose(&s->holder);
    }

    free(s->interactive_tmpbuf);
    s->interactive_tmpbuf = NULL;
    s->interactive_tmpbuf_size = 0;
    free(s->interactive_strbuf);
    s->interactive_strbuf = NULL;
    s->interactive_strbuf_size = 0;

    if (s->dicCustomInitialized) appletStorageCloseTmem(&s->dicStorage);
    if (s->wordInfoInitialized) appletStorageCloseTmem(&s->wordInfoStorage);

    memset(s, 0, sizeof(SwkbdInline));

    return rc;
}

Result swkbdInlineLaunch(SwkbdInline* s) {
    Result rc=0;

    rc = appletCreateLibraryApplet(&s->holder, AppletId_swkbd, s->calcArg.initArg.mode!=SwkbdInlineMode_UserDisplay ? LibAppletMode_Background : LibAppletMode_Unknown3);
    if (R_FAILED(rc)) return rc;

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, s->version);
    rc = libappletArgsPush(&commonargs, &s->holder);

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(&s->holder, &s->calcArg.initArg, sizeof(s->calcArg.initArg));

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&s->holder);

    return rc;
}

static void _swkbdProcessReply(SwkbdInline* s, SwkbdReplyType ReplyType, size_t size) {
    size_t stringendoff_utf8 = 0x7D4;
    size_t stringendoff_utf16 = 0x3EC;
    void* argdataend_utf8 = &s->interactive_tmpbuf[stringendoff_utf8];
    void* argdataend_utf16 = &s->interactive_tmpbuf[stringendoff_utf16];
    char* strdata = (char*)s->interactive_tmpbuf;

    memset(s->interactive_strbuf, 0, s->interactive_strbuf_size);

    if ((ReplyType==SwkbdReplyType_ChangedString && size != 0x3FC) || (ReplyType==SwkbdReplyType_ChangedStringUtf8 && size != 0x7E4)) return;
    if ((ReplyType==SwkbdReplyType_MovedCursor && size != 0x3F4) || (ReplyType==SwkbdReplyType_MovedCursorUtf8 && size != 0x7DC)) return;
    if ((ReplyType==SwkbdReplyType_DecidedEnter && size != 0x3F0) || (ReplyType==SwkbdReplyType_DecidedEnterUtf8 && size != 0x7D8)) return;
    if (ReplyType==SwkbdReplyType_MovedTab && size != 0x3F4) return;

    if (ReplyType==SwkbdReplyType_ChangedString || ReplyType==SwkbdReplyType_MovedCursor || ReplyType==SwkbdReplyType_MovedTab || ReplyType==SwkbdReplyType_DecidedEnter) {
        _swkbdConvertToUTF8(s->interactive_strbuf, (u16*)strdata, s->interactive_strbuf_size-1);
        strdata = s->interactive_strbuf;
    }

    switch(ReplyType) {
        case SwkbdReplyType_FinishedInitialize:
            if (s->finishedInitializeCb) s->finishedInitializeCb();
        break;

        case SwkbdReplyType_ChangedString:
        case SwkbdReplyType_ChangedStringUtf8:
            if (s->changedStringCb) {
                if (ReplyType==SwkbdReplyType_ChangedString) s->changedStringCb(strdata, (SwkbdChangedStringArg*)argdataend_utf16);
                if (ReplyType==SwkbdReplyType_ChangedStringUtf8) s->changedStringCb(strdata, (SwkbdChangedStringArg*)argdataend_utf8);
            }
        break;

        case SwkbdReplyType_MovedCursor:
        case SwkbdReplyType_MovedCursorUtf8:
            if (s->movedCursorCb) {
                if (ReplyType==SwkbdReplyType_MovedCursor) s->movedCursorCb(strdata, (SwkbdMovedCursorArg*)argdataend_utf16);
                if (ReplyType==SwkbdReplyType_MovedCursorUtf8) s->movedCursorCb(strdata, (SwkbdMovedCursorArg*)argdataend_utf8);
            }
        break;

        case SwkbdReplyType_MovedTab:
            if (s->movedTabCb) s->movedTabCb(strdata, (SwkbdMovedTabArg*)argdataend_utf16);
        break;

        case SwkbdReplyType_DecidedEnter:
        case SwkbdReplyType_DecidedEnterUtf8:
            if (s->decidedEnterCb) {
                if (ReplyType==SwkbdReplyType_DecidedEnter) s->decidedEnterCb(strdata, (SwkbdDecidedEnterArg*)argdataend_utf16);
                if (ReplyType==SwkbdReplyType_DecidedEnterUtf8) s->decidedEnterCb(strdata, (SwkbdDecidedEnterArg*)argdataend_utf8);
            }
        break;

        case SwkbdReplyType_UnsetCustomizeDic:
            if (s->dicCustomInitialized) {
                appletStorageCloseTmem(&s->dicStorage);
                s->dicCustomInitialized = false;
            }
        break;

        case SwkbdReplyType_ReleasedUserWordInfo:
            if (s->releasedUserWordInfoCb) s->releasedUserWordInfoCb();

            if (s->wordInfoInitialized) {
                appletStorageCloseTmem(&s->wordInfoStorage);
                s->wordInfoInitialized = false;
            }
        break;

        default:
        break;
    }
}

static Result _swkbdGetReplies(SwkbdInline* s) {
    Result rc=0;
    AppletStorage storage;
    SwkbdReplyType ReplyType=0;

    while(R_SUCCEEDED(appletHolderPopInteractiveOutData(&s->holder, &storage))) {
        s64 tmpsize=0;
        rc = appletStorageGetSize(&storage, &tmpsize);
        memset(s->interactive_tmpbuf, 0, s->interactive_tmpbuf_size);

        if (R_SUCCEEDED(rc) && (tmpsize < 8 || tmpsize-8 > s->interactive_tmpbuf_size)) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if (R_SUCCEEDED(rc)) rc = appletStorageRead(&storage, 0x0, &s->state, sizeof(s->state));
        if (R_SUCCEEDED(rc)) rc = appletStorageRead(&storage, 0x4, &ReplyType, sizeof(u32));
        if (R_SUCCEEDED(rc) && tmpsize >= 8) rc = appletStorageRead(&storage, 0x8, s->interactive_tmpbuf, tmpsize-8);

        appletStorageClose(&storage);

        if (R_FAILED(rc)) break;

        _swkbdProcessReply(s, ReplyType, tmpsize-8);
    }

    return rc;
}

Result swkbdInlineUpdate(SwkbdInline* s, SwkbdState* out_state) {
    Result rc=0;

    u8 fadetype=0;
    if (s->calcArg.footerScalable) {
        swkbdInlineSetFooterBgAlpha(s, s->calcArg.keytopBgAlpha);

        fadetype = s->calcArg.keytopBgAlpha != 1.0f;
    }
    else {
        fadetype = 2;
    }
    swkbdInlineSetInputModeFadeType(s, fadetype);

    if (appletHolderCheckFinished(&s->holder)) {
        appletHolderJoin(&s->holder);
        appletHolderClose(&s->holder);

        s->state = SwkbdState_Inactive;
        if (out_state) *out_state = s->state;
        return 0;
    }

    if (s->calcArg.flags) {
        rc = _swkbdSendRequest(s, SwkbdRequestCommand_Calc, &s->calcArg, sizeof(s->calcArg));
        s->calcArg.flags = 0;
        if (R_FAILED(rc)) return rc;
    }

    rc = _swkbdGetReplies(s);

    if (out_state) *out_state = s->state;

    return rc;
}

void swkbdInlineSetFinishedInitializeCallback(SwkbdInline* s, VoidFn cb) {
    s->finishedInitializeCb = cb;
}

void swkbdInlineSetChangedStringCallback(SwkbdInline* s, SwkbdChangedStringCb cb) {
    s->changedStringCb = cb;
}

void swkbdInlineSetMovedCursorCallback(SwkbdInline* s, SwkbdMovedCursorCb cb) {
    s->movedCursorCb = cb;
}

void swkbdInlineSetMovedTabCallback(SwkbdInline* s, SwkbdMovedTabCb cb) {
    s->movedTabCb = cb;
}

void swkbdInlineSetDecidedEnterCallback(SwkbdInline* s, SwkbdDecidedEnterCb cb) {
    s->decidedEnterCb = cb;
}

void swkbdInlineSetReleasedUserWordInfoCallback(SwkbdInline* s, VoidFn cb) {
    s->releasedUserWordInfoCb = cb;
}

static void _swkbdInlineUpdateAppearFlags(SwkbdInline* s) {
    u32 mask = 0x10000000;
    u32 tmp = s->calcArg.appearArg.flags;
    if (!s->directionalButtonAssignFlag) tmp &= ~mask;
    if (s->directionalButtonAssignFlag) tmp |= mask;
    s->calcArg.appearArg.flags = tmp;
}

void swkbdInlineAppear(SwkbdInline* s, SwkbdAppearArg* arg) {
    memcpy(&s->calcArg.appearArg, arg, sizeof(SwkbdAppearArg));
    _swkbdInlineUpdateAppearFlags(s);
    s->calcArg.flags = (s->calcArg.flags & ~0x80) | 0x4;
}

void swkbdInlineDisappear(SwkbdInline* s) {
    s->calcArg.flags = (s->calcArg.flags & ~0x4) | 0x80;
}

void swkbdInlineMakeAppearArg(SwkbdAppearArg* arg, SwkbdType type) {
    memset(arg, 0, sizeof(SwkbdAppearArg));

    arg->unk_x20 = -1;
    arg->unk_x24 = -1;
    arg->unk_x30 = 1;
    arg->type = type;
}

void swkbdInlineAppearArgSetOkButtonText(SwkbdAppearArg* arg,  const char* str) {
    _swkbdConvertToUTF16ByteSize(arg->okButtonText, str, sizeof(arg->okButtonText));
}

void swkbdInlineAppearArgSetLeftButtonText(SwkbdAppearArg* arg, const char* str) {
    _swkbdConvertToUTF16(&arg->leftButtonText, str, 1);
}

void swkbdInlineAppearArgSetRightButtonText(SwkbdAppearArg* arg, const char* str) {
    _swkbdConvertToUTF16(&arg->rightButtonText, str, 1);
}

void swkbdInlineSetVolume(SwkbdInline* s, float volume) {
    if (s->calcArg.volume == volume) return;
    s->calcArg.volume = volume;
    s->calcArg.flags |= 0x2;
}

void swkbdInlineSetInputText(SwkbdInline* s, const char* str) {
    _swkbdConvertToUTF16ByteSize(s->calcArg.inputText, str, sizeof(s->calcArg.inputText));
    s->calcArg.flags |= 0x8;
}

void swkbdInlineSetCursorPos(SwkbdInline* s, s32 pos) {
    s->calcArg.cursorPos = pos;
    s->calcArg.flags |= 0x10;
}

Result swkbdInlineSetUserWordInfo(SwkbdInline* s, const SwkbdDictWord *input, s32 entries) {
    Result rc=0;
    size_t size=0;

    if (s->state > SwkbdState_Initialized || s->wordInfoInitialized) return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);
    if (entries < 0 || entries > 0x3e8) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (input==NULL || entries==0) return swkbdInlineUnsetUserWordInfo(s);

    size = size*sizeof(SwkbdDictWord) + 0x8;
    size = (size + 0xfff) & ~0xfff;

    rc = appletCreateTransferMemoryStorage(&s->wordInfoStorage, NULL, size, true);
    if (R_FAILED(rc)) return rc;

    u32 req = SwkbdRequestCommand_SetUserWordInfo;
    rc = appletStorageWrite(&s->wordInfoStorage, 0x0, &req, sizeof(req));
    if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&s->wordInfoStorage, 0x4, &entries, sizeof(entries));
    if (R_SUCCEEDED(rc) && entries>0) rc = appletStorageWrite(&s->wordInfoStorage, 0x8, input, sizeof(SwkbdDictWord) * entries);

    if (R_SUCCEEDED(rc)) rc = appletHolderPushInteractiveInData(&s->holder, &s->wordInfoStorage);
    if (R_FAILED(rc)) appletStorageCloseTmem(&s->wordInfoStorage);

    if (R_SUCCEEDED(rc)) {
        s->wordInfoInitialized = true;
        s->calcArg.flags &= ~0x400;
    }

    return rc;
}

Result swkbdInlineUnsetUserWordInfo(SwkbdInline* s) {
    if (s->state > SwkbdState_Initialized) return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);
    if (!s->wordInfoInitialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    s->calcArg.flags |= 0x400;

    return 0;
}

static void _swkbdInlineSetBoolFlag(SwkbdInline* s, u8* arg, bool flag, u64 bitmask) {
    u8 tmp = flag!=0;
    if (*arg == tmp) return;
    *arg = tmp;
    s->calcArg.flags |= bitmask;
}

static void _swkbdInlineSetBoolDisableFlag(SwkbdInline* s, u8* arg, bool flag, u64 bitmask) {
    _swkbdInlineSetBoolFlag(s, arg, !flag, bitmask);
}

void swkbdInlineSetUtf8Mode(SwkbdInline* s, bool flag) {
    _swkbdInlineSetBoolFlag(s, &s->calcArg.utf8Mode, flag, 0x20);
}

Result swkbdInlineSetCustomizeDic(SwkbdInline* s, void* buffer, size_t size, SwkbdCustomizeDicInfo *info) {
    Result rc=0;

    if (s->state > SwkbdState_Initialized || s->dicCustomInitialized) return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    rc = appletCreateHandleStorageTmem(&s->dicStorage, buffer, size);
    if (R_FAILED(rc)) return rc;
    rc = appletHolderPushInteractiveInData(&s->holder, &s->dicStorage);
    if (R_FAILED(rc)) {
        appletStorageCloseTmem(&s->dicStorage);
        return rc;
    }

    s->dicCustomInitialized = true;

    rc = _swkbdSendRequest(s, SwkbdRequestCommand_SetCustomizeDic, info, sizeof(SwkbdCustomizeDicInfo));

    return rc;
}

void swkbdInlineUnsetCustomizeDic(SwkbdInline* s) {
    if (s->state > SwkbdState_Initialized || !s->dicCustomInitialized) return;
    s->calcArg.flags |= 0x40;
}

void swkbdInlineSetInputModeFadeType(SwkbdInline* s, u8 type) {
    if (s->calcArg.inputModeFadeType == type) return;
    s->calcArg.inputModeFadeType = type;
    s->calcArg.flags |= 0x100;
}

void swkbdInlineSetAlphaEnabledInInputMode(SwkbdInline* s, bool flag) {
    _swkbdInlineSetBoolFlag(s, &s->calcArg.alphaEnabledInInputMode, flag, 0x100);
}

void swkbdInlineSetKeytopBgAlpha(SwkbdInline* s, float alpha) {
    _swkbdClampFloat(&alpha);
    if (s->calcArg.keytopBgAlpha == alpha) return;
    s->calcArg.keytopBgAlpha = alpha;
    s->calcArg.flags |= 0x100;
}

void swkbdInlineSetFooterBgAlpha(SwkbdInline* s, float alpha) {
    _swkbdClampFloat(&alpha);
    if (s->calcArg.footerBgAlpha == alpha) return;
    s->calcArg.footerBgAlpha = alpha;
    s->calcArg.flags |= 0x100;
}

void swkbdInlineSetKeytopAsFloating(SwkbdInline* s, bool flag) {
    _swkbdInlineSetBoolFlag(s, &s->calcArg.keytopAsFloating, flag, 0x200);
}

void swkbdInlineSetFooterScalable(SwkbdInline* s, bool flag) {
    _swkbdInlineSetBoolFlag(s, &s->calcArg.footerScalable, flag, 0x200);
}

void swkbdInlineSetTouchFlag(SwkbdInline* s, bool flag) {
    _swkbdInlineSetBoolDisableFlag(s, &s->calcArg.disableTouch, flag, 0x200);
}

static void _swkbdInlineSetKeytopScale(SwkbdInline* s, float x, float y) {
    if (s->calcArg.keytopScaleX == x && s->calcArg.keytopScaleY == y) return;
    s->calcArg.keytopScaleX = x;
    s->calcArg.keytopScaleY = y;
    s->calcArg.flags |= 0x200;
}

static void _swkbdInlineSetBalloonScale(SwkbdInline* s, float scale) {
    if (s->calcArg.balloonScale == scale) return;
    s->calcArg.balloonScale = scale;
    s->calcArg.flags |= 0x200;
}

void swkbdInlineSetKeytopScale(SwkbdInline* s, float scale) {
    _swkbdInlineSetKeytopScale(s, scale, scale);

    scale = fminf(scale + 0.15f, 1.0f);
    _swkbdInlineSetBalloonScale(s, scale);
}

void swkbdInlineSetKeytopTranslate(SwkbdInline* s, float x, float y) {
    if (s->calcArg.keytopTranslateX == x && s->calcArg.keytopTranslateY == y) return;
    s->calcArg.keytopTranslateX = x;
    s->calcArg.keytopTranslateY = y;
    s->calcArg.flags |= 0x200;
}

void swkbdInlineSetUSBKeyboardFlag(SwkbdInline* s, bool flag) {
    _swkbdInlineSetBoolDisableFlag(s, &s->calcArg.disableUSBKeyboard, flag, 0x800);
}

void swkbdInlineSetDirectionalButtonAssignFlag(SwkbdInline* s, bool flag) {
    s->directionalButtonAssignFlag = flag;
    _swkbdInlineUpdateAppearFlags(s);
    s->calcArg.flags |= 0x1000;
}

void swkbdInlineSetSeGroup(SwkbdInline* s, u8 seGroup, bool flag) {
    s->calcArg.seGroup = seGroup;
    s->calcArg.flags |= flag ? 0x2000 : 0x4000;
}

void swkbdInlineSetBackspaceFlag(SwkbdInline* s, bool flag) {
    s->calcArg.enableBackspace = flag!=0;
    s->calcArg.flags |= 0x8000;
}

