enum {
    NvRegDma_Launch = 0x0c0,
    NvRegDma_SourceAddr = 0x100,
    NvRegDma_DestinationAddr = 0x102,
    NvRegDma_SourcePitch = 0x104,
    NvRegDma_DestinationPitch = 0x105,
    NvRegDma_Count = 0x106,
    NvRegDma_RemapConstant = 0x1c0,
    NvRegDma_RemapControl = 0x1c2,
    NvRegDma_DestinationWidth = 0x1c4,
    NvRegDma_DestinationHeight = 0x1c5,
};

enum {
    NvRegDmaRemapValue_SourceX = 0,
    NvRegDmaRemapValue_SourceY = 1,
    NvRegDmaRemapValue_SourceZ = 2,
    NvRegDmaRemapValue_SourceW = 3,
    NvRegDmaRemapValue_Constant0 = 4,
    NvRegDmaRemapValue_Constant1 = 5,
};
