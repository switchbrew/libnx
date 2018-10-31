#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "kernel/event.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/buffer.h"
#include "nvidia/address_space.h"
#include "nvidia/channel.h"
#include "nvidia/fence.h"
#include "nvidia/info.h"
#include "nvidia/gpu/cmd_list.h"
#include "nvidia/gpu/gpfifo.h"
#include "nvidia/gpu/zcull_ctx.h"
#include "nvidia/gpu/3d_ctx.h"
#include "nvidia/gpu/error_notifier.h"
#include "nvidia/gpu/gpu.h"

Result nvGpuCreate(NvGpu* g)
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
        rc = nvAddressSpaceCreate(&g->addr_space, nvInfoGetGpuCharacteristics()->big_page_size);

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
        nvGpuClose(g);

    return rc;
}

void nvGpuClose(NvGpu* g)
{
    nvErrorNotifierClose(&g->error_notifier);
    nvChannelClose(&g->gpu_channel);
    nvZcullContextClose(&g->zcull_ctx);
    nv3DContextClose(&g->_3d_ctx);
    nvGpfifoClose(&g->gpfifo);
    nvAddressSpaceClose(&g->addr_space);

    nvBufferExit();
    nvInfoExit();
}
