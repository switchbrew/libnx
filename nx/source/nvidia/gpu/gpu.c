#include <switch.h>

Result nvgpuCreate(NvGpu* g)
{
    Result rc;

    if (R_FAILED(nvinfoInit()))
        return MAKERESULT(Module_Libnx, LibnxError_NvinfoFailedToInitialize);

    if (R_FAILED(nvbufInit()))
        return MAKERESULT(Module_Libnx, LibnxError_NvbufFailedToInitialize);

    rc = nvchannelCreate(&g->gpu_channel, "/dev/nvhost-gpu");

    if (R_SUCCEEDED(rc))
        rc = nvasCreate(&g->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvasReserveFull(&g->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvasBindToChannel(&g->addr_space, &g->gpu_channel);

    if (R_SUCCEEDED(rc))
        rc = nvchannelSetNvmapFd(&g->gpu_channel);

    if (R_SUCCEEDED(rc))
        rc = nvfifoCreate(&g->gpfifo, &g->gpu_channel);

    if (R_SUCCEEDED(rc))
        rc = nvzcullCreate(&g->zcull_ctx, g);

    if (R_FAILED(rc))
        nvgpuClose(g);

    return rc;
}

void nvgpuClose(NvGpu* g)
{
    nvbufExit();
    nvinfoExit();

    nvfifoClose(&g->gpfifo);
    nvasClose(&g->addr_space);
    nvchannelClose(&g->gpu_channel);
}
