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

Result nvCmdListCreate(NvCmdList* c, NvGpu* parent, size_t max_cmds)
{
    Result rc;

    rc = nvBufferCreate(
        &c->buffer, max_cmds * 4, 0x1000, NvKind_Pitch, false, false,
        &parent->addr_space);

    if (R_SUCCEEDED(rc)) {
        c->offset = 0;
        c->num_cmds = 0;
        c->max_cmds = max_cmds;
        c->parent = parent;
    }

    return rc;
}

void nvCmdListClose(NvCmdList* c) {
    nvBufferFree(&c->buffer);
}

iova_t nvCmdListGetGpuAddr(NvCmdList* c) {
    return nvBufferGetGpuAddr(&c->buffer);
}

u64 nvCmdListGetListSize(NvCmdList* c) {
    return c->num_cmds;
}

void nvCmdListReset(NvCmdList* c) {
    c->offset = 0;
    c->num_cmds = 0;
}

u32* nvCmdListInsert(NvCmdList* c, size_t num_cmds)
{
    // Has enough space?
    if ((c->offset + c->num_cmds + num_cmds) > c->max_cmds)
        return NULL;

    c->num_cmds += num_cmds;

    u32* list = (u32*) nvBufferGetCpuAddr(&c->buffer);
    return &list[c->offset + c->num_cmds - num_cmds];
}
