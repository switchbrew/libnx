#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/swkbd.h"
#include "runtime/util/utf.h"

//TODO: InlineKeyboard currently isn't supported.

void swkbdCreate(SwkbdConfig* c) {
    memset(c, 0, sizeof(SwkbdConfig));
}

static void _swkbdConvertToUTF8(char* out, const u16* in, size_t max) {
    out[0] = 0;
    if (out==NULL || in==NULL) return;

    ssize_t units = utf16_to_utf8((uint8_t*)out, in, max);
    if (units < 0) return;
    out[units] = 0;
}

static Result _swkbdProcessOutput(AppletHolder* h, char* out_string, size_t out_string_size) {
    Result rc=0;
    AppletStorage outstorage;
    u32 CloseResult=0;
    uint16_t* strbuf = NULL;
    size_t strbuf_size = 0x7D4;

    rc = appletHolderPopOutData(h, &outstorage);
    if (R_FAILED(rc)) return rc;

    strbuf = (u16*)malloc(strbuf_size+2);
    if (strbuf==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    if (strbuf) memset(strbuf, 0, strbuf_size+2);

    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&outstorage, 0, &CloseResult, sizeof(CloseResult));
    if (R_SUCCEEDED(rc) && CloseResult!=0) rc = 0x29f;//TODO: See below.
    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&outstorage, sizeof(CloseResult), strbuf, strbuf_size);

    if (R_SUCCEEDED(rc)) _swkbdConvertToUTF8(out_string, strbuf, out_string_size-1);

    free(strbuf);
    appletStorageClose(&outstorage);

    return rc;
}

Result swkbdShow(SwkbdConfig* c, char* out_string, size_t out_string_size) {
    Result rc=0;
    AppletHolder holder;
    AppletStorage storage;
    u8 *workbuf = NULL;//TODO
    size_t workbuf_size = 0x1000;//TODO

    memset(&storage, 0, sizeof(AppletStorage));

    rc = appletCreateLibraryApplet(&holder, AppletId_swkbd, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    LibAppletArgs commonargs;
    libappletArgsCreate(&commonargs, 0x5);//1.0.0+ version
    rc = libappletArgsPush(&commonargs, &holder);

    if (R_SUCCEEDED(rc)) rc = libappletPushInData(&holder, &c->arg, sizeof(c->arg));

    if (R_SUCCEEDED(rc)) {
        if (R_SUCCEEDED(rc)) rc = appletCreateTransferMemoryStorage(&storage, workbuf, workbuf_size, true);
        appletHolderPushInData(&holder, &storage);
    }

    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);

    if (R_SUCCEEDED(rc)) {
        while(appletHolderWaitInteractiveOut(&holder)) {
            //TODO: Handle Interactive data here.
        }
    }

    if (R_SUCCEEDED(rc)) {
        appletHolderJoin(&holder);

        LibAppletExitReason reason = appletHolderGetExitReason(&holder);

        if (reason == LibAppletExitReason_Canceled) {
            rc = 0x29f;//TODO: Official sw returns this, replace it with something else.
        }
        else if (reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            //TODO: Official sw asserts here - return a proper error here.
            return -1;
        }
        else { //success
            rc = _swkbdProcessOutput(&holder, out_string, out_string_size);
        }
    }

    appletHolderClose(&holder);
    appletStorageCloseTmem(&storage);

    return rc;
}

