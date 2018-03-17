#include <switch.h>

Result nvCmdListCreate(NvCmdList* c, NvGpu* parent, size_t max_cmds)
{
    Result rc;

    rc = nvBufferCreate(
        &c->buffer, max_cmds * 4, 0x1000, NvBufferKind_Pitch,
        &parent->addr_space);

    if (R_SUCCEEDED(rc)) {
        nvBufferMakeCpuUncached(&c->buffer);

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
    return c->num_cmds * 4;
}

u32* nvCmdListInsert(NvCmdList* c, size_t num_cmds)
{
    // Has enough space?
    if ((c->num_cmds + num_cmds) > c->max_cmds)
        return NULL;

    c->num_cmds += num_cmds;

    u32* list = (u32*) nvBufferGetCpuAddr(&c->buffer);
    return &list[c->num_cmds - num_cmds];
}
