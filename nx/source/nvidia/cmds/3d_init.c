#include <switch.h>
#include <string.h>

Result vnInit3D(Vn* vn) {
    Result rc;

    VnCmd(vn,
        // ???
        NvIncr(0, NvReg3D_MmeShadowScratch(0x1A), 0, 0xffffffff),
        NvImm(0, NvReg3D_MmeShadowScratch(0x19), 0),
        //
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
    for (i=0; i<16; i++) {
        VnCmd(vn, NvImm(0, NvReg3D_ScissorEnable(i), 1));
    }

    VnCmd(vn, NvImm(0, NvReg3D_PrimRestartWithDrawArrays, 1),
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
        VnCmd(vn, NvImm(0, NvReg3D_VertexStreamEnableDivisor(i), 0));
    }

    VnCmd(
        vn,
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
        NvImm(0, NvReg3D_TextureConstantBufferIndex, 2),
        NvImm(0, 0xc4, 0x503),
        NvIncr(0, NvReg3D_LocalBase, 0x01000000),
        NvImm(0, 0x44c, 0x13),
        NvImm(0, 0xdd, 0),
        NvIncr(0, NvReg3D_Layer, 0x10000),
        NvImm(0, 0x488, 5),
        NvIncr(0, 0x514, 0x00800008),
        //NvImm(0, 0xab, 3), // FAULTY
        //NvImm(0, 0xa4, 0),
        //NvImm(0, 0x221, 0x3f));
        );

    // TODO: Call macro_14f(0x00418800, 1, 1).
    // TODO: Call macro_14f(0x00419a08, 0, 0x10).
    // TODO: Call macro_14f(0x00419f78, 0, 8).
    // TODO: Call macro_14f(0x00404468, 0x07ffffff, 0x3fffffff).
    // TODO: Call macro_14f(0x00419a04, 1, 1).
    // TODO: Call macro_14f(0x00419a04, 2, 2).

    VnCmd(
        vn,
        // Reset Zcull.
        NvImm(0, 0x65a, 0x11),
        NvImm(0, NvReg3D_ZcullTestMask, 0),
        NvImm(0, NvReg3D_ZcullRegion, 0),
        NvIncr(0, 0x054, 0x49000000, 0x49000001),
        NvIncr(0, NvReg3D_MmeShadowScratch(0x18), 0x05000500)
        );

    // TODO: Call macro_21d(5, 0x00050000, 0x67);

    // TODO: Of what size is this buffer actually supposed to be?
    rc = nvBufferCreateRw(
        &vn->vertex_runout, 0x10000, 0x1000, 0, &vn->parent->addr_space);

    if (R_FAILED(rc))
        return rc;

    iova_t gpu_addr = nvBufferGetGpuAddr(&vn->vertex_runout);
    VnCmd(vn, NvIncr(0, NvReg3D_VertexRunoutAddr, gpu_addr >> 32, gpu_addr));

    // TODO: Call macro_206(0x194);

    // TODO: Write an addr(?) to low->0x8e4,high->0x8e5
    // TODO: Write 0 to 0x8e6, 0x8e7.

    // TODO: Call macro_226(5, 0x00056900, 0x100)
    // TODO: Call macro_226(5, 0x00056A00, 0x800)

    // Bind all const buffers index 0 to same buffer (of size 0x5f00).
    rc = nvBufferCreateRw(
        &vn->const_buffer0, 0x5f00 + 5*0x200, 0x1000, 0, &vn->parent->addr_space);

    if (R_FAILED(rc))
        return rc;

    gpu_addr = nvBufferGetGpuAddr(&vn->const_buffer0);

    VnCmd(vn,
        NvIncr(
            0, NvReg3D_ConstantBufferSize,
            0x5f00,
            gpu_addr >> 32, gpu_addr
        )
    );
    gpu_addr += 0x5f00;

    for (i=0; i<5; i++) {
        VnCmd(vn, NvImm(0, NvReg3D_ConstantBufferBind(i), 1));
    }

    // Bind const buffer index 2 to differnet buffers (each of size 0x200).
    for (i=0; i<5; i++) {
        VnCmd(vn,
            NvIncr(0,
                NvReg3D_ConstantBufferSize,
                0x5f00, // Size
                gpu_addr >> 32, gpu_addr // Addr
            ),
            NvImm(0, NvReg3D_ConstantBufferBind(i), 0x21),
            NvIncrOnce(
                0, NvReg3D_ConstantBufferLoadOffset, 0,
                0,1,2,3,4,5,6,7
            )
        );
        gpu_addr += 0x200;
    }

    VnCmd(vn,
        NvImm(0, NvReg3D_BlendIndependent, 1),
        NvImm(0, NvReg3D_EdgeFlag, 1),
        NvImm(0, NvReg3D_ViewportTransformEnable, 1),
        NvIncr(0, NvReg3D_ViewportControl, 0x181d) // ???
    );

    // NO issue.

    // Reset all the viewports.
    for (i=0; i<16; i++) {
        VnCmd(vn,
            NvIncr(0, NvReg3D_ViewportScaleX(i),
                f2i(0.5), /* ScaleX */
                f2i(0.5), /* ScaleY */
                f2i(0.5), /* ScaleZ */
                f2i(0.5), /* TranslateX */
                f2i(0.5), /* TranslateY */
                f2i(0.5)  /* TranslateZ */
            ),
            NvIncr(0, NvReg3D_ViewportHorizontal(i),
                0 | (1<<16), /* Horizontal */
                0 | (1<<16)  /* Vertical */
            )
        );
    }

    //VnCmd(vn, NvImm(0, NvReg3D_ScreenHorizontalControl, 0x10)); // FAULTY

    // Reset all the scissors.
    for (i=0; i<16; i++) {
        VnCmd(vn,
            NvIncr(0, NvReg3D_ScissorHorizontal(i),
                (0xffff << 16) | 0, /* Horizontal */
                (0xffff << 16) | 0  /* Vertical */
            )
        );
    }

    // Setup RAM for macros.
    VnCmd(vn,
        NvImm(0, NvReg3D_MmeShadowRamControl, 1),
        NvIncr(0, NvReg3D_MmeShadowScratch(0x1c),
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffffffff)
    );

    // Reset IndexArrayLimit.
    VnCmd(vn,
        NvIncr(0, NvReg3D_IndexArrayLimit, 0xFF, 0xFFFFFFFF),
        NvImm(0, NvReg3D_PrimRestartWithDrawArrays, 0)
    );

    // More RAM setup.
    VnCmd(vn,
        NvIncr(0, NvReg3D_MmeShadowScratch(0x2a), 0x0500055f),
        NvIncr(0, NvReg3D_MmeShadowScratch(0x2b), 0x05000561),
        NvIncr(0, NvReg3D_MmeShadowScratch(0x2c), 0x05000563),
        NvIncr(0, NvReg3D_MmeShadowScratch(0x2d), 0x05000565),
        NvIncr(0, NvReg3D_MmeShadowScratch(0x2e), 0x05000567),
        NvImm(0, NvReg3D_MmeShadowScratch(0x2f), 0x200),
        NvImm(0, NvReg3D_MmeShadowScratch(0x30), 0x200),
        NvImm(0, NvReg3D_MmeShadowScratch(0x31), 0x200),
        NvImm(0, NvReg3D_MmeShadowScratch(0x32), 0x200),
        NvImm(0, NvReg3D_MmeShadowScratch(0x33), 0x200)
    );

    VnCmd(
        vn,
        NvIncr(0, NvReg3D_SampleCountEnable, 1),
        NvIncr(0, NvReg3D_ClipDistanceEnable, 0xff),
        NvIncr(0, NvReg3D_MsaaMask, 0xffff, 0xffff, 0xffff, 0xffff),
        NvImm(0, 0x367, 0),
        NvImm(0, NvReg3D_PointSpriteEnable, 1),
        NvImm(0, NvReg3D_PointCoordReplace, 4),
        NvImm(0, NvReg3D_VpPointSize, 1),
        NvImm(0, 0x68b, 0),
        NvImm(0, NvReg3D_StencilTwoSideEnable, 1));
    // NvImm(0, 0xe2a, 0x184)); // MACRO CALL NOT IMPLEMENTED

    // TODO: Call macro_206(0x184);
    //VnCmd(vn, NvIncr(0, NvReg3D_ConstantBufferLoadN, 0x44fffe00));

    VnCmd(
        vn, 
        NvImm(0, NvReg3D_ZetaArrayMode, 1),
        NvImm(0, NvReg3D_ConservativeRaster, 0),
        );

    // TODO: Call macro_14f(0x00418800, 0, 0x01800000);

    VnCmd(
        vn,
        NvImm(0, NvReg3D_MmeShadowScratch(0x34), 0),
        NvImm(0, 0xbb, 0),
        NvImm(0, NvReg3D_MultisampleRasterEnable, 0),
        NvImm(0, NvReg3D_CoverageModulationEnable, 0),
        NvImm(0, 0x44c, 0x13), // not in fermi ?
        NvImm(0, NvReg3D_MultisampleCoverageToColor, 0)
    );

    //VnCmd(vn, NvIncr(0, NvReg3D_CodeAddr, 4, 0x00000000));

    VnCmd(vn,
        NvImm(0, NvReg3D_MmeShadowScratch(0x27), 0x230),
        NvImm(0, NvReg3D_MmeShadowScratch(0x23), 0x430),
        NvImm(0, 0x5ad, 0)
    );

    // TODO: Call macro_14f(0x00418e40, 7, 0xf);
    // TODO: Call macro_14f(0x00418e58, 0x842, 0xffff);
    // TODO: Call macro_14f(0x00418e40, 0x70, 0xf0);
    // TODO: Call macro_14f(0x00418e58, 0x04f10000, 0xffff0000);
    // TODO: Call macro_14f(0x00418e40, 0x700, 0xf00);
    // TODO: Call macro_14f(0x00418e5c, 0x53, 0xffff);
    // TODO: Call macro_14f(0x00418e40, 0x7000, 0xf000);
    // TODO: Call macro_14f(0x00418e5c, 0xe90000, 0xffff0000);
    // TODO: Call macro_14f(0x00418e40, 0x70000, 0xf0000);
    // TODO: Call macro_14f(0x00418e60, 0xea, 0xffff);
    // TODO: Call macro_14f(0x00418e40, 0x700000, 0xf00000);
    // TODO: Call macro_14f(0x00418e60, 0x00eb0000, 0xffff0000);
    // TODO: Call macro_14f(0x00418e40, 0x07000000, 0x0f000000);
    // TODO: Call macro_14f(0x00418e64, 0x208, 0xffff);
    // TODO: Call macro_14f(0x00418e40, 0x70000000, 0xf0000000);
    // TODO: Call macro_14f(0x00418e64, 0x02090000, 0xffff0000);
    // TODO: Call macro_14f(0x00418e44, 7, 0xf);
    // TODO: Call macro_14f(0x00418e68, 0x20a, 0xffff);
    // TODO: Call macro_14f(0x00418e44, 0x70, 0xf0);
    // TODO: Call macro_14f(0x00418e68, 0x020b0000, 0xffff0000);
    // TODO: Call macro_14f(0x00418e44, 0x700, 0xf00);
    // TODO: Call macro_14f(0x00418e6c, 0x644, 0xffff);

    // Setting up TiledCache and other stuff.
    VnCmd(vn,
        NvImm(0, 0x3d8, 0),
        NvIncr(0, 0x3d9, 0x00800080),
        NvIncr(0, 0x3da, 0x1109),
        NvIncr(0, 0x3db, 0x08080202),
        NvImm(0, 0x442, 0x1f),
        NvIncr(0, 0x3dc, 0x00080001),
        NvImm(0, 0x44d, 0),
    );

    // TODO: Subchannel 1:
    /*
      NvImm(0, 0x54a, 0),
      NvImm(0, 0x982, 2),
      NvIncr(0, NvReg3D_LocalBase, 0x01000000),
      NvIncr(0, 0x85, 0x03000000),
      NvIncr(0, NvReg3D_CodeAddr, 4, 0x00000000),
      NvImm(0xc4, 0x503),
    */

    // TODO: Subchannel 6:
    /*
      2001c00b type: 1, subchannel: 6
      UNKNOWN (0x00b) <- 0x80000000
      2001c00b type: 1, subchannel: 6
      UNKNOWN (0x00b) <- 0x70000000
    */

    // Flush texture info cache.
    VnCmd(vn,
        NvImm(0, 0x4a2, 0),
        NvImm(0, 0x369, 0x11),
        NvImm(0, 0x50a, 0),
        NvImm(0, 0x509, 0)
    );

    VnCmd(vn,
        NvIncr(0, 0x1e9, 0x7ff8),
        NvIncr(0, 0x1ea, 0x7ffc)
    );
    return 0;
}
