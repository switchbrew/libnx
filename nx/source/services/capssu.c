#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/caps.h"
#include "services/capssu.h"
#include "services/sm.h"

static Service g_capssuSrv;
static u64 g_capssuRefCnt;

Result capssuInitialize(void) {
    Result rc=0;

    atomicIncrement64(&g_capssuRefCnt);

    if (serviceIsActive(&g_capssuSrv))
        return 0;

    if (hosversionBefore(4,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capssuSrv, "caps:su");

    if (R_FAILED(rc)) capssuExit();

    return rc;
}

void capssuExit(void) {
    if (atomicDecrement64(&g_capssuRefCnt) == 0)
        serviceClose(&g_capssuSrv);
}

static Result _capssuSaveScreenShotEx0(const void* buffer, size_t size, CapsScreenShotAttribute *attr, u32 unk, CapsApplicationAlbumEntry *out) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsScreenShotAttribute attr;
        u32 unk;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddSendBuffer(&c, buffer, size, 1);

    raw = serviceIpcPrepareHeader(&g_capssuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 203;
    raw->attr = *attr;
    raw->unk = unk;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capssuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            CapsApplicationAlbumEntry out;
        } *resp;

        serviceIpcParse(&g_capssuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result capssuSaveScreenShot(const void* buffer, size_t size, u32 unk, u32 attr_val, CapsApplicationAlbumEntry *out) {
    CapsScreenShotAttribute attr;

    memset(&attr, 0, sizeof(attr));
    attr.unk_x0 = attr_val;

    return _capssuSaveScreenShotEx0(buffer, size, &attr, unk, out);
}

Result capssuSaveScreenShotEx0(const void* buffer, size_t size, CapsScreenShotAttribute *attr, u32 unk, CapsApplicationAlbumEntry *out) {
    return _capssuSaveScreenShotEx0(buffer, size, attr, unk, out);
}

