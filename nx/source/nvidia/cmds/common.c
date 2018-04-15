#include <switch.h>
#include <string.h>

Result vnInit(Vn* vn, NvGpu* parent)
{
    Result rc;

    vn->parent = parent;

    rc = nvCmdListCreate(&vn->cmd_list, parent, 0x1000*4);

    if (R_FAILED(rc))
        return rc;

    vnAddCmd(vn,
        NvIncr(0, NvCmdCommon_BindObject, NvClassNumber_3D),
        NvIncr(1, NvCmdCommon_BindObject, NvClassNumber_Compute),
        NvIncr(2, NvCmdCommon_BindObject, NvClassNumber_Kepler),
        NvIncr(3, NvCmdCommon_BindObject, NvClassNumber_2D),
        NvIncr(4, NvCmdCommon_BindObject, NvClassNumber_DMA)
    );

    return rc;
}
