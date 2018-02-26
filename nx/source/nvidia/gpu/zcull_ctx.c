#include <switch.h>

Result nvzcullCreate(NvZcullContext* z, NvGpu* parent)
{
    z->parent = parent;
    return 0;
}

void nvzcullClose(NvZcullContext* z) {
    /**/
}
