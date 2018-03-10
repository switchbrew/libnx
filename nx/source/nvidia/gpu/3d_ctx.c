#include <switch.h>

Result nv3dCreate(Nv3dContext* t, NvGpu* parent)
{
    t->parent = parent;

    // TODO: Get class number from nvinfo*().
    return nvioctlChannel_AllocObjCtx(parent->gpu_channel.fd, 0xB197, 0, &t->obj_id);
}

void nv3dClose(Nv3dContext* t) {
    // Empty
}
