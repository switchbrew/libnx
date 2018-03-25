enum {
    NvReg3D_ClearColor = 0x360,
    NvReg3D_ScreenScissorHorizontal = 0x3fd,
    NvReg3D_ScreenScissorVertical = 0x3fe,
    NvReg3D_RenderTargetControl = 0x487,
    NvReg3D_RenderTargetNAddr = 0x200,
    NvReg3D_RenderTargetNHorizontal = 0x202,
    NvReg3D_RenderTargetNVertical = 0x203,
    NvReg3D_RenderTargetNFormat = 0x204,
    NvReg3D_RenderTargetNTileMode = 0x205,
    NvReg3D_RenderTargetNArrayMode = 0x206,
    NvReg3D_RenderTargetNLayerStride = 0x207,
    NvReg3D_RenderTargetNBaseLayer = 0x208,
    NvReg3D_MultisampleEnable = 0x54d,
    NvReg3D_ClearBufferTrigger = 0x674,
};

void nvCmdsClearColor(NvCmdList* cmds, float colors[4]);
