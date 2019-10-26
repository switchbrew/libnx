#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "kernel/event.h"
#include "services/audin.h"
#include "runtime/hosversion.h"

#define DEVICE_NAME_LENGTH 0x100
#define DEFAULT_SAMPLE_RATE 0xBB80
#define DEFAULT_CHANNEL_COUNT 0x00020000

static Service g_audinSrv;
static Service g_audinIAudioIn;

static Event g_audinBufferEvent;

static u32 g_sampleRate = 0;
static u32 g_channelCount = 0;
static PcmFormat g_pcmFormat = PcmFormat_Invalid;
static AudioInState g_deviceState = AudioInState_Stopped;

static Result _audinRegisterBufferEvent(Event *BufferEvent);

NX_GENERATE_SERVICE_GUARD(audin);

Result _audinInitialize(void) {
    Result rc = 0;
    rc = smGetService(&g_audinSrv, "audin:u");
    
    // Setup the default device
    if (R_SUCCEEDED(rc)) {
        // Passing an empty device name will open the default "BuiltInHeadset"
        char DeviceNameIn[DEVICE_NAME_LENGTH] = {0};
        char DeviceNameOut[DEVICE_NAME_LENGTH] = {0};
        
        // Open audio input device
        rc = audinOpenAudioIn(DeviceNameIn, DeviceNameOut, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_COUNT, &g_sampleRate, &g_channelCount, &g_pcmFormat, &g_deviceState);
    }
    
    // Register global handle for buffer events
    if (R_SUCCEEDED(rc))
        rc = _audinRegisterBufferEvent(&g_audinBufferEvent);
    
    return rc;
}

void _audinCleanup(void) {
    eventClose(&g_audinBufferEvent);

    g_sampleRate = 0;
    g_channelCount = 0;
    g_pcmFormat = PcmFormat_Invalid;
    g_deviceState = AudioInState_Stopped;

    serviceClose(&g_audinIAudioIn);
    serviceClose(&g_audinSrv);
}

Service* audinGetServiceSession(void) {
    return &g_audinSrv;
}

Service* audinGetServiceSession_AudioIn(void) {
    return &g_audinIAudioIn;
}

u32 audinGetSampleRate(void) {
    return g_sampleRate;
}

u32 audinGetChannelCount(void) {
    return g_channelCount;
}

PcmFormat audinGetPcmFormat(void) {
    return g_pcmFormat;
}

AudioInState audinGetDeviceState(void) {
    return g_deviceState;
}

static Result _audinCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _audinCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _audinCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _audinCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _audinCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

Result audinWaitCaptureFinish(AudioInBuffer **released, u32* released_count, u64 timeout) {
    // Wait on the buffer event handle
    Result rc = eventWait(&g_audinBufferEvent, timeout);
        
    if (R_SUCCEEDED(rc)) {
        // Grab the released buffer
        rc = audinGetReleasedAudioInBuffer(released, released_count);
    }
    
    return rc;
}

Result audinCaptureBuffer(AudioInBuffer *source, AudioInBuffer **released) {
    Result rc = 0;
    u32 released_count = 0;
    
    // Try to push the supplied buffer to the audio input device
    rc = audinAppendAudioInBuffer(source);
    
    if (R_SUCCEEDED(rc))
        rc = audinWaitCaptureFinish(released, &released_count, U64_MAX);
    
    return rc;
}

Result audinListAudioIns(char *DeviceNames, s32 count, u32 *DeviceNamesCount) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchOut(&g_audinSrv, new_cmd==0 ? 0 : 2, *DeviceNamesCount,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_Out },
        .buffers = { { DeviceNames, count*DEVICE_NAME_LENGTH } },
    );
}

Result audinOpenAudioIn(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioInState *State) {
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
    Result rc = serviceDispatchInOut(&g_audinSrv, new_cmd==0 ? 1 : 3, in, out,
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
        .out_objects = &g_audinIAudioIn,
    );
    if (R_SUCCEEDED(rc)) {
        if (SampleRateOut) *SampleRateOut = out.sample_rate;
        if (ChannelCountOut) *ChannelCountOut = out.channel_count;
        if (Format) *Format = out.pcm_format;
        if (State) *State = out.state;
    }
    return rc;
}

Result audinGetAudioInState(AudioInState *State) {
    u32 tmp=0;
    Result rc = _audinCmdNoInOutU32(&g_audinIAudioIn, &tmp, 0);
    if (R_SUCCEEDED(rc) && State) *State = tmp;
    return rc;
}

Result audinStartAudioIn(void) {
    return _audinCmdNoIO(&g_audinIAudioIn, 1);
}

Result audinStopAudioIn(void) {
    return _audinCmdNoIO(&g_audinIAudioIn, 2);
}

Result audinAppendAudioInBuffer(AudioInBuffer *Buffer) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    u64 tmp = (u64)Buffer;
    return serviceDispatchIn(&g_audinIAudioIn, new_cmd==0 ? 3 : 8, tmp,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_In },
        .buffers = { { Buffer, sizeof(*Buffer) } },
    );
}

static Result _audinRegisterBufferEvent(Event *BufferEvent) {
    return _audinCmdGetEvent(&g_audinIAudioIn, BufferEvent, true, 4);
}

Result audinGetReleasedAudioInBuffer(AudioInBuffer **Buffer, u32 *ReleasedBuffersCount) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchOut(&g_audinIAudioIn, new_cmd==0 ? 5 : 9, *ReleasedBuffersCount,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_Out },
        .buffers = { { Buffer, sizeof(*Buffer) } },
    );
}

Result audinContainsAudioInBuffer(AudioInBuffer *Buffer, bool *ContainsBuffer) {
    u64 tmp = (u64)Buffer;
    u8 out=0;
    Result rc = serviceDispatchInOut(&g_audinIAudioIn, 6, tmp, out);
    if (R_SUCCEEDED(rc) && ContainsBuffer) *ContainsBuffer = out & 1;
    return rc;
}
