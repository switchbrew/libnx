#include <string.h>
#include <switch.h>

static bool g_gfxInitialized = 0;
static viDisplay g_gfxDisplay;
static viLayer g_gfxLayer;
static u8 g_gfxNativeWindow[0x100];
static u64 g_gfxNativeWindow_Size;
static s32 g_gfxNativeWindow_ID;
static binderSession g_gfxBinderSession;

static Result _gfxGetNativeWindowID(u8 *buf, u64 size, s32 *out_ID) {
    u32 *bufptr = (u32*)buf;

    //Validate ParcelData{Size|Offset}.
    if((u64)bufptr[1] >= size || (u64)bufptr[0] >= size || ((u64)bufptr[1])+((u64)bufptr[0]) >= size) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);
    if(bufptr[0] < 0xc) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);

    //bufptr = start of ParcelData
    bufptr = (u32*)&buf[bufptr[1]];

    *out_ID = (s32)bufptr[2];

    return 0;
}

static Result _gfxInit(viServiceType servicetype, const char *DisplayName, u32 LayerFlags, u64 LayerId) {
    Result rc=0;

    if(g_gfxInitialized)return 0;

    g_gfxNativeWindow_ID = 0;

    rc = viInitialize(servicetype);
    if (R_FAILED(rc)) return rc;

    rc = viOpenDisplay(DisplayName, &g_gfxDisplay);

    if (R_SUCCEEDED(rc)) rc = viOpenLayer(g_gfxNativeWindow, &g_gfxNativeWindow_Size, &g_gfxDisplay, &g_gfxLayer, LayerFlags, LayerId);

    if (R_SUCCEEDED(rc)) rc = _gfxGetNativeWindowID(g_gfxNativeWindow, g_gfxNativeWindow_Size, &g_gfxNativeWindow_ID);

    if (R_SUCCEEDED(rc)) {
        binderCreateSession(&g_gfxBinderSession, viGetSession_IHOSBinderDriverRelay(), g_gfxNativeWindow_ID);
        rc = binderInitSession(&g_gfxBinderSession, 0x0f);
    }

    //TODO: Send binder parcels.

    if (R_FAILED(rc)) {
        binderExitSession(&g_gfxBinderSession);
        viCloseLayer(&g_gfxLayer);
        viCloseDisplay(&g_gfxDisplay);
        viExit();
    }

    if (R_SUCCEEDED(rc)) g_gfxInitialized = 1;

    return rc;
}

void gfxInitDefault(void) {
    Result rc = _gfxInit(VILAYERFLAGS_Default, "Default", VILAYERFLAGS_Default, 0);
    if (R_FAILED(rc)) fatalSimple(rc);
}

void gfxExit(void) {
    if(!g_gfxInitialized)return;

    binderExitSession(&g_gfxBinderSession);

    viCloseLayer(&g_gfxLayer);
    viCloseDisplay(&g_gfxDisplay);

    viExit();

    g_gfxInitialized = 0;
    g_gfxNativeWindow_ID = 0;
}

