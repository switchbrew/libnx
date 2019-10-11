#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include "kernel/shmem.h"
#include "services/pl.h"

#define SHAREDMEMFONT_SIZE 0x1100000

static Service g_plSrv;
static SharedMemory g_plSharedmem;

static Result _plGetSharedMemoryNativeHandle(Handle* handle_out);

NX_GENERATE_SERVICE_GUARD(pl);

Result _plInitialize(void) {
    Result rc=0;
    Handle sharedmem_handle=0;

    rc = smGetService(&g_plSrv, "pl:u");

    if (R_SUCCEEDED(rc)) {
        rc = _plGetSharedMemoryNativeHandle(&sharedmem_handle);

        if (R_SUCCEEDED(rc)) {
            shmemLoadRemote(&g_plSharedmem, sharedmem_handle, 0x1100000, Perm_R);

            rc = shmemMap(&g_plSharedmem);
        }
    }

    if (R_FAILED(rc)) plExit();

    return rc;
}

void _plCleanup(void) {
    serviceClose(&g_plSrv);
    shmemClose(&g_plSharedmem);
}

Service* plGetServiceSession(void) {
    return &g_plSrv;
}

void* plGetSharedmemAddr(void) {
    return shmemGetAddr(&g_plSharedmem);
}

static Result _plCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _plCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _plCmdInU32OutU32(Service* srv, u32 inval, u32 *out, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, inval, *out);
}

static Result _plRequestLoad(u32 SharedFontType) {
    return _plCmdInU32NoOut(&g_plSrv, SharedFontType, 0);
}

static Result _plGetLoadState(u32 SharedFontType, u32* LoadState) {
    return _plCmdInU32OutU32(&g_plSrv, SharedFontType, LoadState, 1);
}

static Result _plGetSize(u32 SharedFontType, u32* size) {
    return _plCmdInU32OutU32(&g_plSrv, SharedFontType, size, 2);
}

static Result _plGetSharedMemoryAddressOffset(u32 SharedFontType, u32* offset) {
    return _plCmdInU32OutU32(&g_plSrv, SharedFontType, offset, 3);
}

static Result _plGetSharedMemoryNativeHandle(Handle* handle_out) {
    return _plCmdGetHandle(&g_plSrv, handle_out, 4);
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

Result plGetSharedFontByType(PlFontData* font, PlSharedFontType SharedFontType) {
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

Result plGetSharedFont(u64 LanguageCode, PlFontData* fonts, s32 max_fonts, s32* total_fonts) {
    Result rc=0;
    u32 types[PlSharedFontType_Total]={0};
    u32 offsets[PlSharedFontType_Total]={0};
    u32 sizes[PlSharedFontType_Total]={0};
    size_t size = PlSharedFontType_Total*sizeof(u32);
    s32 font_count=0, i;
    u8* sharedmem_addr = (u8*)plGetSharedmemAddr();

    memset(fonts, 0, sizeof(PlFontData) * max_fonts);

    if (total_fonts) *total_fonts = 0;

    for (i=0; i<PlSharedFontType_Total; i++) {
        rc = _plRequestLoadWait(i);
        if (R_FAILED(rc)) return rc;
    }

    struct {
        u8 fonts_loaded;
        s32 total_fonts;
    } out;

    rc = serviceDispatchInOut(&g_plSrv, 5, LanguageCode, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { types, size },
            { offsets, size },
            { sizes, size },
        },
    );
    if (R_SUCCEEDED(rc) && out.fonts_loaded) {
        font_count = out.total_fonts;
        if (font_count > PlSharedFontType_Total) font_count = PlSharedFontType_Total;
        if (font_count > max_fonts) font_count = max_fonts;
        if (font_count < 0) font_count = 0;
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
    return rc;
}
