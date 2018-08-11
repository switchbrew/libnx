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
#include "nvidia/gpu/cmd_list.h"
#include "nvidia/gpu/gpfifo.h"
#include "nvidia/gpu/zcull_ctx.h"
#include "nvidia/gpu/3d_ctx.h"
#include "nvidia/gpu/error_notifier.h"
#include "nvidia/gpu/gpu.h"

Result nv3DContextCreate(Nv3DContext* t, NvGpu* parent)
{
    t->parent = parent;

    return nvioctlChannel_AllocObjCtx(
        parent->gpu_channel.fd, NvClassNumber_3D, 0, &t->obj_id);
}

void nv3DContextClose(Nv3DContext* t) {
    // Empty
}
