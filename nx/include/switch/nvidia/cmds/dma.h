enum {
    NvRegDma_Launch = 0xC0,
    NvRegDma_SourceAddr = 0x100,
    NvRegDma_DestinationAddr = 0x102,
    NvRegDma_SourcePitch = 0x104,
    NvRegDma_DestinationPitch = 0x105,
    NvRegDma_Count = 0x106,
    /*
    0x1C0 MemsetValue? 1 uint
    0x1C2 MemsetControl? 1 bitfield Seen: 0x34444
    0x1C4 MemsetLength? 1 uint In units of 4 bytes. 
    */
};
