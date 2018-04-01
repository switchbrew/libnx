#include <switch.h>
#include <string.h>

Result vnInit3D(Vn* vn) {
    Result rc;

    VnCmd(vn,
        // ???
        NvIncr(0, NvReg3D_MmeShadowScratch(0x1A), 0, 0xffffffff),
        NvImm(0, NvReg3D_MmeShadowScratch(0x19), 0),
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
        NvImm(0, 0xab, 3),
        NvImm(0, 0xa4, 0),
        NvImm(0, 0x221, 0x3f));

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
        &vn->vertex_runout, 0x10000/*???*/, 0x1000, 0, &vn->parent->addr_space);

    if (R_FAILED(rc))
        return rc;

    iova_t gpu_addr = nvBufferGetGpuAddr(&vn->vertex_runout);
    VnCmd(vn, NvIncr(0, NvReg3D_VertexRunoutAddr, gpu_addr >> 32, gpu_addr));

    // TODO: Call macro_206(0x194);

    // TODO: Write an addr(?) to low->0x8e4,high->0x8e5
    // TODO: Write 0 to 0x8e6, 0x8e7.

    // TODO: Call macro_226(5 /* addr_hi */, 0x00056900 /* addr_low */, 0x100)
    // TODO: Call macro_226(5 /* addr_hi */, 0x00056A00 /* addr_low */, 0x800)

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
                0x5f00, /* Size */
                gpu_addr >> 32, gpu_addr /* Addr */
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

    VnCmd(vn, NvImm(0, NvReg3D_ScreenHorizontalControl, 0x10));

    return 0;
}
