#include <switch.h>
#include <string.h>

void nvCmdsFifoInit(NvCmdList* cmds) {
    NvCmd(cmds, NvIncr(0, NvCmdCommon_BindObject, NvClassNumber_3D));
    NvCmd(cmds, NvIncr(1, NvCmdCommon_BindObject, NvClassNumber_Compute));
    NvCmd(cmds, NvIncr(2, NvCmdCommon_BindObject, NvClassNumber_Kepler));
    NvCmd(cmds, NvIncr(3, NvCmdCommon_BindObject, NvClassNumber_2D));
    NvCmd(cmds, NvIncr(4, NvCmdCommon_BindObject, NvClassNumber_DMA));
}
