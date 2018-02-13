#include <string.h>
#include "types.h"
#include "result.h"
#include "ipc.h"
#include "services/audout.h"
#include "services/sm.h"

#define DEVICE_NAME_LENGTH 0x100
#define DEFAULT_SAMPLE_RATE 0xBB80
#define CHANNEL_COUNT_MAGIC 0xCAFE0000

static Service g_audoutSrv;
static Service g_audoutIAudioOut;

static u32 g_sampleRate = 0;
static u32 g_channelCount = 0;
static PcmFormat g_pcmFormat = PcmFormat_Invalid;
static AudioOutState g_deviceState = AudioOutState_Stopped;

Result audoutInitialize(void)
{
    if (serviceIsActive(&g_audoutSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    Result rc = 0;
    rc = smGetService(&g_audoutSrv, "audout:u");
    
    // Setup the default device
    if (R_SUCCEEDED(rc))
    {        
        // Passing an empty device name will open the default "DeviceOut"
        char DeviceNameIn[DEVICE_NAME_LENGTH] = {0};
        char DeviceNameOut[DEVICE_NAME_LENGTH] = {0};
        
        rc = audoutOpenAudioOut(DeviceNameIn, DeviceNameOut, DEFAULT_SAMPLE_RATE, CHANNEL_COUNT_MAGIC, &g_sampleRate, &g_channelCount, &g_pcmFormat, &g_deviceState);
    }
    
    if (R_FAILED(rc))
        audoutExit();

    return rc;
}

void audoutExit(void)
{
    g_sampleRate = 0;
    g_channelCount = 0;
    g_pcmFormat = PcmFormat_Invalid;
    g_deviceState = AudioOutState_Stopped;

    serviceClose(&g_audoutIAudioOut);
    serviceClose(&g_audoutSrv);
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

Result audoutListAudioOuts(char *DeviceNames, u32 *DeviceNamesCount) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvBuffer(&c, DeviceNames, DEVICE_NAME_LENGTH, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_audoutSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 DeviceNamesCount;
        } *resp = r.Raw;
        
        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && DeviceNamesCount)
            *DeviceNamesCount = resp->DeviceNamesCount;
    }

    return rc;
}

Result audoutOpenAudioOut(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioOutState *State) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sample_rate;
        u32 channel_count;
        u64 client_pid;
    } *raw;

    ipcSendPid(&c);
    ipcSendHandleCopy(&c, CUR_PROCESS_HANDLE);
    ipcAddSendBuffer(&c, DeviceNameIn, DEVICE_NAME_LENGTH, 0);
    ipcAddRecvBuffer(&c, DeviceNameOut, DEVICE_NAME_LENGTH, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->sample_rate = SampleRateIn;
    raw->channel_count = ChannelCountIn;
    raw->client_pid = 0;

    Result rc = serviceIpcDispatch(&g_audoutSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 sample_rate;
            u32 channel_count;
            u32 pcm_format;
            u32 state;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&g_audoutIAudioOut, r.Handles[0]);
            
            if (SampleRateOut)
                *SampleRateOut = resp->sample_rate;
            
            if (ChannelCountOut)
                *ChannelCountOut = resp->channel_count;
            
            if (Format)
                *Format = resp->pcm_format;
            
            if (State)
                *State = resp->state;
        }
    }

    return rc;
}

Result audoutGetAudioOutState(AudioOutState *State) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 state;
        } *resp = r.Raw;
        
        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && State)
            *State = resp->state;
    }

    return rc;
}

Result audoutStartAudioOut(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;
        
        rc = resp->result;
    }

    return rc;
}

Result audoutStopAudioOut(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;
        
        rc = resp->result;
    }

    return rc;
}

Result audoutAppendAudioOutBuffer(AudioOutBuffer *Buffer) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 tag;
    } *raw;

    ipcAddSendBuffer(&c, Buffer, sizeof(AudioOutBuffer), 0);
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->tag = (u64)&Buffer;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;
        
        rc = resp->result;
    }

    return rc;
}

Result audoutRegisterBufferEvent(Handle *BufferEvent) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;
        
        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && BufferEvent)
            *BufferEvent = r.Handles[0];
    }

    return rc;
}

Result audoutGetReleasedAudioOutBuffer(AudioOutBuffer *Buffer, u32 *ReleasedBuffersCount) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvBuffer(&c, Buffer, sizeof(AudioOutBuffer), 0);
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 released_buffers_count;
        } *resp = r.Raw;
        
        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && ReleasedBuffersCount)
            *ReleasedBuffersCount = resp->released_buffers_count;
    }

    return rc;
}

Result audoutContainsAudioOutBuffer(AudioOutBuffer *Buffer, bool *ContainsBuffer) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 tag;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;
    raw->tag = (u64)&Buffer;

    Result rc = serviceIpcDispatch(&g_audoutIAudioOut);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 contains_buffer;
        } *resp = r.Raw;
        
        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && ContainsBuffer)
            *ContainsBuffer = (resp->contains_buffer & 0x01);
    }

    return rc;
}
