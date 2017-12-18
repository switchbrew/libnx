#include <string.h>
#include <switch.h>

static bool g_gfxInitialized = 0;
static viDisplay g_gfxDisplay;
static Handle g_gfxDisplayVsyncEvent = INVALID_HANDLE;
static viLayer g_gfxLayer;
static u8 g_gfxNativeWindow[0x100];
static u64 g_gfxNativeWindow_Size;
static s32 g_gfxNativeWindow_ID;
static binderSession g_gfxBinderSession;
static s32 g_gfxCurrentBuffer = 0;
static s32 g_gfxCurrentProducerBuffer = 0;
static bool g_gfx_ProducerConnected = 0;
static bool g_gfx_ProducerSlotsRequested[2] = {0, 0};
static u8 *g_gfxFramebuf;
static size_t g_gfxFramebufSize;
/*static u64 g_gfx_DisplayResolution_width, g_gfx_DisplayResolution_height;
static s32 g_gfx_Query_width, g_gfx_Query_height;*/
static bufferProducerFence g_gfx_DequeueBuffer_fence;
static bufferProducerQueueBufferOutput g_gfx_Connect_QueueBufferOutput;
static bufferProducerQueueBufferOutput g_gfx_QueueBuffer_QueueBufferOutput;

static bool g_gfxDoubleBuf = 1;

extern u32 __nx_applet_type;

extern u32 g_nvgfx_totalframebufs;
extern size_t g_nvgfx_singleframebuf_size;
extern nvioctl_fence g_nvgfx_nvhostgpu_gpfifo_fence;

//static Result _gfxGetDisplayResolution(u64 *width, u64 *height);

//TODO: Figure out what all is required to use non-720p width/height.

//TODO: Let the user configure some of this?
static bufferProducerQueueBufferInput g_gfxQueueBufferData = {
    .timestamp = 0x0,
    .isAutoTimestamp = 0x1,
    .crop = {0x0, 0x0, 0x0, 0x0},
    .scalingMode = 0x0,
    .transform = 0x2,
    .stickyTransform = 0x0,
    .unk = {0x0, 0x1},

    .fence = {
        .is_valid = 0x1,
        .nv_fences = {
            {
            .id = 0xffffffff, //Official sw sets this to the output fence from the last nvioctlChannel_SubmitGPFIFO().
            .value = 0x0,
            },
            {0xffffffff, 0x0}, {0xffffffff, 0x0}, {0xffffffff, 0x0},
        },
    }
};

static Result _gfxGetNativeWindowID(u8 *buf, u64 size, s32 *out_ID) {
    u32 *bufptr = (u32*)buf;

    //Validate ParcelData{Size|Offset}.
    if((u64)bufptr[1] > size || (u64)bufptr[0] > size || ((u64)bufptr[1])+((u64)bufptr[0]) > size) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);
    if(bufptr[0] < 0xc) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);
    //bufptr = start of ParcelData
    bufptr = (u32*)&buf[bufptr[1]];

    *out_ID = (s32)bufptr[2];

    return 0;
}

static Result _gfxDequeueBuffer(void) {
    Result rc=0;
    bufferProducerFence *fence = &g_gfx_DequeueBuffer_fence;
    bufferProducerFence tmp_fence;
    bool async=0;

    if (!g_gfxDoubleBuf) {
        g_gfxCurrentProducerBuffer = -1;
        return 0;
    }

    memcpy(&tmp_fence, fence, sizeof(bufferProducerFence));//Offical sw waits on the fence from the previous DequeueBuffer call. Using the fence from the current DequeueBuffer call results in nvgfxEventWait() failing.

    rc = bufferProducerDequeueBuffer(async, 1280, 720, 0, 0x300, &g_gfxCurrentProducerBuffer, fence);

    if (R_SUCCEEDED(rc) && tmp_fence.is_valid) rc = nvgfxEventWait(tmp_fence.nv_fences[0].id, tmp_fence.nv_fences[0].value, -1);

    if (R_SUCCEEDED(rc)) g_gfxCurrentBuffer = (g_gfxCurrentBuffer + 1) & (g_nvgfx_totalframebufs-1);

    //if (R_SUCCEEDED(rc)) rc = nvgfxSubmitGpfifo();

    return rc;
}

static Result _gfxQueueBuffer(s32 buf) {
    Result rc=0;

    if (buf == -1) return 0;

    g_gfxQueueBufferData.timestamp = svcGetSystemTick();//This is probably not the proper value for the timestamp, but shouldn't(?) matter.
    //if (g_nvgfx_nvhostgpu_gpfifo_fence.id) memcpy(&g_gfxQueueBufferData.fence.nv_fences[0], &g_nvgfx_nvhostgpu_gpfifo_fence, sizeof(nvioctl_fence));
    //if (g_nvgfx_nvhostgpu_gpfifo_fence.id) rc = nvgfxEventWait(g_nvgfx_nvhostgpu_gpfifo_fence.id, g_nvgfx_nvhostgpu_gpfifo_fence.value, -1);
    if (R_FAILED(rc)) return rc;

    rc = bufferProducerQueueBuffer(buf, &g_gfxQueueBufferData, &g_gfx_QueueBuffer_QueueBufferOutput);
    if (R_FAILED(rc)) return rc;

    return rc;
}

static Result _gfxInit(viServiceType servicetype, const char *DisplayName, u32 LayerFlags, u64 LayerId, nvServiceType nv_servicetype, size_t nv_transfermem_size) {
    Result rc=0;
    u32 i=0;

    if(g_gfxInitialized)return 0;

    g_gfxNativeWindow_ID = 0;
    g_gfxDisplayVsyncEvent = INVALID_HANDLE;
    g_gfxCurrentBuffer = -1;
    g_gfxCurrentProducerBuffer = -1;
    g_gfx_ProducerConnected = 0;
    g_gfxFramebuf = NULL;
    g_gfxFramebufSize = 0;
    g_gfxDoubleBuf = 1;

    memset(g_gfx_ProducerSlotsRequested, 0, sizeof(g_gfx_ProducerSlotsRequested));
    memset(&g_gfx_DequeueBuffer_fence, 0, sizeof(g_gfx_DequeueBuffer_fence));

    rc = viInitialize(servicetype);
    if (R_FAILED(rc)) return rc;

    rc = viOpenDisplay(DisplayName, &g_gfxDisplay);

    if (R_SUCCEEDED(rc)) rc = viGetDisplayVsyncEvent(&g_gfxDisplay, &g_gfxDisplayVsyncEvent);

    if (R_SUCCEEDED(rc)) rc = viOpenLayer(g_gfxNativeWindow, &g_gfxNativeWindow_Size, &g_gfxDisplay, &g_gfxLayer, LayerFlags, LayerId);

    if (R_SUCCEEDED(rc)) rc = viSetLayerScalingMode(&g_gfxLayer, VISCALINGMODE_Default);

    if (R_SUCCEEDED(rc)) rc = _gfxGetNativeWindowID(g_gfxNativeWindow, g_gfxNativeWindow_Size, &g_gfxNativeWindow_ID);

    if (R_SUCCEEDED(rc)) {
        binderCreateSession(&g_gfxBinderSession, viGetSession_IHOSBinderDriverRelay(), g_gfxNativeWindow_ID);
        rc = binderInitSession(&g_gfxBinderSession, 0x0f);
    }

    if (R_SUCCEEDED(rc)) rc = nvInitialize(nv_servicetype, nv_transfermem_size);

    if (R_SUCCEEDED(rc)) rc = bufferProducerInitialize(&g_gfxBinderSession);

    if (R_SUCCEEDED(rc)) rc = bufferProducerConnect(2, 0, &g_gfx_Connect_QueueBufferOutput);

    if (R_SUCCEEDED(rc)) g_gfx_ProducerConnected = 1;

    if (R_SUCCEEDED(rc)) rc = nvgfxInitialize();

    if (R_SUCCEEDED(rc)) rc = nvgfxGetFramebuffer(&g_gfxFramebuf, &g_gfxFramebufSize);

    if (R_SUCCEEDED(rc)) { //TODO: Merge this into gfxSwapBuffers()?
       for(i=0; i<2; i++) {
           rc = _gfxDequeueBuffer();
           if (R_FAILED(rc)) break;

           rc = bufferProducerRequestBuffer(g_gfxCurrentProducerBuffer);
           if (R_FAILED(rc)) break;

           g_gfx_ProducerSlotsRequested[i] = 1;

           //Officially, nvioctlNvmap_FromID() and nvioctlChannel_SubmitGPFIFO() are used here.

           rc = _gfxQueueBuffer(g_gfxCurrentProducerBuffer);
           if (R_FAILED(rc)) {
               g_gfxCurrentProducerBuffer = -1;
               break;
           }
       }
    }

    if (R_SUCCEEDED(rc)) svcSleepThread(3000000000);

    if (R_SUCCEEDED(rc)) rc = _gfxDequeueBuffer();

    /*if (R_SUCCEEDED(rc)) rc = _gfxGetDisplayResolution(&g_gfx_DisplayResolution_width, &g_gfx_DisplayResolution_height);

    if (R_SUCCEEDED(rc)) rc = bufferProducerQuery(NATIVE_WINDOW_WIDTH, &g_gfx_Query_width);
    if (R_SUCCEEDED(rc)) rc = bufferProducerQuery(NATIVE_WINDOW_HEIGHT, &g_gfx_Query_height);*/

    /*if (R_SUCCEEDED(rc)) { //Workaround a gfx display issue.
        for(i=0; i<2; i++)gfxWaitForVsync();
    }*/

    if (R_FAILED(rc)) {
        _gfxQueueBuffer(g_gfxCurrentProducerBuffer);
        for(i=0; i<2; i++) {
            if (g_gfx_ProducerSlotsRequested[i]) bufferProducerDetachBuffer(i);
        }
        if (g_gfx_ProducerConnected) bufferProducerDisconnect(2);

        nvgfxExit();
        bufferProducerExit();
        binderExitSession(&g_gfxBinderSession);
        nvExit();

        viCloseLayer(&g_gfxLayer);
        viCloseDisplay(&g_gfxDisplay);
        viExit();

        if(g_gfxDisplayVsyncEvent != INVALID_HANDLE) {
            svcCloseHandle(g_gfxDisplayVsyncEvent);
            g_gfxDisplayVsyncEvent = INVALID_HANDLE;
        }

        g_gfxNativeWindow_ID = 0;
        g_gfxCurrentBuffer = 0;
        g_gfxCurrentProducerBuffer = -1;
        g_gfx_ProducerConnected = 0;
        g_gfxFramebuf = NULL;
        g_gfxFramebufSize = 0;

        memset(g_gfx_ProducerSlotsRequested, 0, sizeof(g_gfx_ProducerSlotsRequested));
    }

    if (R_SUCCEEDED(rc)) g_gfxInitialized = 1;

    return rc;
}

void gfxInitDefault(void) {
    nvServiceType nv_servicetype = NVSERVTYPE_Default;

    if(__nx_applet_type != APPLET_TYPE_None) {
        switch(__nx_applet_type) {
            case APPLET_TYPE_Application:
            case APPLET_TYPE_SystemApplication:
               nv_servicetype = NVSERVTYPE_Application;
            break;

            case APPLET_TYPE_SystemApplet:
            case APPLET_TYPE_LibraryApplet:
            case APPLET_TYPE_OverlayApplet:
               nv_servicetype = NVSERVTYPE_Applet;
            break;
        }
    }

    Result rc = _gfxInit(VILAYERFLAGS_Default, "Default", VILAYERFLAGS_Default, 0, nv_servicetype, 0x300000);
    if (R_FAILED(rc)) fatalSimple(rc);
}

void gfxExit(void) {
    u32 i=0;
    if(!g_gfxInitialized)return;

    _gfxQueueBuffer(g_gfxCurrentProducerBuffer);
    for(i=0; i<2; i++) {
        if (g_gfx_ProducerSlotsRequested[i]) bufferProducerDetachBuffer(i);
    }
    if (g_gfx_ProducerConnected) bufferProducerDisconnect(2);

    nvgfxExit();

    bufferProducerExit();
    binderExitSession(&g_gfxBinderSession);

    nvExit();

    viCloseLayer(&g_gfxLayer);

    if(g_gfxDisplayVsyncEvent != INVALID_HANDLE) {
        svcCloseHandle(g_gfxDisplayVsyncEvent);
        g_gfxDisplayVsyncEvent = INVALID_HANDLE;
    }

    viCloseDisplay(&g_gfxDisplay);

    viExit();

    g_gfxInitialized = 0;
    g_gfxNativeWindow_ID = 0;

    g_gfxCurrentBuffer = 0;
    g_gfxCurrentProducerBuffer = -1;
    g_gfx_ProducerConnected = 0;
    g_gfxFramebuf = NULL;
    g_gfxFramebufSize = 0;

    memset(g_gfx_ProducerSlotsRequested, 0, sizeof(g_gfx_ProducerSlotsRequested));
}

static void _waitevent(Handle *handle) {
    s32 tmpindex=0;
    Result rc=0, rc2=0;

    do {
        rc = svcWaitSynchronization(&tmpindex, handle, 1, U64_MAX);
        if (R_SUCCEEDED(rc)) rc2 = svcResetSignal(*handle);
    } while(R_FAILED(rc) || (rc2 & 0x3FFFFF)==0xFA01);

    if (R_FAILED(rc2)) fatalSimple(rc2);
}

void gfxWaitForVsync() {
    _waitevent(&g_gfxDisplayVsyncEvent);
}

void gfxSwapBuffers() {
    Result rc=0;

    rc = _gfxQueueBuffer(g_gfxCurrentProducerBuffer);

    if (R_SUCCEEDED(rc)) rc = _gfxDequeueBuffer();

    #ifdef BUFFERSWAP_DELAY_HACK
    gfxWaitForVsync();
    gfxWaitForVsync();
    #endif

    if (R_FAILED(rc)) fatalSimple(rc);
}

u8* gfxGetFramebuffer(u32* width, u32* height) {
    if(width) *width = 1280;
    if(height) *height = 720;

    return &g_gfxFramebuf[g_gfxCurrentBuffer*g_nvgfx_singleframebuf_size];
}

void gfxSetDoubleBuffering(bool doubleBuffering) {
    g_gfxDoubleBuf = doubleBuffering;
}

void gfxFlushBuffers(void) {
    armDCacheFlush(&g_gfxFramebuf[g_gfxCurrentBuffer*g_nvgfx_singleframebuf_size], g_nvgfx_singleframebuf_size);
}

/*static Result _gfxGetDisplayResolution(u64 *width, u64 *height) {
    return viGetDisplayResolution(&g_gfxDisplay, width, height);
}*/

