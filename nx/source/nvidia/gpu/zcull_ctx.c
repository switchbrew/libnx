#include <switch.h>

Result nvzcullCreate(NvZcullContext* z, NvGpu* parent)
{
    Result rc;

    z->parent = parent;
    rc = nvbufCreateRw(&z->ctx_buf, nvinfoGetZcullCtxSize(), 0x1000, NvBufferKind_Pitch);

    iova_t iova_out;

    if (R_SUCCEEDED(rc))
        rc = nvasMapBuffer(&parent->addr_space, &z->ctx_buf, &iova_out);

    if (R_SUCCEEDED(rc))
        rc = nvioctlChannel_ZCullBind(parent->gpu_channel.fd, iova_out, NvZcullConfig_SeparateBuffer);

    return rc;
}

void nvzcullClose(NvZcullContext* z) {
    // TODO: Unmap z->ctx_buf from parent->addr_space?
    nvbufFree(&z->ctx_buf);
}
