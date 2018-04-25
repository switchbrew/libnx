#include <string.h>

#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/shmem.h"
#include "services/sm.h"
#include "services/pl.h"

#define SHAREDMEMFONT_SIZE 0x1100000

static Service g_plSrv;
static u64 g_plRefCnt;
static SharedMemory g_plSharedmem;

static Result _plGetSharedMemoryNativeHandle(Handle* handle_out);

Result plInitialize(void)
{
    Result rc=0;
    Handle sharedmem_handle=0;

    atomicIncrement64(&g_plRefCnt);

    if (serviceIsActive(&g_plSrv))
        return 0;

    rc = smGetService(&g_plSrv, "pl:u");

    if (R_SUCCEEDED(rc))
    {
        rc = _plGetSharedMemoryNativeHandle(&sharedmem_handle);

        if (R_SUCCEEDED(rc))
        {
            shmemLoadRemote(&g_plSharedmem, sharedmem_handle, 0x1100000, Perm_R);

            rc = shmemMap(&g_plSharedmem);
        }
    }

    if (R_FAILED(rc)) plExit();

    return rc;
}

void plExit(void)
{
    if (atomicDecrement64(&g_plRefCnt) == 0) {
        serviceClose(&g_plSrv);
        shmemClose(&g_plSharedmem);
    }
}

void* plGetSharedmemAddr(void) {
    return shmemGetAddr(&g_plSharedmem);
}

static Result _plRequestLoad(u32 SharedFontType) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 SharedFontType;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->SharedFontType = SharedFontType;

    Result rc = serviceIpcDispatch(&g_plSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _plGetLoadState(u32 SharedFontType, u32* LoadState) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 SharedFontType;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->SharedFontType = SharedFontType;

    Result rc = serviceIpcDispatch(&g_plSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 LoadState;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && LoadState) *LoadState = resp->LoadState;
    }

    return rc;
}

static Result _plGetSize(u32 SharedFontType, u32* size) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 SharedFontType;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->SharedFontType = SharedFontType;

    Result rc = serviceIpcDispatch(&g_plSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

static Result _plGetSharedMemoryAddressOffset(u32 SharedFontType, u32* offset) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 SharedFontType;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->SharedFontType = SharedFontType;

    Result rc = serviceIpcDispatch(&g_plSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 offset;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && offset) *offset = resp->offset;
    }

    return rc;
}

static Result _plGetSharedMemoryNativeHandle(Handle* handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_plSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

static Result _plVerifyFontRange(u32 offset, u32 size) {
    if (offset >= SHAREDMEMFONT_SIZE || size > SHAREDMEMFONT_SIZE || ((u64)offset + (u64)size) > SHAREDMEMFONT_SIZE)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return 0;
}

static Result _plRequestLoadWait(u32 SharedFontType) {
    Result rc=0;
    u32 LoadState=0;
    bool load_started = 0;

    while(1) {//Wait/request for the font to be loaded if it's not already loaded.
        if (load_started) svcSleepThread(1000000000ULL / 60);

        rc = _plGetLoadState(SharedFontType, &LoadState);
        if (R_FAILED(rc) || LoadState==0x1) break;

        if (load_started) continue;
        load_started = 1;
        rc = _plRequestLoad(SharedFontType);
        if (R_FAILED(rc)) break;
    }

    return rc;
}

Result plGetSharedFontByType(PlFontData* font, u32 SharedFontType) {
    Result rc=0;
    u8* sharedmem_addr = (u8*)plGetSharedmemAddr();

    memset(font, 0, sizeof(PlFontData));

    font->type = SharedFontType;

    rc = _plRequestLoadWait(SharedFontType);
    if (R_FAILED(rc)) return rc;

    rc = _plGetSize(SharedFontType, &font->size);
    if (R_FAILED(rc)) return rc;

    rc = _plGetSharedMemoryAddressOffset(SharedFontType, &font->offset);
    if (R_FAILED(rc)) return rc;

    rc = _plVerifyFontRange(font->offset, font->size);
    if (R_FAILED(rc)) return rc;

    font->address = &sharedmem_addr[font->offset];

    return rc;
}

Result plGetSharedFont(u64 LanguageCode, PlFontData* fonts, size_t max_fonts, size_t* total_fonts) {
    Result rc=0;
    u32 types[PlSharedFontType_Total];
    u32 offsets[PlSharedFontType_Total];
    u32 sizes[PlSharedFontType_Total];
    size_t size = sizeof(u32) * PlSharedFontType_Total;
    u32 font_count=0, i;
    u8* sharedmem_addr = (u8*)plGetSharedmemAddr();

    memset(types, 0, sizeof(types));
    memset(offsets, 0, sizeof(offsets));
    memset(sizes, 0, sizeof(sizes));
    memset(fonts, 0, sizeof(PlFontData) * max_fonts);

    if (total_fonts) *total_fonts = 0;

    for (i=0; i<PlSharedFontType_Total; i++) {
        rc = _plRequestLoadWait(i);
        if (R_FAILED(rc)) return rc;
    }

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, types, size, 0);
    ipcAddRecvBuffer(&c, offsets, size, 0);
    ipcAddRecvBuffer(&c, sizes, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 LanguageCode;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->LanguageCode = LanguageCode;

    rc = serviceIpcDispatch(&g_plSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 fonts_loaded;
            u32 total_fonts;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (resp->fonts_loaded==0) return rc;

            font_count = resp->total_fonts;
            if (font_count > PlSharedFontType_Total) font_count = PlSharedFontType_Total;
            if (font_count > max_fonts) font_count = max_fonts;
            if (total_fonts) *total_fonts = font_count;
            if (font_count==0) return rc;

            for (i=0; i<font_count; i++) {
                rc = _plVerifyFontRange(offsets[i], sizes[i]);
                if (R_FAILED(rc)) return rc;

                fonts[i].type = types[i];
                fonts[i].offset = offsets[i];
                fonts[i].size = sizes[i];

                fonts[i].address = &sharedmem_addr[offsets[i]];
            }
        }
    }

    return rc;
}
