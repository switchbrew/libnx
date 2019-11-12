#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/capssu.h"

static Service g_capssuSrv;

static Result _capssuSetShimLibraryVersion(u64 version);

NX_GENERATE_SERVICE_GUARD(capssu);

Result _capssuInitialize(void) {
    Result rc=0;

    if (hosversionBefore(4,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capssuSrv, "caps:su");

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _capssuSetShimLibraryVersion(capsGetShimLibraryVersion());

    return rc;
}

void _capssuCleanup(void) {
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

    const struct {
        u64 version;
        u64 AppletResourceUserId;
    } in = { version, AppletResourceUserId };

    return serviceDispatchIn(&g_capssuSrv, 32, in,
        .in_send_pid = true,
    );
}

static Result _capssuSaveScreenShotEx0(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationAlbumEntry *out) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        CapsScreenShotAttribute attr;
        u32 reportoption;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { *attr, reportoption, 0, AppletResourceUserId };

    return serviceDispatchInOut(&g_capssuSrv, 203, in, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
        .in_send_pid = true,
    );
}

static Result _capssuSaveScreenShotEx(u32 cmd_id, bool pid, const void* argbuf, size_t argbuf_size, const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationAlbumEntry *out) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        CapsScreenShotAttribute attr;
        u32 reportoption;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { *attr, reportoption, 0, AppletResourceUserId };

    return serviceDispatchInOut(&g_capssuSrv, cmd_id, in, *out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { argbuf, argbuf_size },
            { buffer, size },
        },
        .in_send_pid = pid,
    );
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

Result capssuSaveScreenShotWithUserIds(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, const AccountUid* uids, size_t uid_count, CapsApplicationAlbumEntry *out) {
    CapsScreenShotAttribute attr;
    CapsUserIdList list;

    if (uid_count > ACC_USER_LIST_SIZE)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memset(&attr, 0, sizeof(attr));
    attr.orientation = orientation;
    attr.unk_xc = 1;

    memset(&list, 0, sizeof(list));
    if (uids && uid_count) memcpy(list.uids, uids, uid_count*sizeof(AccountUid));
    list.count = uid_count;

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

