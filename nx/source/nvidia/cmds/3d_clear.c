#include <switch.h>
#include <string.h>

void vnClearBuffer(
    Vn* vn, NvBuffer* buf, u32 width, u32 height, float colors[4])
{
    vnAddCmd(vn,
          NvIncr(0, NvReg3D_ClearColor, f2i(colors[0]), f2i(colors[1]), f2i(colors[2]), f2i(colors[3])),
          NvIncr(0, NvReg3D_ScreenScissorHorizontal, 0 | (width << 16), 0 | (height << 16)),
          NvIncr(0, NvReg3D_RenderTargetControl, (076543210 << 4) | 1)); // bit0 probably enables RT #0

    iova_t gpu_addr = nvBufferGetGpuAddr(buf);
    vnAddCmd(vn,
          NvIncr(0, NvReg3D_RenderTargetNAddr + 0x10*0,
            gpu_addr >> 32, gpu_addr,
            width, height,
            0xc2,   /* Format */
            0x1000, /* TileMode */
            1,      /* ArrayMode */
            0,      /* Stride */
            0       /* BaseLayer */
        ));

    // Disable zeta + multisample
    vnAddCmd(vn, NvImm(0, 0x54E, 0), NvImm(0, 0x54D, 0));

    // Only layer 0.
    int z;
    for (z=0; z<1; z++)
        vnAddCmd(vn, NvImm(0, NvReg3D_ClearBufferTrigger, 0x3c | (z << 10)));
}
