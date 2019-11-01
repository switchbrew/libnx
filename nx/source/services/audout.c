#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "kernel/event.h"
#include "services/audout.h"
#include "runtime/hosversion.h"

#define DEVICE_NAME_LENGTH 0x100
#define DEFAULT_SAMPLE_RATE 0xBB80
#define DEFAULT_CHANNEL_COUNT 0x00020000

static Service g_audoutSrv;
static Service g_audoutIAudioOut;

static Event g_audoutBufferEvent;

static u32 g_sampleRate = 0;
static u32 g_channelCount = 0;
static PcmFormat g_pcmFormat = PcmFormat_Invalid;
static AudioOutState g_deviceState = AudioOutState_Stopped;

static Result _audoutRegisterBufferEvent(Event *BufferEvent);

NX_GENERATE_SERVICE_GUARD(audout);

Result _audoutInitialize(void) {
    Result rc = 0;
    rc = smGetService(&g_audoutSrv, "audout:u");
    
    // Setup the default device
    if (R_SUCCEEDED(rc)) {
        // Passing an empty device name will open the default "DeviceOut"
        char DeviceNameIn[DEVICE_NAME_LENGTH] = {0};
        char DeviceNameOut[DEVICE_NAME_LENGTH] = {0};
        
        // Open audio output device
        rc = audoutOpenAudioOut(DeviceNameIn, DeviceNameOut, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_COUNT, &g_sampleRate, &g_channelCount, &g_pcmFormat, &g_deviceState);
    }
    
    // Register global handle for buffer events
    if (R_SUCCEEDED(rc))
        rc = _audoutRegisterBufferEvent(&g_audoutBufferEvent);

    return rc;
}

void _audoutCleanup(void) {
    eventClose(&g_audoutBufferEvent);

    g_sampleRate = 0;
    g_channelCount = 0;
    g_pcmFormat = PcmFormat_Invalid;
    g_deviceState = AudioOutState_Stopped;

    serviceClose(&g_audoutIAudioOut);
    serviceClose(&g_audoutSrv);
}

Service* audoutGetServiceSession(void) {
    return &g_audoutSrv;
}

Service* audoutGetServiceSession_AudioOut(void) {
    return &g_audoutIAudioOut;
}

u32 audoutGetSampleRate(void) {
    return g_sampleRate;
}

u32 audoutGetChannelCount(void) {
    return g_channelCount;
}

PcmFormat audoutGetPcmFormat(void) {
    return g_pcmFormat;
}

AudioOutState audoutGetDeviceState(void) {
    return g_deviceState;
}

static Result _audoutCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _audoutCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _audoutCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _audoutCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _audoutCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

Result audoutWaitPlayFinish(AudioOutBuffer **released, u32* released_count, u64 timeout) {
    // Wait on the buffer event handle
    Result rc = eventWait(&g_audoutBufferEvent, timeout);
    
    if (R_SUCCEEDED(rc)) {
        // Grab the released buffer
        rc = audoutGetReleasedAudioOutBuffer(released, released_count);
    }
    
    return rc;
}

Result audoutPlayBuffer(AudioOutBuffer *source, AudioOutBuffer **released) {
    Result rc = 0;
    u32 released_count = 0;
    
    // Try to push the supplied buffer to the audio output device
    rc = audoutAppendAudioOutBuffer(source);
    
    if (R_SUCCEEDED(rc))
        rc = audoutWaitPlayFinish(released, &released_count, U64_MAX);
    
    return rc;
}

Result audoutListAudioOuts(char *DeviceNames, s32 count, u32 *DeviceNamesCount) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchOut(&g_audoutSrv, new_cmd==0 ? 0 : 2, *DeviceNamesCount,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_Out },
        .buffers = { { DeviceNames, count*DEVICE_NAME_LENGTH } },
    );
}

Result audoutOpenAudioOut(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioOutState *State) {
    bool new_cmd = hosversionAtLeast(3,0,0);

    const struct {
        u32 sample_rate;
        u32 channel_count;
        u64 client_pid;
    } in = { SampleRateIn, ChannelCountIn, 0 };

    struct {
        u32 sample_rate;
        u32 channel_count;
        u32 pcm_format;
        u32 state;
    } out;

    u32 tmpattr = new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect;
    Result rc = serviceDispatchInOut(&g_audoutSrv, new_cmd==0 ? 1 : 3, in, out,
        .buffer_attrs = {
            tmpattr | SfBufferAttr_In,
            tmpattr | SfBufferAttr_Out,
        },
        .buffers = {
            { DeviceNameIn, DEVICE_NAME_LENGTH },
            { DeviceNameOut, DEVICE_NAME_LENGTH },
        },
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { CUR_PROCESS_HANDLE },
        .out_num_objects = 1,
        .out_objects = &g_audoutIAudioOut,
    );
    if (R_SUCCEEDED(rc)) {
        if (SampleRateOut) *SampleRateOut = out.sample_rate;
        if (ChannelCountOut) *ChannelCountOut = out.channel_count;
        if (Format) *Format = out.pcm_format;
        if (State) *State = out.state;
    }
    return rc;
}

Result audoutGetAudioOutState(AudioOutState *State) {
    u32 tmp=0;
    Result rc = _audoutCmdNoInOutU32(&g_audoutIAudioOut, &tmp, 0);
    if (R_SUCCEEDED(rc) && State) *State = tmp;
    return rc;
}

Result audoutStartAudioOut(void) {
    return _audoutCmdNoIO(&g_audoutIAudioOut, 1);
}

Result audoutStopAudioOut(void) {
    return _audoutCmdNoIO(&g_audoutIAudioOut, 2);
}

Result audoutAppendAudioOutBuffer(AudioOutBuffer *Buffer) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    u64 tmp = (u64)Buffer;
    return serviceDispatchIn(&g_audoutIAudioOut, new_cmd==0 ? 3 : 7, tmp,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_In },
        .buffers = { { Buffer, sizeof(*Buffer) } },
    );
}

static Result _audoutRegisterBufferEvent(Event *BufferEvent) {
    return _audoutCmdGetEvent(&g_audoutIAudioOut, BufferEvent, true, 4);
}

Result audoutGetReleasedAudioOutBuffer(AudioOutBuffer **Buffer, u32 *ReleasedBuffersCount) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchOut(&g_audoutIAudioOut, new_cmd==0 ? 5 : 8, *ReleasedBuffersCount,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_Out },
        .buffers = { { Buffer, sizeof(*Buffer) } },
    );
}

Result audoutContainsAudioOutBuffer(AudioOutBuffer *Buffer, bool *ContainsBuffer) {
    u64 tmp = (u64)Buffer;
    u8 out=0;
    Result rc = serviceDispatchInOut(&g_audoutIAudioOut, 6, tmp, out);
    if (R_SUCCEEDED(rc) && ContainsBuffer) *ContainsBuffer = out & 1;
    return rc;
}
