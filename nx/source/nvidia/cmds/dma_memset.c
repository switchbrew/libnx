#include <switch.h>
#include <string.h>

void vnDmaClear32(Vn* vn, iova_t dst, u32 val, size_t size) {
    vnAddCmd(
        vn,
        NvIncr(4, NvRegDma_RemapConstant, val),
        NvIncr(4, NvRegDma_RemapControl, 0x30000 | (NvRegDmaRemapValue_Constant0 * 0x1111)),
        NvIncr(4, NvRegDma_SourceAddr, dst>>32, dst, dst>>32, dst),
        NvIncr(4, NvRegDma_DestinationWidth, size/4, 1),
        NvIncr(4, NvRegDma_Count, size),
        NvIncr(4, NvRegDma_Launch, 0x586)
    );
}
