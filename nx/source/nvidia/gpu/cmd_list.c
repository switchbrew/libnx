#include <switch.h>

Result nvCmdListCreate(NvCmdList* c, NvGpu* parent, size_t max_cmds)
{
    Result rc;

    rc = nvBufferCreate(&c->buffer, max_cmds * 4, 0x1000, NvBufferKind_Pitch);

    if (R_SUCCEEDED(rc)) {
        nvBufferMakeCpuUncached(&c->buffer);

        rc = nvAddressSpaceMapBuffer(
            &parent->addr_space, &c->buffer, NvBufferKind_Pitch, &c->gpu_addr);

        c->num_cmds = 0;
        c->max_cmds = max_cmds;
        c->parent = parent;
    }

    return rc;
}

void nvCmdListClose(NvCmdList* c)
{
    // TODO: nvAddressSpaceUnmapBuffer?
    nvBufferMakeCpuCached(&c->buffer);
    nvBufferFree(&c->buffer);
}

iova_t nvCmdListGetGpuAddr(NvCmdList* c) {
    return c->gpu_addr;
}

u64 nvCmdListGetListSize(NvCmdList* c) {
    return c->num_cmds * 4;
}

u32* nvCmdListInsert(NvCmdList* c, size_t num_cmds)
{
    // Has enough space?
    if ((c->num_cmds + num_cmds) > c->max_cmds)
        return NULL;

    u32* ptr = (u32*) nvBufferGetAddr(&c->buffer);
    ptr += c->num_cmds;

    c->num_cmds += num_cmds;
    return ptr;
}
