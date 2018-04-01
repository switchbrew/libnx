enum {
    NvReg3D_LocalBase = 0x1df,
    NvReg3D_RenderTargetNAddr = 0x80,
    NvReg3D_RenderTargetNHorizontal = 0x82,
    NvReg3D_RenderTargetNVertical = 0x83,
    NvReg3D_LineWidthSeparate = 0x83,
    NvReg3D_RenderTargetNFormat = 0x84,
    NvReg3D_RenderTargetNTileMode = 0x85,
    NvReg3D_RenderTargetNArrayMode = 0x86,
    NvReg3D_RenderTargetNLayerStride = 0x87,
    NvReg3D_RenderTargetNBaseLayer = 0x88,
    NvReg3D_ClipRectNHorizontal = 0x340,
    NvReg3D_ClipRectNVertical = 0x341,
    NvReg3D_CallLimitLog = 0x359,
    NvReg3D_ClearColor = 0x360,
    NvReg3D_PrimRestartWithDrawArrays = 0x37a,
    NvReg3D_ScissorNEnable = 0x380,
    NvReg3D_ScissorNHorizontal = 0x381,
    NvReg3D_ScissorNVertical = 0x382,
    NvReg3D_VertexRunoutAddr = 0x3e1,
    NvReg3D_ScreenScissorHorizontal = 0x3fd,
    NvReg3D_ScreenScissorVertical = 0x3fe,
    NvReg3D_ClearFlags = 0x43e,
    NvReg3D_RenderTargetControl = 0x487, 
    NvReg3D_LinkedTsc = 0x48d,
    NvReg3D_ZcullStatCtrsEnable = 0x547,
    NvReg3D_MultisampleEnable = 0x54d,
    NvReg3D_MultisampleControl = 0x54f,
    NvReg3D_ZcullRegion = 0x564,
    NvReg3D_StencilTwoSideEnable = 0x565,
    NvReg3D_MultisampleCsaaEnable = 0x56d,
    NvReg3D_Layer = 0x573,
    NvReg3D_MultisampleMode = 0x574,
    NvReg3D_VertexIdGenMode = 0x593,
    NvReg3D_PointRasterRules = 0x597,
    NvReg3D_ProvokingVertexLast = 0x5a1,
    NvReg3D_VertexStreamNEnableDivisor = 0x620,
    NvReg3D_ZcullTestMask = 0x65b,
    NvReg3D_ClearBufferTrigger = 0x674,
    NvReg3D_TextureConstBufferIndex = 0x982,
};

#define NvReg3D_ClipRectHorizontal(n) \
    ((NvReg3D_ClipRectNHorizontal) + 2*(n))
#define NvReg3D_ClipRectVertical(n) \
    ((NvReg3D_ClipRectNVertical) + 2*(n))

#define NvReg3D_ScissorEnable(n) \
    ((NvReg3D_ScissorNEnable) + 4*(n))

#define NvReg3D_VertexStreamEnableDivisor(n) \
    ((NvReg3D_VertexStreamNEnableDivisor) + (n))
