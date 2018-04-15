#include <switch.h>
#include <string.h>

void vnSetRenderTargets(Vn* vn, VnRenderTargetConfig* targets, size_t num_targets) {
    size_t i;

    vnAddCmd(vn, NvIncr(0, NvReg3D_RenderTargetControl, (076543210 << 4) | num_targets));

    for (i=0; i<num_targets; i++) {
        iova_t gpu_addr = nvBufferGetGpuAddr(targets[i].color_buffer);

        vnAddCmd(
            vn,
            NvIncr(0,
                NvReg3D_RenderTargetAddr(i),
                gpu_addr >> 32,
                gpu_addr,
                targets[i].width,
                targets[i].height,
                targets[i].format,
                0,
                0,
                targets[i].width, // TODO: Round up to power of 2?
                0
            )
        );
    }
}
