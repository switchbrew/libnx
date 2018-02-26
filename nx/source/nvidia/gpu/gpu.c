#include <switch.h>

Result nvgpuCreate(NvGpu* g)
{
    Result rc;

    rc = nvchannelCreate(&g->gpu_channel, "/dev/nvhost-gpu");

    if (R_SUCCEEDED(rc))
        rc = nvasCreate(&g->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvasReserveFull(&g->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvasBindToChannel(&g->addr_space, &g->gpu_channel);

    if (R_FAILED(rc))
        nvgpuClose(g);

    return rc;
}

void nvgpuClose(NvGpu* g)
{
    nvasClose(&g->addr_space);
    nvchannelClose(&g->gpu_channel);
}
