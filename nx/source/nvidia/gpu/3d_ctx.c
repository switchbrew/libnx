#include <switch.h>

Result nv3dCreate(Nv3dContext* t, NvGpu* parent)
{
    t->parent = parent;
    return 0;
}

void nv3dClose(Nv3dContext* t) {
    /**/
}
