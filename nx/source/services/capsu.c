#include <string.h>
#include <time.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/caps.h"
#include "services/capsu.h"
#include "services/sm.h"

static Service g_capsuSrv;
static Service g_capsuAccessor;
static u64 g_capsuRefCnt;

static Result _capsuSetShimLibraryVersion(u64 version);

Result capsuInitialize(void) {
    Result rc=0;

    atomicIncrement64(&g_capsuRefCnt);

    if (serviceIsActive(&g_capsuSrv))
        return 0;

    if (hosversionBefore(5,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capsuSrv, "caps:u");

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _capsuSetShimLibraryVersion(capsGetShimLibraryVersion());

    if (R_FAILED(rc)) capsuExit();

    return rc;
}

void capsuExit(void) {
    if (atomicDecrement64(&g_capsuRefCnt) == 0) {
        serviceClose(&g_capsuAccessor);
        serviceClose(&g_capsuSrv);
    }
}

Service* capsuGetServiceSession(void) {
    return &g_capsuSrv;
}

Service* capsuGetServiceSession_Accessor(void) {
    return &g_capsuAccessor;
}

static Result _capsuCmdInU64(Service* srv, u64 inval, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _capsuSetShimLibraryVersion(u64 version) {
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

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 32;
    raw->version = version;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _capsuGetAlbumFileList0AafeAruidDeprecated(void* entries, size_t entrysize, size_t count, u8 type, u64 start_timestamp, u64 end_timestamp, u64 *total_entries) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 type;
        u64 start_timestamp;
        u64 end_timestamp;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, entries, count*entrysize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 102;
    raw->type = type;
    raw->start_timestamp = start_timestamp;
    raw->end_timestamp = end_timestamp;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

static Result _capsuDeleteAlbumFileByAruid(u64 cmd_id, u8 type, const CapsApplicationAlbumFileEntry *entry) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 type;
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->type = type;
    raw->entry = *entry;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _capsuGetAlbumFileSizeByAruid(const CapsApplicationAlbumFileEntry *entry, u64 *size) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 104;
    raw->entry = *entry;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

static Result _capsuPrecheckToCreateContentsByAruid(u8 type, u64 unk) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 type;
        u64 unk;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 130;
    raw->type = type;
    raw->unk = unk;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _capsuLoadAlbumScreenShotImageByAruid(u64 cmd_id, CapsLoadAlbumScreenShotImageOutputForApplication *out, void* image, size_t image_size, void* workbuf, size_t workbuf_size, const CapsApplicationAlbumFileEntry *entry, const CapsScreenShotDecodeOption *option) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsApplicationAlbumFileEntry entry;
        CapsScreenShotDecodeOption option;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, out, sizeof(*out), BufferType_Normal);
    ipcAddRecvBuffer(&c, image, image_size, BufferType_Type1);
    ipcAddRecvBuffer(&c, workbuf, workbuf_size, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->entry = *entry;
    raw->option = *option;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _capsuGetAlbumFileListAaeAruid(u64 cmd_id, void* entries, size_t entrysize, size_t count, u8 type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, u64 *total_entries) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 type;
        CapsAlbumFileDateTime start_datetime;
        CapsAlbumFileDateTime end_datetime;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, entries, count*entrysize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->type = type;
    raw->start_datetime = *start_datetime;
    raw->end_datetime = *end_datetime;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

static Result _capsuGetAlbumFileListAaeUidAruid(u64 cmd_id, void* entries, size_t entrysize, size_t count, u8 type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, u128 userID, u64 *total_entries) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 type;
        CapsAlbumFileDateTime start_datetime;
        CapsAlbumFileDateTime end_datetime;
        u8 pad[6];
        union { u128 userID; } PACKED;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    ipcAddRecvBuffer(&c, entries, count*entrysize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->type = type;
    raw->start_datetime = *start_datetime;
    raw->end_datetime = *end_datetime;
    raw->userID = userID;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

static Result _capsuOpenAccessorSessionForApplication(Service* srv_out, const CapsApplicationAlbumFileEntry *entry) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(&g_capsuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 60002;
    raw->entry = *entry;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capsuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && srv_out) {
            serviceCreateSubservice(srv_out, &g_capsuSrv, &r, 0);
        }
    }

    return rc;
}

static Result _capsuOpenAlbumMovieReadStream(u64 *stream, const CapsApplicationAlbumFileEntry *entry) {
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        CapsApplicationAlbumFileEntry entry;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = serviceIpcPrepareHeader(&g_capsuAccessor, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2001;
    raw->entry = *entry;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_capsuAccessor);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 stream;
        } *resp;

        serviceIpcParse(&g_capsuAccessor, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && stream) *stream = resp->stream;
    }

    return rc;
}

static Result _capsuGetAlbumMovieReadStreamMovieDataSize(u64 stream, u64 *size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 stream;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_capsuAccessor, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2003;
    raw->stream = stream;

    Result rc = serviceIpcDispatch(&g_capsuAccessor);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp;

        serviceIpcParse(&g_capsuAccessor, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

static Result _capsuReadMovieDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 stream;
        s64 offset;
    } *raw;

    ipcAddRecvBuffer(&c, buffer, size, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_capsuAccessor, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2004;
    raw->stream = stream;
    raw->offset = offset;

    Result rc = serviceIpcDispatch(&g_capsuAccessor);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 actual_size;
        } *resp;

        serviceIpcParse(&g_capsuAccessor, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && actual_size) *actual_size = resp->actual_size;
    }

    return rc;
}

static inline u64 _capsuMakeTimestamp(const CapsAlbumFileDateTime *datetime) {
    struct tm tmptm = {.tm_sec = datetime->second, .tm_min = datetime->minute, .tm_hour = datetime->hour,
                       .tm_mday = datetime->day, .tm_mon = datetime->month, .tm_year = datetime->year - 1900};

    return mktime(&tmptm);
}

Result capsuGetAlbumFileListDeprecated1(CapsApplicationAlbumFileEntry *entries, size_t count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, u64 *total_entries) {
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

Result capsuGetAlbumFileListDeprecated2(CapsApplicationAlbumFileEntry *entries, size_t count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, u128 userID, u64 *total_entries) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    return _capsuGetAlbumFileListAaeUidAruid(141, entries, sizeof(CapsApplicationAlbumFileEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, userID, total_entries);
}

Result capsuGetAlbumFileList3(CapsApplicationAlbumEntry *entries, size_t count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, u64 *total_entries) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    return _capsuGetAlbumFileListAaeAruid(142, entries, sizeof(CapsApplicationAlbumEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, total_entries);
}

Result capsuGetAlbumFileList4(CapsApplicationAlbumEntry *entries, size_t count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, u128 userID, u64 *total_entries) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    CapsAlbumFileDateTime default_start = capsGetDefaultStartDateTime();
    CapsAlbumFileDateTime default_end = capsGetDefaultEndDateTime();

    return _capsuGetAlbumFileListAaeUidAruid(143, entries, sizeof(CapsApplicationAlbumEntry), count, type, start_datetime ? start_datetime : &default_start, end_datetime ? end_datetime : &default_end, userID, total_entries);
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

    return _capsuCmdInU64(&g_capsuAccessor, stream, 2002);
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

    return _capsuCmdInU64(&g_capsuAccessor, stream, 2005);
}

