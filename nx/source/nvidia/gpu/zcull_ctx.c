#include <switch.h>

Result nvZcullContextCreate(NvZcullContext* z, NvGpu* parent)
{
    Result rc;

    z->parent = parent;

    rc = nvBufferCreateRw(
        &z->ctx_buf, nvInfoGetZcullCtxSize(), 0x20000, NvBufferKind_Pitch,
        &parent->addr_space);

    if (R_SUCCEEDED(rc))
        rc = nvBufferMapAsTexture(&z->ctx_buf, NvBufferKind_Generic_16BX2);

    if (R_SUCCEEDED(rc))
        rc = nvioctlChannel_ZCullBind(
            parent->gpu_channel.fd, nvBufferGetGpuAddr(&z->ctx_buf),
            NvZcullConfig_SeparateBuffer);

    return rc;
}

void nvZcullContextClose(NvZcullContext* z) {
    // TODO: Unmap z->ctx_buf from parent->addr_space?
    nvBufferFree(&z->ctx_buf);
}
