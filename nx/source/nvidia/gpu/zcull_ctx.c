#include <switch.h>

Result nvZcullContextCreate(NvZcullContext* z, NvGpu* parent)
{
    Result rc;

    z->parent = parent;
    rc = nvBufferCreateRw(&z->ctx_buf, nvInfoGetZcullCtxSize(), 0x20000, NvBufferKind_Pitch);

    iova_t iova_out;

    if (R_SUCCEEDED(rc))
        rc = nvAddressSpaceMapBuffer(&parent->addr_space, &z->ctx_buf, NvBufferKind_Pitch, &iova_out);

    if (R_SUCCEEDED(rc))

        rc = nvAddressSpaceMapBuffer(&parent->addr_space, &z->ctx_buf, NvBufferKind_Generic_16BX2, /*&iova_out*/ NULL);

    if (R_SUCCEEDED(rc))
        rc = nvioctlChannel_ZCullBind(parent->gpu_channel.fd, iova_out, NvZcullConfig_SeparateBuffer);

    return rc;
}

void nvZcullContextClose(NvZcullContext* z) {
    // TODO: Unmap z->ctx_buf from parent->addr_space?
    nvBufferFree(&z->ctx_buf);
}
