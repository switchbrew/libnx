#include <string.h>
#include "types.h"
#include "result.h"
#include "services/vi.h"
#include "services/fatal.h"
#include "display/binder.h"
#include "display/buffer_producer.h"
#include "display/native_window.h"
#include "nvidia/graphic_buffer.h"

static ViDisplay g_viDisplay;
static ViLayer g_viLayer;
static NWindow g_defaultWin;

NWindow* nwindowGetDefault(void)
{
    return &g_defaultWin;
}

void __nx_win_init(void)
{
    Result rc;
    rc = viInitialize(ViServiceType_Default);
    if (R_SUCCEEDED(rc)) {
        rc = viOpenDefaultDisplay(&g_viDisplay);
        if (R_SUCCEEDED(rc)) {
            rc = viCreateLayer(&g_viDisplay, &g_viLayer);
            if (R_SUCCEEDED(rc)) {
                rc = viSetLayerScalingMode(&g_viLayer, ViScalingMode_FitToLayer);
                if (R_SUCCEEDED(rc)) {
                    rc = nwindowCreateFromLayer(&g_defaultWin, &g_viLayer);
                    if (R_SUCCEEDED(rc))
                        nwindowSetDimensions(&g_defaultWin, 1280, 720);
                }
                if (R_FAILED(rc))
                    viCloseLayer(&g_viLayer);
            }
            if (R_FAILED(rc))
                viCloseDisplay(&g_viDisplay);
        }
        if (R_FAILED(rc))
            viExit();
    }
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGfxInit));
}

void __nx_win_exit(void)
{
    nwindowClose(&g_defaultWin);
    viCloseLayer(&g_viLayer);
    viCloseDisplay(&g_viDisplay);
    viExit();
}
