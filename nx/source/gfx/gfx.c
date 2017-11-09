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

static u32 g_gfxQueueBufferData[0x5c>>2] = {
0x54, 0x0,
0x0, 0x0, //u64 timestamp
0x1, 0x0, 0x0,
0x0, 0x0, 0x0, 0x2,
0x0, 0x0, 0x1, 0x1,
0x42,
0x13f4, //Increased by 6/7 each time.
0xffffffff, 0x0,
0xffffffff, 0x0, 0xffffffff, 0x0};

static u32 g_gfxBufferInitData[0x178>>2] = {
0x1, 0x16c, 0x0,
0x47424652,
1280, 720,
1280,
0x1, 0xb00, 0x2a, 0x0,
0x0, 0x51, 0xffffffff, 0xcb8,
0x0, 0xdaffcaff, 0x2a, 0x0,
0xb00, 0x1, 0x1, 1280,
0x3c0000, 0x1, 0x0, 1280,
720, 0x532120, 0x1, 0x3,
0x1400, 0xcb8,
0x0,
0xfe,
0x4, 0x0, 0x0, 0x0,
0x0, 0x3c0000, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0,
0x0,
0x0, 0x0 //Unknown, some timestamp perhaps?
};

static Result _gfxGetNativeWindowID(u8 *buf, u64 size, s32 *out_ID) {
    u32 *bufptr = (u32*)buf;

    //Validate ParcelData{Size|Offset}.
    if((u64)bufptr[1] >= size || (u64)bufptr[0] >= size || ((u64)bufptr[1])+((u64)bufptr[0]) >= size) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);
    if(bufptr[0] < 0xc) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);

    //bufptr = start of ParcelData
    bufptr = (u32*)&buf[bufptr[1]];

    *out_ID = (s32)bufptr[2];

    return 0;
}

static Result _gfxDequeueBuffer() {
    return gfxproducerDequeueBuffer(1, 1280, 720, 0, 0x300);//reply_parcel currently contains error(s), presumably due to nv not being initialized for this.
}

static Result _gfxQueueBuffer(s32 buf) {
    Result rc=0;
    u64 *ptr64 = (u64*)&g_gfxQueueBufferData;
    ptr64[1] = svcGetSystemTick();//Unknown what is actually used for timestamp, but shouldn't(?) matter.

    rc = gfxproducerQueueBuffer(buf, (u8*)g_gfxQueueBufferData);
    if (R_FAILED(rc)) return rc;

    if(buf==0) {
        g_gfxQueueBufferData[0x10]+= 0x6;
    } else {
        g_gfxQueueBufferData[0x10]+= 0x7;
    }

    return rc;
}

static Result _gfxInit(viServiceType servicetype, const char *DisplayName, u32 LayerFlags, u64 LayerId) {
    Result rc=0;
    s32 tmp=0;
    u32 i=0;
    u64 *ptr64 = (u64*)g_gfxBufferInitData;

    if(g_gfxInitialized)return 0;

    g_gfxNativeWindow_ID = 0;
    g_gfxDisplayVsyncEvent = INVALID_HANDLE;
    g_gfxCurrentBuffer = 0;

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

    if (R_SUCCEEDED(rc)) rc = gfxproducerInitialize(&g_gfxBinderSession);

    if (R_SUCCEEDED(rc)) rc = gfxproducerConnect(2, 0);

    if (R_SUCCEEDED(rc)) rc = gfxproducerQuery(2, &tmp);//"NATIVE_WINDOW_FORMAT"

    if (R_SUCCEEDED(rc)) {
       for(i=0; i<2; i++) {
           g_gfxBufferInitData[0xa] = i;
           g_gfxBufferInitData[0x21] = 0x3c0000*i;
           ptr64[0x170>>3] = svcGetSystemTick();
           rc = gfxproducerBufferInit(i, (u8*)g_gfxBufferInitData);
           if (R_FAILED(rc)) break;
       }
    }

    if (R_SUCCEEDED(rc)) {
       for(i=0; i<2; i++) {
           rc = _gfxDequeueBuffer();
           if (R_FAILED(rc)) break;

           rc = gfxproducerRequestBuffer(i);//reply_parcel currently contains an error, presumably due to _gfxDequeueBuffer() failing as mentioned above.
           if (R_FAILED(rc)) break;

           rc = _gfxQueueBuffer(i);//reply_parcel currently contains the same error as gfxproducerRequestBuffer() above.
           if (R_FAILED(rc)) break;
       }
    }

    if (R_SUCCEEDED(rc)) rc = _gfxDequeueBuffer();

    if (R_FAILED(rc)) {
        binderExitSession(&g_gfxBinderSession);
        viCloseLayer(&g_gfxLayer);
        viCloseDisplay(&g_gfxDisplay);
        viExit();
    }

    if (R_SUCCEEDED(rc)) g_gfxInitialized = 1;

    return rc;
}

void gfxInitDefault(void) {
    Result rc = _gfxInit(VILAYERFLAGS_Default, "Default", VILAYERFLAGS_Default, 0);
    if (R_FAILED(rc)) fatalSimple(rc);
}

void gfxExit(void) {
    if(!g_gfxInitialized)return;

    gfxproducerExit();

    binderExitSession(&g_gfxBinderSession);

    viCloseLayer(&g_gfxLayer);

    if(g_gfxDisplayVsyncEvent != INVALID_HANDLE) {
        svcCloseHandle(g_gfxDisplayVsyncEvent);
        g_gfxDisplayVsyncEvent = INVALID_HANDLE;
    }

    viCloseDisplay(&g_gfxDisplay);

    viExit();

    g_gfxInitialized = 0;
    g_gfxNativeWindow_ID = 0;
}

void gfxWaitForVsync() {
    s32 tmpindex=0;
    svcClearEvent(g_gfxDisplayVsyncEvent);
    svcWaitSynchronization(&tmpindex, &g_gfxDisplayVsyncEvent, 1, U64_MAX);
}

void gfxSwapBuffers() {
    Result rc=0;

    rc = _gfxQueueBuffer(g_gfxCurrentBuffer);
    g_gfxCurrentBuffer ^= 1;

    if (R_SUCCEEDED(rc)) rc = _gfxDequeueBuffer();

    if (R_FAILED(rc)) fatalSimple(rc);
}

