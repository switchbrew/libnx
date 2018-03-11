#include <switch.h>

Result nvgpuCreate(NvGpu* g)
{
    Result rc;

    if (R_FAILED(nvInfoInit()))
        return MAKERESULT(Module_Libnx, LibnxError_NvinfoFailedToInitialize);

    if (R_FAILED(nvBufferInit())) {
        nvInfoExit();
        return MAKERESULT(Module_Libnx, LibnxError_NvbufFailedToInitialize);
    }

    rc = nvChannelCreate(&g->gpu_channel, "/dev/nvhost-gpu");

    if (R_SUCCEEDED(rc))
        rc = nvAddressSpaceCreate(&g->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvAddressSpaceReserveFull(&g->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvAddressSpaceBindToChannel(&g->addr_space, &g->gpu_channel);

    if (R_SUCCEEDED(rc))
        rc = nvChannelSetNvmapFd(&g->gpu_channel);

    if (R_SUCCEEDED(rc))
        rc = nvGpfifoCreate(&g->gpfifo, &g->gpu_channel);

    if (R_SUCCEEDED(rc))
        rc = nv3DContextCreate(&g->_3d_ctx, g);

    if (R_SUCCEEDED(rc))
        rc = nvErrorNotifierCreate(&g->error_notifier, g);

    if (R_SUCCEEDED(rc))
        rc = nvChannelSetPriority(&g->gpu_channel, NvChannelPriority_Medium);

    if (R_SUCCEEDED(rc))
        rc = nvZcullContextCreate(&g->zcull_ctx, g);

    if (R_FAILED(rc))
        nvgpuClose(g);

    return rc;
}

void nvgpuClose(NvGpu* g)
{
    nvBufferExit();
    nvInfoExit();

    nvErrorNotifierClose(&g->error_notifier);
    nvZcullContextClose(&g->zcull_ctx);
    nv3DContextClose(&g->_3d_ctx);
    nvGpfifoClose(&g->gpfifo);
    nvAddressSpaceClose(&g->addr_space);
    nvChannelClose(&g->gpu_channel);
}
