#include <switch.h>
#include <string.h>

void nvCmds3DInitialize(NvCmdList* cmds) {
    NvCmd(
        cmds,
        // ???
        NvIncr(0, 0xd1a, 0, 0xffffffff),
        NvImm(0, 0xd19, 0),
        // Reset multisampling
        NvImm(0, NvReg3D_MultisampleEnable, 0),
        NvImm(0, NvReg3D_MultisampleCsaaEnable, 0),
        NvImm(0, NvReg3D_MultisampleMode, 0),
        NvImm(0, NvReg3D_MultisampleControl, 0),
        // ???
        NvImm(0, 0x433, 4),
        NvImm(0, 0x438, 0xff),
        NvImm(0, 0x439, 0xff),
        NvImm(0, 0x43b, 0xff),
        NvImm(0, 0x43c, 4),
        NvImm(0, 0x1d3, 0x3f),
        //
        NvIncr(0, NvReg3D_ClipRectNHorizontal, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        NvImm(0, NvReg3D_ClearFlags, 0x101));

    size_t i;
    for (i=0; i<16; i++)
        NvCmd(cmds, NvImm(0, NvReg3D_ScissorEnable(i), 1));

    NvCmd(cmds, NvImm(0, NvReg3D_PrimRestartWithDrawArrays, 1),
          NvImm(0, NvReg3D_PointRasterRules, 0),
          NvImm(0, NvReg3D_LinkedTsc, 0),
          NvImm(0, NvReg3D_ProvokingVertexLast, 1),
          // ???
          NvImm(0, 0x54a, 0),
          NvImm(0, 0x400, 0x10),
          NvImm(0, 0x86, 0x10),
          NvImm(0, 0x43f, 0x10),
          NvImm(0, 0x4a4, 0x10),
          NvImm(0, 0x4b6, 0x10),
          NvImm(0, 0x4b7, 0x10),
          //
          NvImm(0, NvReg3D_CallLimitLog, 8),
          // ???
          NvImm(0, 0x450, 0x10),
          NvImm(0, 0x584, 0xe));

    for (i=0; i<16; i++) {
        NvCmd(cmds, NvImm(0, NvReg3D_VertexStreamEnableDivisor(i), 0));
    }

    NvCmd(
        cmds,
        NvImm(0, NvReg3D_VertexIdGenMode, 0),
        NvImm(0, NvReg3D_ZcullStatCtrsEnable, 1),
        NvImm(0, NvReg3D_LineWidthSeparate, 1),
        // ???
        NvImm(0, 0xc3, 0),
        NvImm(0, 0xc0, 3),
        NvImm(0, 0x3f7, 1),
        NvImm(0, 0x670, 1),
        NvImm(0, 0x3e3, 0),
        NvImm(0, NvReg3D_StencilTwoSideEnable, 1),
        NvImm(0, NvReg3D_TextureConstBufferIndex, 2),
        NvImm(0, 0xc4, 0x503),
        NvIncr(0, NvReg3D_LocalBase, 0x01000000),
        NvImm(0, 0x44c, 0x13),
        NvImm(0, 0xdd, 0),
        NvIncr(0, NvReg3D_Layer, 0x10000),
        NvImm(0, 0x488, 5),
        NvIncr(0, 0x514, 0x00800008),
        NvImm(0, 0xab, 3),
        NvImm(0, 0xa4, 0),
        NvImm(0, 0x221, 0x3f));

    // TODO: Call some macro shit (0xe16).

    NvCmd(
        cmds,
        // Reset Zcull.
        NvImm(0, NvReg3D_ZcullTestMask, 0),
        NvImm(0, 0x65a, 0x11),
        NvImm(0, NvReg3D_ZcullRegion, 0),
        NvIncr(0, 0x054, 0x49000000, 0x49000001),
        NvIncr(0, 0xd18, 0x05000500),
        );

    // TODO: Call some macro shit (0xe34)

    // TODO: Fill in NvReg3D_VertexRunoutAddr with a valid addr.

    // TODO: Call some macro shit (0xe2a)

    // TODO: CB_DATA stuff

    // TODO: Call some macro shit (0xe32)

    // TODO: CB_DATA stuff

    // TODO: CB_BIND stuff
}

void nvCmdsClearBuffer(
    NvCmdList* cmds, NvBuffer* buf, u32 width, u32 height, float colors[4])
{
    NvCmd(cmds, NvIncr(0, NvReg3D_ClearColor,
        f2i(colors[0]), f2i(colors[1]), f2i(colors[2]), f2i(colors[3])));
    NvCmd(cmds, NvIncr(0, NvReg3D_ScreenScissorHorizontal,
                       0 | (0x100 << 16), 0 | (0x100 << 16)));
    NvCmd(cmds, NvImm(0, NvReg3D_RenderTargetControl, 1)); // bit0 probably enables RT #0

    iova_t gpu_addr = nvBufferGetGpuAddr(buf);
    NvCmd(cmds, NvIncr(NvReg3D_RenderTargetNAddr + 0x10*0,
                       gpu_addr >> 32, gpu_addr,
                       width, height,
                       0xfe /* Format */,
                       0x1000 /* TileMode */,
                       1 /* ArrayMode */,
                       0 /* Stride */,
                       0 /* BaseLayer */
              ));
    int z;
    for (z=0; z<32; z++)
        NvCmd(cmds, NvImm(0, NvReg3D_ClearBufferTrigger, 0x3c | (z << 10)));
    /*
      TODO:
    IMMED_NVC0(push, NVC0_3D(ZETA_ENABLE), 0);
    IMMED_NVC0(push, NVC0_3D(MULTISAMPLE_MODE), 0);
    */
    
}
