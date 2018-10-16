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

Result nvZcullContextCreate(NvZcullContext* z, NvGpu* parent)
{
    Result rc;

    z->parent = parent;

    rc = nvBufferCreate(
        &z->ctx_buf, nvInfoGetZcullCtxSize(), 0x20000, NvKind_Pitch, false, true,
        &parent->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvioctlChannel_ZCullBind(
            parent->gpu_channel.fd, nvBufferGetGpuAddr(&z->ctx_buf),
            NvZcullConfig_SeparateBuffer);

    return rc;
}

void nvZcullContextClose(NvZcullContext* z) {
    nvBufferFree(&z->ctx_buf);
}
