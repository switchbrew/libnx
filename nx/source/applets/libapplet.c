#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/counter.h"
#include "services/applet.h"
#include "applets/libapplet.h"

void libappletArgsCreate(LibAppletArgs* a, u32 version) {
    memset(a, 0, sizeof(LibAppletArgs));

    a->CommonArgs_version = 1;
    a->CommonArgs_size = sizeof(LibAppletArgs);

    a->LaVersion = version;
    a->ExpectedThemeColor = appletGetThemeColorType();
}

void libappletArgsSetPlayStartupSound(LibAppletArgs* a, bool flag) {
    a->PlayStartupSound = flag!=0;
}

Result libappletCreateWriteStorage(AppletStorage* s, const void* buffer, size_t size) {
    Result rc=0;

    rc = appletCreateStorage(s, size);
    if (R_FAILED(rc)) return rc;

    rc = appletStorageWrite(s, 0, buffer, size);
    if (R_FAILED(rc)) {
        appletStorageClose(s);
        return rc;
    }

    return rc;
}

Result libappletReadStorage(AppletStorage* s, void* buffer, size_t size, size_t *transfer_size) {
    Result rc=0;
    s64 tmpsize=0;

    rc = appletStorageGetSize(s, &tmpsize);

    if (R_SUCCEEDED(rc)) {
        if (tmpsize < size) size = tmpsize;
        rc = appletStorageRead(s, 0, buffer, size);
    }

    if (R_SUCCEEDED(rc) && transfer_size) *transfer_size = size;

    return rc;
}

static Result _libappletPushInData(AppletHolder *h, const void* buffer, size_t size) {
    Result rc=0;
    AppletStorage storage;

    rc = libappletCreateWriteStorage(&storage, buffer, size);
    if (R_FAILED(rc)) return rc;

    return appletHolderPushInData(h, &storage);
}

Result libappletArgsPush(LibAppletArgs* a, AppletHolder *h) {
    //Official sw stores the header in LibAppletArgs seperately (first 8-bytes), but here we're including it with the LibAppletCommonArguments struct.
    //Official sw uses appletStorageWrite twice, for writing the header then the rest of the struct.

    a->tick = armGetSystemTick();

    return _libappletPushInData(h, a, sizeof(LibAppletArgs));
}

static Result _libappletQlaunchRequest(u8* buf, size_t size) {
    Result rc=0;
    AppletStorage storage;

    rc = libappletCreateWriteStorage(&storage, buf, size);
    if (R_FAILED(rc)) return rc;

    return appletPushToGeneralChannel(&storage);
}

Result libappletPushInData(AppletHolder *h, const void* buffer, size_t size) {
    return _libappletPushInData(h, buffer, size);
}

Result libappletPopOutData(AppletHolder *h, void* buffer, size_t size, size_t *transfer_size) {
    Result rc=0;
    AppletStorage storage;

    rc = appletHolderPopOutData(h, &storage);
    if (R_FAILED(rc)) return rc;

    rc = libappletReadStorage(&storage, buffer, size, transfer_size);
    appletStorageClose(&storage);
    return rc;
}

Result libappletStart(AppletHolder *h) {
    Result rc=0;

    rc = appletHolderStart(h);

    if (R_SUCCEEDED(rc)) {
        appletHolderJoin(h);

        LibAppletExitReason reason = appletHolderGetExitReason(h);

        if (reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }
    }

    return rc;
}

Result libappletLaunch(AppletId id, LibAppletArgs *commonargs, const void* arg, size_t arg_size, void* reply, size_t reply_size, size_t *out_reply_size) {
    Result rc=0;
    AppletHolder holder;

    rc = appletCreateLibraryApplet(&holder, id, LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return rc;

    rc = libappletArgsPush(commonargs, &holder);

    if (R_SUCCEEDED(rc) && arg && arg_size) rc = libappletPushInData(&holder, arg, arg_size);

    if (R_SUCCEEDED(rc)) rc = libappletStart(&holder);

    if (R_SUCCEEDED(rc) && reply && reply_size) rc = libappletPopOutData(&holder, reply, reply_size, out_reply_size);

    appletHolderClose(&holder);

    return rc;
}

Result libappletRequestHomeMenu(void) {
    u8 storagedata[0x10] = {0x53, 0x41, 0x4d, 0x53, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};//RequestHomeMenu
    return _libappletQlaunchRequest(storagedata, sizeof(storagedata));
}

Result libappletRequestJumpToSystemUpdate(void) {
    u8 storagedata[0x10] = {0x53, 0x41, 0x4d, 0x53, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};//RequestJumpToSystemUpdate
    return _libappletQlaunchRequest(storagedata, sizeof(storagedata));
}

