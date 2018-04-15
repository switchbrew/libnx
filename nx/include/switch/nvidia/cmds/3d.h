enum {
    NvReg3D_MmeShadowRamControl = 0x49,
    NvReg3D_LineWidthSeparate = 0x83,
    NvReg3D_LocalBase = 0x1df,
    NvReg3D_RenderTargetNAddr = 0x200,
    NvReg3D_RenderTargetNHorizontal = 0x202,
    NvReg3D_RenderTargetNVertical = 0x203,
    NvReg3D_RenderTargetNFormat = 0x204,
    NvReg3D_RenderTargetNTileMode = 0x205,
    NvReg3D_RenderTargetNArrayMode = 0x206,
    NvReg3D_RenderTargetNLayerStride = 0x207,
    NvReg3D_RenderTargetNBaseLayer = 0x208,
    NvReg3D_ViewportNScaleX = 0x280,
    NvReg3D_ViewportNScaleY = 0x281,
    NvReg3D_ViewportNScaleZ = 0x282,
    NvReg3D_ViewportNTranslateX = 0x283,
    NvReg3D_ViewportNTranslateY = 0x284,
    NvReg3D_ViewportNTranslateZ = 0x285,
    NvReg3D_ViewportNSwizzles = 0x286,
    NvReg3D_ViewportNSubpixelPrecisionBias = 0x287,
    NvReg3D_ViewportNHorizontal= 0x300,
    NvReg3D_ViewportNVertical= 0x301,
    NvReg3D_ViewportNDepthRangeNear= 0x302,
    NvReg3D_ViewportNDepthRangeFar= 0x303,
    NvReg3D_ClipRectNHorizontal = 0x340,
    NvReg3D_ClipRectNVertical = 0x341,
    NvReg3D_CallLimitLog = 0x359,
    NvReg3D_ClearColor = 0x360,
    NvReg3D_PrimRestartWithDrawArrays = 0x37a,
    NvReg3D_ScissorNEnable = 0x380,
    NvReg3D_ScissorNHorizontal = 0x381,
    NvReg3D_ScissorNVertical = 0x382,
    NvReg3D_VertexRunoutAddr = 0x3e1,
    NvReg3D_MultisampleRasterEnable = 0x3ed,
    NvReg3D_MsaaMask = 0x3ef,
    NvReg3D_CoverageModulationEnable = 0x3f6,
    NvReg3D_ScreenScissorHorizontal = 0x3fd,
    NvReg3D_ScreenScissorVertical = 0x3fe,
    NvReg3D_ClearFlags = 0x43e,
    NvReg3D_ConservativeRaster = 0x452,
    NvReg3D_MultisampleCoverageToColor = 0x47e,
    NvReg3D_RenderTargetControl = 0x487, 
    NvReg3D_ZetaArrayMode = 0x48c,
    NvReg3D_LinkedTsc = 0x48d,
    NvReg3D_BlendIndependent = 0x4b9,
    NvReg3D_ScreenHorizontalControl = 0x4be,
    NvReg3D_ClipDistanceEnable = 0x544,
    NvReg3D_SampleCountEnable = 0x545,
    NvReg3D_ZcullStatCtrsEnable = 0x547,
    NvReg3D_PointSpriteEnable = 0x548,
    NvReg3D_MultisampleEnable = 0x54d,
    NvReg3D_MultisampleControl = 0x54f,
    NvReg3D_ZcullRegion = 0x564,
    NvReg3D_StencilTwoSideEnable = 0x565,
    NvReg3D_MultisampleCsaaEnable = 0x56d,
    NvReg3D_Layer = 0x573,
    NvReg3D_MultisampleMode = 0x574,
    NvReg3D_EdgeFlag = 0x579,
    NvReg3D_PointCoordReplace = 0x581,
    NvReg3D_CodeAddr = 0x582,
    NvReg3D_VertexIdGenMode = 0x593,
    NvReg3D_PointRasterRules = 0x597,
    NvReg3D_ProvokingVertexLast = 0x5a1,
    NvReg3D_IndexArrayLimit = 0x5f4,
    NvReg3D_VertexStreamNEnableDivisor = 0x620,
    NvReg3D_VpPointSize = 0x644,
    NvReg3D_ZcullTestMask = 0x65b,
    NvReg3D_ClearBufferTrigger = 0x674,
    NvReg3D_ViewportTransformEnable = 0x64b,
    NvReg3D_ViewportControl = 0x64f,

    NvReg3D_ConstantBufferSize = 0x8e0,
    NvReg3D_ConstantBufferAddr = 0x8e1,
    NvReg3D_ConstantBufferLoadOffset = 0x8e3,
    NvReg3D_ConstantBufferLoadN = 0x8e3,
    NvReg3D_TextureConstantBufferIndex = 0x982,
    NvReg3D_ConstantBufferBindN = 0x904,
    NvReg3D_MmeShadowScratchN = 0xd00,
};

#define NvReg3D_ViewportScaleX(n) \
    (NvReg3D_ViewportNScaleX + 8*(n))
#define NvReg3D_ViewportScaleY(n) \
    (NvReg3D_ViewportNScaleY + 8*(n))
#define NvReg3D_ViewportScaleZ(n) \
    (NvReg3D_ViewportNScaleZ + 8*(n))
#define NvReg3D_ViewportTranslateX(n) \
    (NvReg3D_ViewportNTranslateX + 8*(n))
#define NvReg3D_ViewportTranslateY(n) \
    (NvReg3D_ViewportNTranslateY + 8*(n))
#define NvReg3D_ViewportTranslateZ(n) \
    (NvReg3D_ViewportNTranslateZ + 8*(n))
#define NvReg3D_ViewportSwizzles(n) \
    (NvReg3D_ViewportNSwizzles + 8*(n))
#define NvReg3D_ViewportSubpixelPrecisionBias(n) \
    (NvReg3D_ViewportNSubpixelPrecisionBias + 8*(n))

#define NvReg3D_RenderTargetAddr(n) \
    (NvReg3D_RenderTargetNAddr + 16*(n))

#define NvReg3D_ViewportHorizontal(n) \
    (NvReg3D_ViewportNHorizontal + 4*(n))
#define NvReg3D_ViewportVertical(n) \
    (NvReg3D_ViewportNVertical + 4*(n))
#define NvReg3D_ViewportDepthRangeNear(n) \
    (NvReg3D_ViewportNDepthRangeNear + 4*(n))
#define NvReg3D_ViewportDepthRangeFar(n) \
    (NvReg3D_ViewportNDepthRangeFar + 4*(n))

#define NvReg3D_ClipRectHorizontal(n) \
    (NvReg3D_ClipRectNHorizontal + 2*(n))
#define NvReg3D_ClipRectVertical(n) \
    (NvReg3D_ClipRectNVertical + 2*(n))

#define NvReg3D_ScissorEnable(n) \
    (NvReg3D_ScissorNEnable + 4*(n))
#define NvReg3D_ScissorHorizontal(n) \
    (NvReg3D_ScissorNHorizontal + 4*(n))
#define NvReg3D_ScissorVertical(n) \
    (NvReg3D_ScissorNVertical + 4*(n))

#define NvReg3D_VertexStreamEnableDivisor(n) \
    (NvReg3D_VertexStreamNEnableDivisor + (n))

#define NvReg3D_ConstantBufferBind(n) \
    (NvReg3D_ConstantBufferBindN + 8*(n))

#define NvReg3D_MmeShadowScratch(n) \
    (NvReg3D_MmeShadowScratchN + (n))
