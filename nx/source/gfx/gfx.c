#include <string.h>
#include <switch.h>

static bool g_gfxInitialized = 0;
static viDisplay g_gfxDisplay;
static viLayer g_gfxLayer;
static u8 g_gfxNativeWindow[0x100];
static u64 g_gfxNativeWindow_Size;

static Result _gfxInit(viServiceType servicetype, const char *DisplayName, u32 LayerFlags, u64 LayerId) {
    Result rc=0;

    if(g_gfxInitialized)return 0;

    rc = viInitialize(servicetype);
    if (R_FAILED(rc)) return rc;

    rc = viOpenDisplay(DisplayName, &g_gfxDisplay);

    if (R_SUCCEEDED(rc)) rc = viOpenLayer(g_gfxNativeWindow, &g_gfxNativeWindow_Size, &g_gfxDisplay, &g_gfxLayer, LayerFlags, LayerId);

    if (R_FAILED(rc)) {
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

    viCloseLayer(&g_gfxLayer);
    viCloseDisplay(&g_gfxDisplay);

    viExit();

    g_gfxInitialized = 0;
}

