#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <time.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/capsa.h"

static Service g_capsaSrv;
static Service g_capsaAccessor;

NX_GENERATE_SERVICE_GUARD(capsa);

Result _capsaInitialize(void) {
    return smGetService(&g_capsaSrv, "caps:a");
}

void _capsaCleanup(void) {
    serviceClose(&g_capsaAccessor);
    serviceClose(&g_capsaSrv);
}

Service* capsaGetServiceSession(void) {
    return &g_capsaSrv;
}

Service* capsaGetServiceSession_Accessor(void) {
    return &g_capsaAccessor;
}

static Result _capsaCmdInU8NoOut(Service* srv, u8 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

Result capsaGetAlbumFileCount(CapsAlbumStorage storage, u64* count) {
    u8 inval = storage;
    return serviceDispatchInOut(&g_capsaSrv, 0, inval, *count);
}

Result capsaGetAlbumFileList(CapsAlbumStorage storage, u64* count, CapsAlbumEntry* buffer, u64 buffer_size) {
    u8 inval = storage;
    return serviceDispatchInOut(&g_capsaSrv, 1, inval, *count,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size } },
    );
}

Result capsaLoadAlbumFile(const CapsAlbumFileId *file_id, u64 *out_size, void* workbuf, u64 workbuf_size) {
    return serviceDispatchInOut(&g_capsaSrv, 2, *file_id, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { workbuf, workbuf_size } },
    );
}

Result capsaDeleteAlbumFile(const CapsAlbumFileId *file_id) {
    return serviceDispatchIn(&g_capsaSrv, 3, *file_id);
}

Result capsaStorageCopyAlbumFile(const CapsAlbumFileId* file_id, CapsAlbumStorage dst_storage) {
    struct {
        u8 storage;
        u8 pad_x1[0x7];
        CapsAlbumFileId file_id;
    } in = { dst_storage, {0}, *file_id };
    return serviceDispatchIn(&g_capsaSrv, 4, in);
}

Result capsaIsAlbumMounted(CapsAlbumStorage storage, bool* is_mounted) {
    u8 inval = storage;
    return serviceDispatchInOut(&g_capsaSrv, 5, inval, *is_mounted);
}

Result capsaGetAlbumUsage(CapsAlbumStorage storage, CapsAlbumUsage2 *out) {
    u8 inval = storage;
    return serviceDispatchInOut(&g_capsaSrv, 6, inval, *out);
}

Result capsaGetAlbumFileSize(const CapsAlbumFileId *file_id, u64* size) {
    return serviceDispatchInOut(&g_capsaSrv, 7, *file_id, *size);
}

Result capsaLoadAlbumFileThumbnail(const CapsAlbumFileId *file_id, u64 *out_size, void* image, u64 image_size) {
    return serviceDispatchInOut(&g_capsaSrv, 8, *file_id, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { image, image_size } },
    );
}

static Result _capsaLoadAlbumScreenshot(u64* width, u64* height, const CapsAlbumFileId *file_id, void* image, u64 image_size, void* workbuf, u64 workbuf_size, u32 cmd_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        u64 width;
        u64 height;
    } out;
    Result rc = serviceDispatchInOut(&g_capsaSrv, cmd_id, *file_id, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { image, image_size }, { workbuf, workbuf_size } },
    );
    *width = out.width;
    *height = out.height;
    return rc;
}

Result capsaLoadAlbumScreenShotImage(u64* width, u64* height, const CapsAlbumFileId *file_id, void* image, u64 image_size, void* workbuf, u64 workbuf_size) {
    return _capsaLoadAlbumScreenshot(width, height, file_id, image, image_size, workbuf, workbuf_size, 9);
}

Result capsaLoadAlbumScreenShotThumbnailImage(u64* width, u64* height, const CapsAlbumFileId *file_id, void* image, u64 image_size, void* workbuf, u64 workbuf_size) {
    return _capsaLoadAlbumScreenshot(width, height, file_id, image, image_size, workbuf, workbuf_size, 10);
}

static Result _capsaLoadAlbumScreenshotEx(u64* width, u64* height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size, u32 cmd_id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        CapsAlbumFileId file_id;
        CapsScreenShotDecodeOption opts;
    } in = { *file_id, *opts };
    struct {
        u64 width;
        u64 height;
    } out;
    Result rc = serviceDispatchInOut(&g_capsaSrv, cmd_id, in, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { image, image_size }, { workbuf, workbuf_size } },
    );
    *width = out.width;
    *height = out.height;
    return rc;
}

Result capsaLoadAlbumScreenShotImageEx(u64* width, u64* height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size) {
    return _capsaLoadAlbumScreenshotEx(width, height, file_id, opts, image, image_size, workbuf, workbuf_size, 12);
}

Result capsaLoadAlbumScreenShotThumbnailImageEx(u64* width, u64* height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size) {
    return _capsaLoadAlbumScreenshotEx(width, height, file_id, opts, image, image_size, workbuf, workbuf_size, 13);
}

Result capsaGetAlbumUsage3(CapsAlbumStorage storage, CapsAlbumUsage3 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u8 inval = storage;
    return serviceDispatchInOut(&g_capsaSrv, 15, inval, *out);
}

Result capsaGetAlbumMountResult(CapsAlbumStorage storage) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _capsaCmdInU8NoOut(&g_capsaSrv, storage, 16);
}

Result capsaGetAlbumUsage16(CapsAlbumStorage storage, CapsAlbumUsage16 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    u8 inval = storage;
    return serviceDispatchInOut(&g_capsaSrv, 17, inval, *out);
}

Result capsaGetAutoSavingStorage(CapsAlbumStorage* storage) {
    u8 tmpval = 0;
    Result rc = serviceDispatchOut(&g_capsaSrv, 401, tmpval);
    *storage = tmpval;
    return rc;
}

Result capsaGetRequiredStorageSpaceSizeToCopyAll(CapsAlbumStorage dst_storage, CapsAlbumStorage src_storage, u64* out) {
    struct {
        u8 dest;
        u8 src;
    } in = { dst_storage, src_storage };
    return serviceDispatchInOut(&g_capsaSrv, 501, in, *out);
}

Result capsaLoadAlbumScreenShotThumbnailImageEx1(const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size, void* out, u64 out_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        CapsAlbumFileId file_id;
        CapsScreenShotDecodeOption opts;
    } in = { *file_id, *opts };
    return serviceDispatchIn(&g_capsaSrv, 1003, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_FixedSize, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, out_size }, { image, image_size }, { workbuf, workbuf_size } },
    );
}

Result capsaForceAlbumUnmounted(CapsAlbumStorage storage) {
    return _capsaCmdInU8NoOut(&g_capsaSrv, storage, 8001);
}

Result capsaResetAlbumMountStatus(CapsAlbumStorage storage) {
    return _capsaCmdInU8NoOut(&g_capsaSrv, storage, 8002);
}

Result capsaRefreshAlbumCache(CapsAlbumStorage storage) {
    return _capsaCmdInU8NoOut(&g_capsaSrv, storage, 8011);
}

static Result _capsaOpenAccessorSession(Service* srv_out) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    return serviceDispatchIn(&g_capsaSrv, 60002, AppletResourceUserId,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _capsaOpenAlbumMovieReadStream(u64 *stream, const CapsAlbumFileId *file_id) {
    return serviceDispatchInOut(&g_capsaAccessor, 2001, *file_id, *stream);
}

static Result _capsaReadMovieDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    const struct {
        u64 stream;
        s64 offset;
    } in = { stream, offset };

    return serviceDispatchInOut(&g_capsaAccessor, 2004, in, *actual_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static Result _capsaReadImageDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    const struct {
        u64 stream;
        s64 offset;
    } in = { stream, offset };

    return serviceDispatchInOut(&g_capsaAccessor, 2007, in, *actual_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result capsaOpenAlbumMovieStream(u64 *stream, const CapsAlbumFileId *file_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;

    if (!serviceIsActive(&g_capsaAccessor)) rc = _capsaOpenAccessorSession(&g_capsaAccessor);

    if (R_SUCCEEDED(rc)) rc = _capsaOpenAlbumMovieReadStream(stream, file_id);

    return rc;
}

Result capsaCloseAlbumMovieStream(u64 stream) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchIn(&g_capsaAccessor, 2002, stream);
}

Result capsaGetAlbumMovieStreamSize(u64 stream, u64 *size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_capsaAccessor, 2003, stream, *size);
}

Result capsaReadMovieDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _capsaReadMovieDataFromAlbumMovieReadStream(stream, offset, buffer, size, actual_size);
}

Result capsaGetAlbumMovieReadStreamBrokenReason(u64 stream) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchIn(&g_capsaAccessor, 2005, stream);
}

Result capsaGetAlbumMovieReadStreamImageDataSize(u64 stream, u64 *size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_capsaAccessor, 2006, stream, *size);
}

Result capsaReadImageDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _capsaReadImageDataFromAlbumMovieReadStream(stream, offset, buffer, size, actual_size);
}

Result capsaReadFileAttributeFromAlbumMovieReadStream(u64 stream, CapsScreenShotAttributeForApplication* attribute) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capsaAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_capsaAccessor, 2008, stream, *attribute);
}
