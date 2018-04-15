#include <switch.h>
#include <string.h>

#define DMA_MAXCOPY 0x3FFFFF

void vnDmaCopy(Vn* vn, iova_t dst, iova_t src, size_t num) {
    while (num) {
        size_t part = num;

        if (part >= DMA_MAXCOPY)
            part = DMA_MAXCOPY;

        vnAddCmd(
            vn,
            NvIncr(
                4, NvRegDma_SourceAddr,
                src>>32, src,
                dst>>32, dst,
                1, 1,
                part
            ),
            NvImm(
                4, NvRegDma_Launch,
                0x186
            )
        );

        num -= part;
    }
}
