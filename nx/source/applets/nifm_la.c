#include <string.h>
#include "libapplet_internal.h"
#include "applets/nifm_la.h"

static Result _nifmLaPrepare(NifmRequest* r, AppletHolder *holder) {
    Result rc=0;
    u32 applet_id=0;
    u32 mode=0;
    u32 out_size=0;
    u8 buf[0x1000];
    u8 *data_ptr = buf;

    memset(buf, 0, sizeof(buf));
    rc = nifmRequestGetAppletInfo(r, appletGetThemeColorType(), buf, sizeof(buf), &applet_id, &mode, &out_size);
    if (R_FAILED(rc)) return rc;

    rc = appletCreateLibraryApplet(holder, applet_id, mode);
    if (R_FAILED(rc)) return rc;

    while(out_size >= sizeof(s32)) {
        s32 storage_size = *((s32*)data_ptr);
        data_ptr+= sizeof(s32);
        out_size-= sizeof(s32);
        if (storage_size == -1) return 0;

        if (out_size <= storage_size) break;

        rc = libappletPushInData(holder, data_ptr, storage_size);
        if (R_FAILED(rc)) return rc;

        data_ptr+= storage_size;
        out_size-= storage_size;
    }
    return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
}

static Result _nifmLaFinish(AppletHolder *holder) {
    Result rc=0;
    AppletStorage storage;
    s64 storage_size=0;

    appletHolderJoin(holder);

    LibAppletExitReason reason = appletHolderGetExitReason(holder);

    if (reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
        rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
    }

    if (R_SUCCEEDED(rc)) rc = appletHolderPopOutData(holder, &storage);

    if (R_SUCCEEDED(rc)) rc = appletStorageGetSize(&storage, &storage_size);

    if (R_SUCCEEDED(rc)) {
        s32 ret=-1;
        if (storage_size == sizeof(ret)) {
            rc = appletStorageRead(&storage, 0, &ret, sizeof(ret));
            if (R_SUCCEEDED(rc)) rc = ret==0 ? MAKERESULT(110, 190) : MAKERESULT(110, 2900);
        }
        else if (storage_size >= sizeof(ret)) {
            storage_size-= sizeof(ret);
            Result tmprc=0;
            rc = appletStorageRead(&storage, 0, &ret, sizeof(ret));
            if (R_SUCCEEDED(rc)) rc = appletStorageRead(&storage, sizeof(ret), &tmprc, storage_size < sizeof(tmprc) ? storage_size : sizeof(tmprc));
            if (R_SUCCEEDED(rc)) {
                if ((!ret && storage_size != sizeof(tmprc)) || (ret && storage_size < sizeof(tmprc)))
                    rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
            }
            if (R_SUCCEEDED(rc)) rc = tmprc;
        }
        else
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        appletStorageClose(&storage);
    }

    return rc;
}

Result nifmLaHandleNetworkRequestResult(NifmRequest* r) {
    Result rc=0;
    AppletHolder holder={0};

    rc = nifmGetResult(r);
    if (R_SUCCEEDED(rc)) return rc;

    rc = _nifmLaPrepare(r, &holder);
    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);
    if (R_SUCCEEDED(rc)) rc = _nifmLaFinish(&holder);

    appletHolderClose(&holder);
    return rc;
}

