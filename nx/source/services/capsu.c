#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <time.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/capsu.h"

static Service g_capsuSrv;
static Service g_capsuAccessor;

static Result _capsuSetShimLibraryVersion(u64 version);

NX_GENERATE_SERVICE_GUARD(capsu);

Result _capsuInitialize(void) {
    Result rc=0;

    if (hosversionBefore(5,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capsuSrv, "caps:u");

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _capsuSetShimLibraryVersion(capsGetShimLibraryVersion());

    return rc;
}

void _capsuCleanup(void) {
    serviceClose(&g_capsuAccessor);
    serviceClose(&g_capsuSrv);
}

Service* capsuGetServiceSession(void) {
    return &g_capsuSrv;
}

Service* capsuGetServiceSession_Accessor(void) {
    return &g_capsuAccessor;
}

static Result _capsuCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _capsuSetShimLibraryVersion(u64 version) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        u64 version;
        u64 AppletResourceUserId;
    } in = { version, AppletResourceUserId };

    return serviceDispatchIn(&g_capsuSrv, 32, in,
        .in_send_pid = true,
    );
}

static Result _capsuGetAlbumFileList0AafeAruidDeprecated(void* entries, size_t entrysize, s32 count, u8 type, u64 start_timestamp, u64 end_timestamp, s32 *total_entries) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        u8 type;
        u8 pad[7];
        u64 start_timestamp;
        u64 end_timestamp;
        u64 AppletResourceUserId;
    } in = { type, {0}, start_timestamp, end_timestamp, AppletResourceUserId };

    u64 total_out=0;
    Result rc = serviceDispatchInOut(&g_capsuSrv, 102, in, total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { entries, count*entrysize } },
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = total_out;
    return rc;
}

static Result _capsuDeleteAlbumFileByAruid(u32 cmd_id, u8 type, const CapsApplicationAlbumFileEntry *entry) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        u8 type;
        u8 pad[7];
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } in = { type, {0}, *entry, AppletResourceUserId };

    return serviceDispatchIn(&g_capsuSrv, 103, in,
        .in_send_pid = true,
    );
}

static Result _capsuGetAlbumFileSizeByAruid(const CapsApplicationAlbumFileEntry *entry, u64 *size) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } in = { *entry, AppletResourceUserId };

    return serviceDispatchInOut(&g_capsuSrv, 104, in, *size,
        .in_send_pid = true,
    );
}

static Result _capsuPrecheckToCreateContentsByAruid(u8 type, u64 unk) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        u8 type;
        u8 pad[7];
        u64 unk;
        u64 AppletResourceUserId;
    } in = { type, {0}, unk, AppletResourceUserId };

    return serviceDispatchIn(&g_capsuSrv, 130, in,
        .in_send_pid = true,
    );
}

static Result _capsuLoadAlbumScreenShotImageByAruid(u32 cmd_id, CapsLoadAlbumScreenShotImageOutputForApplication *out, void* image, size_t image_size, void* workbuf, size_t workbuf_size, const CapsApplicationAlbumFileEntry *entry, const CapsScreenShotDecodeOption *option) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        CapsApplicationAlbumFileEntry entry;
        CapsScreenShotDecodeOption option;
        u64 AppletResourceUserId;
    } in = { *entry, *option, AppletResourceUserId };

    return serviceDispatchIn(&g_capsuSrv, cmd_id, in,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { out, sizeof(*out) },
            { image, image_size },
            { workbuf, workbuf_size },
        },
        .in_send_pid = true,
    );
}

static Result _capsuGetAlbumFileListAaeAruid(u32 cmd_id, void* entries, size_t entrysize, s32 count, u8 type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, s32 *total_entries) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        u8 type;
        u8 pad;
        CapsAlbumFileDateTime start_datetime;
        CapsAlbumFileDateTime end_datetime;
        u8 pad2[6];
        u64 AppletResourceUserId;
    } in = { type, 0, *start_datetime, *end_datetime, {0}, AppletResourceUserId };

    u64 total_out=0;
    Result rc = serviceDispatchInOut(&g_capsuSrv, cmd_id, in, total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { entries, count*entrysize } },
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = total_out;
    return rc;
}

static Result _capsuGetAlbumFileListAaeUidAruid(u32 cmd_id, void* entries, size_t entrysize, s32 count, u8 type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, AccountUid uid, s32 *total_entries) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        u8 type;
        u8 pad;
        CapsAlbumFileDateTime start_datetime;
        CapsAlbumFileDateTime end_datetime;
        u8 pad2[6];
        AccountUid uid;
        u64 AppletResourceUserId;
    } in = { type, 0, *start_datetime, *end_datetime, {0}, uid, AppletResourceUserId };

    u64 total_out=0;
    Result rc = serviceDispatchInOut(&g_capsuSrv, cmd_id, in, total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { entries, count*entrysize } },
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = total_out;
    return rc;
}

static Result _capsuOpenAccessorSessionForApplication(Service* srv_out, const CapsApplicationAlbumFileEntry *entry) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } in = { *entry, AppletResourceUserId };

    return serviceDispatchIn(&g_capsuSrv, 60002, in,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _capsuOpenAlbumMovieReadStream(u64 *stream, const CapsApplicationAlbumFileEntry *entry) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    const struct {
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } in = { *entry, AppletResourceUserId };

    return serviceDispatchInOut(&g_capsuAccessor, 2001, in, *stream,
        .in_send_pid = true,
    );
}

static Result _capsuGetAlbumMovieReadStreamMovieDataSize(u64 stream, u64 *size) {
    return serviceDispatchInOut(&g_capsuAccessor, 2003, stream, *size);
}

static Result _capsuReadMovieDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    const struct {
        u64 stream;
        s64 offset;
    } in = { stream, offset };

    return serviceDispatchInOut(&g_capsuAccessor, 2004, in, *actual_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static inline u64 _capsuMakeTimestamp(const CapsAlbumFileDateTime *datetime) {
    struct tm tmptm = {.tm_sec = datetime->second, .tm_min = datetime->minute, .tm_hour = datetime->hour,
                       .tm_mday = datetime->day, .tm_mon = datetime->month, .tm_year = datetime->year - 1900};

    return mktime(&tmptm);
}

Result capsuGetAlbumFileListDeprecated1(CapsApplicationAlbumFileEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, s32 *total_entries) {
    u64 start_timestamp = 0x386BF200;
    u64 end_timestamp = 0xF4865700;

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    if (hosversionBefore(6,0,0)) { // GetAlbumFileListDeprecated0
        if (start_datetime) start_timestamp = _capsuMakeTimestamp(start_datetime);
        if (end_datetime) end_timestamp = _capsuMakeTimestamp(end_datetime);
        return _capsuGetAlbumFileList0AafeAruidDeprecated(entries, sizeof(CapsApplicationAlbumFileEntry), count, type, start_timestamp, end_timestamp, total_entries);
    }

    return _capsuGetAlbumFileListAaeAruid(140, entries, sizeof(CapsApplicationAlbumFileEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, total_entries);
}

Result capsuGetAlbumFileListDeprecated2(CapsApplicationAlbumFileEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, AccountUid uid, s32 *total_entries) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    return _capsuGetAlbumFileListAaeUidAruid(141, entries, sizeof(CapsApplicationAlbumFileEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, uid, total_entries);
}

Result capsuGetAlbumFileList3(CapsApplicationAlbumEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, s32 *total_entries) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    return _capsuGetAlbumFileListAaeAruid(142, entries, sizeof(CapsApplicationAlbumEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, total_entries);
}

Result capsuGetAlbumFileList4(CapsApplicationAlbumEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, AccountUid uid, s32 *total_entries) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    return _capsuGetAlbumFileListAaeUidAruid(143, entries, sizeof(CapsApplicationAlbumEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, uid, total_entries);
}

Result capsuDeleteAlbumFile(CapsContentType type, const CapsApplicationAlbumFileEntry *entry) {
    return _capsuDeleteAlbumFileByAruid(103, type, entry);
}

Result capsuGetAlbumFileSize(const CapsApplicationAlbumFileEntry *entry, u64 *size) {
    return _capsuGetAlbumFileSizeByAruid(entry, size);
}

static void _capsuProcessImageOutput(CapsLoadAlbumScreenShotImageOutputForApplication *out, s32 *width, s32 *height, CapsScreenShotAttributeForApplication *attr, void* userdata, size_t userdata_maxsize, u32 *userdata_size) {
    if (out==NULL) return;

    if (width) *width = out->width;
    if (height) *height = out->height;
    if (attr) memcpy(attr, &out->attr, sizeof(out->attr));

    if (userdata && userdata_maxsize) {
        memset(userdata, 0, userdata_maxsize);
        if (userdata_maxsize > sizeof(out->appdata.userdata)) userdata_maxsize = sizeof(out->appdata.userdata);
        if (userdata_maxsize > out->appdata.size) userdata_maxsize = out->appdata.size;
        memcpy(userdata, out->appdata.userdata, userdata_maxsize);
    }
    if (userdata_size) *userdata_size = out->appdata.size > sizeof(out->appdata.userdata) ? sizeof(out->appdata.userdata) : out->appdata.size;
}

Result capsuLoadAlbumScreenShotImage(s32 *width, s32 *height, CapsScreenShotAttributeForApplication *attr, void* userdata, size_t userdata_maxsize, u32 *userdata_size, void* image, size_t image_size, void* workbuf, size_t workbuf_size, const CapsApplicationAlbumFileEntry *entry, const CapsScreenShotDecodeOption *option) {
    Result rc=0;
    CapsLoadAlbumScreenShotImageOutputForApplication out={0};

    rc = _capsuLoadAlbumScreenShotImageByAruid(110, &out, image, image_size, workbuf, workbuf_size, entry, option);
    if (R_SUCCEEDED(rc)) _capsuProcessImageOutput(&out, width, height, attr, userdata, userdata_maxsize, userdata_size);
    return rc;
}

Result capsuLoadAlbumScreenShotThumbnailImage(s32 *width, s32 *height, CapsScreenShotAttributeForApplication *attr, void* userdata, size_t userdata_maxsize, u32 *userdata_size, void* image, size_t image_size, void* workbuf, size_t workbuf_size, const CapsApplicationAlbumFileEntry *entry, const CapsScreenShotDecodeOption *option) {
    Result rc=0;
    CapsLoadAlbumScreenShotImageOutputForApplication out={0};

    rc = _capsuLoadAlbumScreenShotImageByAruid(120, &out, image, image_size, workbuf, workbuf_size, entry, option);
    if (R_SUCCEEDED(rc)) _capsuProcessImageOutput(&out, width, height, attr, userdata, userdata_maxsize, userdata_size);
    return rc;
}

Result capsuPrecheckToCreateContents(CapsContentType type, u64 unk) {
    return _capsuPrecheckToCreateContentsByAruid(type, unk);
}

Result capsuOpenAlbumMovieStream(u64 *stream, const CapsApplicationAlbumFileEntry *entry) {
    Result rc=0;

    if (!serviceIsActive(&g_capsuAccessor)) rc =_capsuOpenAccessorSessionForApplication(&g_capsuAccessor, entry);

    if (R_SUCCEEDED(rc)) rc = _capsuOpenAlbumMovieReadStream(stream, entry);

    return rc;
}

Result capsuCloseAlbumMovieStream(u64 stream) {
    if (!serviceIsActive(&g_capsuAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _capsuCmdInU64NoOut(&g_capsuAccessor, stream, 2002);
}

Result capsuGetAlbumMovieStreamSize(u64 stream, u64 *size) {
    if (!serviceIsActive(&g_capsuAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _capsuGetAlbumMovieReadStreamMovieDataSize(stream, size);
}

Result capsuReadAlbumMovieStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    if (!serviceIsActive(&g_capsuAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _capsuReadMovieDataFromAlbumMovieReadStream(stream, offset, buffer, size, actual_size);
}

Result capsuGetAlbumMovieStreamBrokenReason(u64 stream) {
    if (!serviceIsActive(&g_capsuAccessor))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _capsuCmdInU64NoOut(&g_capsuAccessor, stream, 2005);
}

