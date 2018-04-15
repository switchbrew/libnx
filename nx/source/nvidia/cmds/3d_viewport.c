#include <switch.h>
#include <string.h>

void vnSetViewport(Vn* vn, size_t index, VnViewportConfig* c) {
    vnAddCmd(
        vn,
        NvIncr(
            0,
            NvReg3D_ViewportScaleX(index),
            c->scale[0], // ScaleX
            c->scale[1], // ScaleY
            c->scale[2], // ScaleZ
            c->translate[0], // TranslateX
            c->translate[1], // TranslateY
            c->translate[2], // TranslateZ
            0, // Swizzles
            0  // SubpixelPrecisionBias
        ),
        NvIncr(
            0,
            NvReg3D_ViewportHorizontal(index),
            (c->window.w << 16) | c->window.x,
            (c->window.h << 16) | c->window.y,
            c->near,
            c->far
        )
    );
}
