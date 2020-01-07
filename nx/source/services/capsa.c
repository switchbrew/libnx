#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <time.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/capsa.h"

static Service g_capsaSrv;

NX_GENERATE_SERVICE_GUARD(capsa);

Result _capsaInitialize(void) {
    return smGetService(&g_capsaSrv, "caps:a");
}

void _capsaCleanup(void) {
    serviceClose(&g_capsaSrv);
}

Service* capsaGetServiceSession(void) {
    return &g_capsaSrv;
}

Result capsaGetAlbumFileCount(CapsAlbumStorage storage, u64* count) {
    return serviceDispatchInOut(&g_capsaSrv, 0, storage, *count);
}

Result capsaGetAlbumFileList(CapsAlbumStorage storage, u64* count, CapsApplicationAlbumEntry* buffer, u64 buffer_size) {
    return serviceDispatchInOut(&g_capsaSrv, 1, storage, *count,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size } },
    );
}

Result capsaLoadAlbumFile(CapsAlbumFileId file_id, u64 *out_size, void* jpeg_buffer, u64 jpeg_buffer_size) {
    return serviceDispatchInOut(&g_capsaSrv, 2, file_id, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { jpeg_buffer, jpeg_buffer_size } },
    );
}

Result capsaDeleteAlbumFile(CapsAlbumFileId file_id) {
    return serviceDispatchIn(&g_capsaSrv, 3, file_id);
}

Result capsaStorageCopyAlbumFile(CapsAlbumFileId file_id, CapsAlbumStorage dst_storage) {
    return serviceDispatchIn(&g_capsaSrv, 4, dst_storage);
}

Result capsaIsAlbumMounted(CapsAlbumStorage storage, bool* is_mounted) {
    return serviceDispatchInOut(&g_capsaSrv, 5, storage, *is_mounted);
}

Result capsaGetAlbumUsage(CapsAlbumStorage storage, CapsAlbumUsage2 *out) {
    return serviceDispatchInOut(&g_capsaSrv, 6, storage, *out);
}

Result capsaGetAlbumFileSize(CapsAlbumFileId file_id, u64* size) {
    return serviceDispatchInOut(&g_capsaSrv, 7, file_id, *size);
}

Result capsaLoadAlbumFileThumbnail(CapsAlbumFileId file_id, u64 *out_size, void* jpeg_buffer, u64 jpeg_buffer_size) {
    return serviceDispatchInOut(&g_capsaSrv, 8, file_id, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { jpeg_buffer, jpeg_buffer_size } },
    );
}

Result capsaGetAlbumUsage3(CapsAlbumStorage storage, CapsAlbumUsage3 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchInOut(&g_capsaSrv, 15, storage, *out);
}

Result capsaGetAlbumMountResult(CapsAlbumStorage storage) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_capsaSrv, 17, storage);
}

Result capsaGetAlbumUsage16(CapsAlbumStorage storage, CapsAlbumUsage16 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchInOut(&g_capsaSrv, 17, storage, *out);
}


Result capsaGetAutoSavingStorage(CapsAlbumStorage* storage) {
    return serviceDispatchOut(&g_capsaSrv, 401, *storage);
}

Result capsaGetRequiredStorageSpaceSizeToCopyAll(CapsAlbumStorage dst_storage, CapsAlbumStorage src_storage, u64* out) {
    struct {
        CapsAlbumStorage dest;
        CapsAlbumStorage src;
    } in = { dst_storage, src_storage };
    return serviceDispatchInOut(&g_capsaSrv, 501, in, *out);
}

Result capsaLoadAlbumScreenShotThumbnailImage(u64* width, u64* height, CapsAlbumFileId file_id, void* jpeg_buffer, u64 jpeg_buffer_size, void* buffer, u64 buffer_size) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        u64 width;
        u64 height;
    } out;
    Result rc = serviceDispatchInOut(&g_capsaSrv, 10, file_id, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size }, { jpeg_buffer, jpeg_buffer_size } },
    );
    *width = out.width;
    *height = out.height;
    return rc;
}

Result capsaLoadAlbumScreenShotThumbnailImageEx(u64* width, u64* height, CapsAlbumFileId file_id, void* jpeg_buffer, u64 jpeg_buffer_size, void* buffer, u64 buffer_size) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        CapsAlbumFileId file_id;
        CapsScreenShotDecodeOption opts;
    } in = { file_id, {{0}} };
    struct {
        u64 width;
        u64 height;
    } out;
    Result rc = serviceDispatchInOut(&g_capsaSrv, 13, in, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size }, { jpeg_buffer, jpeg_buffer_size } },
    );
    *width = out.width;
    *height = out.height;
    return rc;
}

Result capsaLoadAlbumScreenShotThumbnailImageEx1(CapsAlbumFileId file_id, void* jpeg_buffer, u64 jpeg_buffer_size, void* buffer, u64 buffer_size, void* out, u64 out_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    struct {
        CapsAlbumFileId file_id;
        CapsScreenShotDecodeOption opts;
    } in = { file_id, {{0}} };
    return serviceDispatchIn(&g_capsaSrv, 1003, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_FixedSize, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure, SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, out_size }, { buffer, buffer_size }, { jpeg_buffer, jpeg_buffer_size } },
    );
}

Result capsaForceAlbumUnmounted(CapsAlbumStorage storage) {
    return serviceDispatchIn(&g_capsaSrv, 8001, storage);
}

Result capsaResetAlbumMountStatus(CapsAlbumStorage storage) {
    return serviceDispatchIn(&g_capsaSrv, 8002, storage);
}

Result capsaRefreshAlbumCache(CapsAlbumStorage storage) {
    return serviceDispatchIn(&g_capsaSrv, 8011, storage);
}
