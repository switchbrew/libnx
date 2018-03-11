#include <switch.h>

Result nv3DContextCreate(Nv3DContext* t, NvGpu* parent)
{
    t->parent = parent;
    return nvioctlChannel_AllocObjCtx(parent->gpu_channel.fd, NvClassNumber_3D, 0, &t->obj_id);
}

void nv3DContextClose(Nv3DContext* t) {
    // Empty
}
