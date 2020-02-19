#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <time.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/capsc.h"

static Service g_capscSrv;
static Service g_capscControl;

static Result _capscSetShimLibraryVersion(u64 version);

NX_GENERATE_SERVICE_GUARD(capsc);

Result _capscInitialize(void) {
    Result rc=0;

    rc = smGetService(&g_capscSrv, "caps:c");

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _capscSetShimLibraryVersion(capsGetShimLibraryVersion());

    return rc;
}

void _capscCleanup(void) {
    serviceClose(&g_capscControl);
    serviceClose(&g_capscSrv);
}

Service* capscGetServiceSession(void) {
    return &g_capscSrv;
}

static Result _capscSetShimLibraryVersion(u64 version) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        u64 version;
        u64 AppletResourceUserId;
    } in = { version, appletGetAppletResourceUserId() };
    return serviceDispatchIn(&g_capscSrv, 33, in);
}

static Result _capscCmdInU8NoOut(Service *srv, u32 cmd_id, u64 inval) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

Result capscNotifyAlbumStorageIsAvailable(CapsAlbumStorage storage) {
    u8 inval = storage;
    return _capscCmdInU8NoOut(&g_capscSrv, 2001, inval);
}

Result capscNotifyAlbumStorageIsUnAvailable(CapsAlbumStorage storage) {
    u8 inval = storage;
    return _capscCmdInU8NoOut(&g_capscSrv, 2002, inval);
}

Result capscRegisterAppletResourceUserId(u64 appletResourceUserId, u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        u64 appletResourceUserId;
        u64 applicationId;
    } in = { appletResourceUserId, application_id };
    return serviceDispatchIn(&g_capscSrv, 2011, in);
}

Result capscUnregisterAppletResourceUserId(u64 appletResourceUserId, u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        u64 appletResourceUserId;
        u64 applicationId;
    } in = { appletResourceUserId, application_id };
    return serviceDispatchIn(&g_capscSrv, 2012, in);
}

Result capscGetApplicationIdFromAruid(u64 *application_id, u64 aruid) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchInOut(&g_capscSrv, 2013, aruid, *application_id);
}

Result capscCheckApplicationIdRegistered(u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_capscSrv, 2014, application_id);
}

Result capscGenerateCurrentAlbumFileId(u64 application_id, CapsAlbumFileContents contents, CapsAlbumFileId *file_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        u8 type;
        u64 applicationId;
    } in = { contents, application_id };
    return serviceDispatchInOut(&g_capscSrv, 2101, in, *file_id);
}

Result capscGenerateApplicationAlbumEntry(CapsApplicationAlbumEntry *appEntry, const CapsAlbumEntry *entry, u64 application_id) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        CapsAlbumEntry entry;
        u64 applicationId;
    } in = { *entry, application_id };
    return serviceDispatchInOut(&g_capscSrv, 2102, in, *appEntry);
}

Result capscSaveAlbumScreenShotFile(const CapsAlbumFileId *file_id, const void* buffer, u64 buffer_size) {
    if (hosversionBefore(2,0,0) || hosversionAtLeast(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_capscSrv, 2201, file_id,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, buffer_size }, },
    );
}

Result capscSaveAlbumScreenShotFileEx(const CapsAlbumFileId *file_id, u64 version, u64 makernote_offset, u64 makernote_size, const void* buffer, u64 buffer_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const struct {
        CapsAlbumFileId file_id;
        u64 version;
        u64 mn_offset;
        u64 mn_size;
    } in = { *file_id, version, makernote_offset, makernote_size };
    return serviceDispatchIn(&g_capscSrv, 2202, in,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, buffer_size }, },
    );
}

static Result _capscSetOverlayThumbnailData(u32 cmd_id, const CapsAlbumFileId *file_id, const void* image, u64 image_size) {
    return serviceDispatchIn(&g_capscSrv, cmd_id, *file_id,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { image, image_size }, },
    );
}

Result capscSetOverlayScreenShotThumbnailData(const CapsAlbumFileId *file_id, const void* image, u64 image_size) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _capscSetOverlayThumbnailData(2301, file_id, image, image_size);
}

Result capscSetOverlayMovieThumbnailData(const CapsAlbumFileId *file_id, const void* image, u64 image_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _capscSetOverlayThumbnailData(2302, file_id, image, image_size);
}

static Result _capscOpenControlSession(Service *srv_out) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();
    return serviceDispatchIn(&g_capscSrv, 60001, AppletResourceUserId,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _capscOpenAlbumMovieStream(u32 cmd_id, const CapsAlbumFileId *file_id, u64 *stream) {
    return serviceDispatchInOut(&g_capscControl, cmd_id, *file_id, *stream);
}

static Result _capscControlReadDataFromAlbumMovieStream(u32 cmd_id, u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    const struct {
        u64 stream;
        u64 offset;
    } in = { stream, offset };
    return serviceDispatchInOut(&g_capscControl, cmd_id, in, *actual_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static Result _capscControlCmdInU64NoOut(u32 cmd_id, u64 inval) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return serviceDispatchIn(&g_capscControl, cmd_id, inval);
}

Result capscOpenAlbumMovieReadStream(u64 *stream, const CapsAlbumFileId *file_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    Result rc=0;
    if (!serviceIsActive(&g_capscControl)) rc = _capscOpenControlSession(&g_capscControl);
    if (R_SUCCEEDED(rc)) rc = _capscOpenAlbumMovieStream(2001, file_id, stream);
    return rc;
}

Result capscCloseAlbumMovieStream(u64 stream) {
    return _capscControlCmdInU64NoOut(2002, stream);
}

Result capscGetAlbumMovieStreamSize(u64 stream, u64 *size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return serviceDispatchInOut(&g_capscControl, 2003, stream, *size);
}

Result capscReadMovieDataFromAlbumMovieReadStream(u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size) {
    return _capscControlReadDataFromAlbumMovieStream(2004, stream, offset, buffer, size, actual_size);
}

Result capscGetAlbumMovieReadStreamBrokenReason(u64 stream) {
    return _capscControlCmdInU64NoOut(2005, stream);
}

Result capscGetAlbumMovieReadStreamImageDataSize(u64 stream, u64 *size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return serviceDispatchInOut(&g_capscControl, 2006, stream, *size);
}

Result capscReadImageDataFromAlbumMovieReadStream(u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size) {
    return _capscControlReadDataFromAlbumMovieStream(2007, stream, offset, buffer, size, actual_size);
}

Result capscReadFileAttributeFromAlbumMovieReadStream(u64 stream, CapsScreenShotAttribute *attribute) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_capscControl, 2008, stream, *attribute);
}

Result capscOpenAlbumMovieWriteStream(u64 *stream, const CapsAlbumFileId *file_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;

    if (!serviceIsActive(&g_capscControl)) rc = _capscOpenControlSession(&g_capscControl);

    if (R_SUCCEEDED(rc)) rc = _capscOpenAlbumMovieStream(2401, file_id, stream);

    return rc;
}

Result capscFinishAlbumMovieWriteStream(u64 stream) {
    return _capscControlCmdInU64NoOut(2402, stream);
}

Result capscCommitAlbumMovieWriteStream(u64 stream) {
    return _capscControlCmdInU64NoOut(2403, stream);
}

Result capscDiscardAlbumMovieWriteStream(u64 stream) {
    return _capscControlCmdInU64NoOut(2404, stream);
}

Result capscDiscardAlbumMovieWriteStreamNoDelete(u64 stream) {
    return _capscControlCmdInU64NoOut(2405, stream);
}

Result capscCommitAlbumMovieWriteStreamEx(u64 stream, CapsAlbumEntry *entry) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    return serviceDispatchInOut(&g_capscControl, 2406, stream, *entry);
}

Result capscStartAlbumMovieWriteStreamDataSection(u64 stream) {
    return _capscControlCmdInU64NoOut(2411, stream);
}

Result capscEndAlbumMovieWriteStreamDataSection(u64 stream) {
    return _capscControlCmdInU64NoOut(2412, stream);
}

Result capscStartAlbumMovieWriteStreamMetaSection(u64 stream) {
    return _capscControlCmdInU64NoOut(2413, stream);
}

Result capscEndAlbumMovieWriteStreamMetaSection(u64 stream) {
    return _capscControlCmdInU64NoOut(2414, stream);
}

Result capscReadDataFromAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size, u64 *actual_size) {
    return _capscControlReadDataFromAlbumMovieStream(2421, stream, offset, buffer, size, actual_size);
}

static Result _capscWriteToAlbumMovieWriteStream(u32 cmd_id, u64 stream, u64 offset, void* buffer, u64 size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    const struct {
        u64 stream;
        u64 offset;
    } in = { stream, offset };
    return serviceDispatchIn(&g_capscControl, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result capscWriteDataToAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size) {
    return _capscWriteToAlbumMovieWriteStream(2422, stream, offset, buffer, size);
}

Result capscWriteMetaToAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size) {
    return _capscWriteToAlbumMovieWriteStream(2424, stream, offset, buffer, size);
}

Result capscGetAlbumMovieWriteStreamBrokenReason(u64 stream) {
    return _capscControlCmdInU64NoOut(2431, stream);
}

Result capscGetAlbumMovieWriteStreamDataSize(u64 stream, u64 *size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchInOut(&g_capscControl, 2433, stream, *size);
}

Result capscSetAlbumMovieWriteStreamDataSize(u64 stream, u64 size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_capscControl))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u64 stream;
        u64 size;
    } in = { stream, size };
    return serviceDispatchIn(&g_capscControl, 2434, in);
}
