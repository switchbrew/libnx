#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/caps.h"
#include "services/capssu.h"
#include "services/acc.h"
#include "services/sm.h"

static Service g_capssuSrv;
static u64 g_capssuRefCnt;

static Result _capssuSetShimLibraryVersion(u64 version);

Result capssuInitialize(void) {
    Result rc=0;

    atomicIncrement64(&g_capssuRefCnt);

    if (serviceIsActive(&g_capssuSrv))
        return 0;

    if (hosversionBefore(4,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capssuSrv, "caps:su");

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _capssuSetShimLibraryVersion(capsGetShimLibraryVersion());

    if (R_FAILED(rc)) capssuExit();

    return rc;
}

void capssuExit(void) {
    if (atomicDecrement64(&g_capssuRefCnt) == 0)
        serviceClose(&g_capssuSrv);
}

Service* capssuGetServiceSession(void) {
    return &g_capssuSrv;
}

static Result _capssuSetShimLibraryVersion(u64 version) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 version;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(&g_capssuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 32;
    raw->version = version;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capssuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capssuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _capssuSaveScreenShotEx0(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationAlbumEntry *out) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsScreenShotAttribute attr;
        u32 reportoption;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddSendBuffer(&c, buffer, size, BufferType_Type1);

    raw = serviceIpcPrepareHeader(&g_capssuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 203;
    raw->attr = *attr;
    raw->reportoption = reportoption;
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

static Result _capssuSaveScreenShotEx(u64 cmd_id, bool pid, const void* argbuf, size_t argbuf_size, const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationAlbumEntry *out) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsScreenShotAttribute attr;
        u32 reportoption;
        u64 AppletResourceUserId;
    } *raw;

    if (pid) ipcSendPid(&c);
    ipcAddSendBuffer(&c, argbuf, argbuf_size, BufferType_Normal);
    ipcAddSendBuffer(&c, buffer, size, BufferType_Type1);

    raw = serviceIpcPrepareHeader(&g_capssuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->attr = *attr;
    raw->reportoption = reportoption;
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

Result capssuSaveScreenShot(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, CapsApplicationAlbumEntry *out) {
    CapsScreenShotAttribute attr;

    memset(&attr, 0, sizeof(attr));
    attr.orientation = orientation;
    attr.unk_xc = 1;

    return _capssuSaveScreenShotEx0(buffer, size, &attr, reportoption, out);
}

Result capssuSaveScreenShotWithUserData(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, const void* userdata, size_t userdata_size, CapsApplicationAlbumEntry *out) {
    CapsScreenShotAttribute attr;
    CapsApplicationData appdata;

    if (userdata_size > sizeof(appdata).userdata)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(&attr, 0, sizeof(attr));
    attr.orientation = orientation;
    attr.unk_xc = 1;

    memset(&appdata, 0, sizeof(appdata));
    if (userdata && userdata_size) memcpy(appdata.userdata, userdata, userdata_size);
    appdata.size = userdata_size;

    return capssuSaveScreenShotEx1(buffer, size, &attr, reportoption, &appdata, out);
}

Result capssuSaveScreenShotWithUserIds(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, const u128* userIDs, size_t userID_count, CapsApplicationAlbumEntry *out) {
    CapsScreenShotAttribute attr;
    CapsUserIdList list;

    if (userID_count > ACC_USER_LIST_SIZE)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(&attr, 0, sizeof(attr));
    attr.orientation = orientation;
    attr.unk_xc = 1;

    memset(&list, 0, sizeof(list));
    if (userIDs && userID_count) memcpy(list.userIDs, userIDs, userID_count*sizeof(u128));
    list.count = userID_count;

    return capssuSaveScreenShotEx2(buffer, size, &attr, reportoption, &list, out);
}

Result capssuSaveScreenShotEx0(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationAlbumEntry *out) {
    return _capssuSaveScreenShotEx0(buffer, size, attr, reportoption, out);
}

Result capssuSaveScreenShotEx1(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationData *appdata, CapsApplicationAlbumEntry *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _capssuSaveScreenShotEx(205, true, appdata, sizeof(*appdata), buffer, size, attr, reportoption, out);
}

Result capssuSaveScreenShotEx2(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsUserIdList *list, CapsApplicationAlbumEntry *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _capssuSaveScreenShotEx(210, false, list, sizeof(*list), buffer, size, attr, reportoption, out);
}

