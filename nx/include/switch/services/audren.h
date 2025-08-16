/**
 * @file audren.h
 * @brief Audio renderer service.
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../kernel/event.h"
#include "../audio/audio.h"
#include "../sf/service.h"

#define AUDREN_TIMER_FREQ_HZ           200.0f
#define AUDREN_TIMER_PERIOD_MS         5.0f
#define AUDREN_SAMPLES_PER_FRAME_32KHZ 160
#define AUDREN_SAMPLES_PER_FRAME_48KHZ 240

#define AUDREN_INPUT_PARAM_ALIGNMENT  0x1000
#define AUDREN_OUTPUT_PARAM_ALIGNMENT 0x10
#define AUDREN_MEMPOOL_ALIGNMENT      0x1000
#define AUDREN_BUFFER_ALIGNMENT       0x40

#define AUDREN_REVISION_1 0x31564552 // REV1 [1.0.0+]
#define AUDREN_REVISION_2 0x32564552 // REV2 [2.0.0+]
#define AUDREN_REVISION_3 0x33564552 // REV3 [3.0.0+]
#define AUDREN_REVISION_4 0x34564552 // REV4 [4.0.0+]
#define AUDREN_REVISION_5 0x35564552 // REV5 [6.0.0+]
#define AUDREN_REVISION_6 0x36564552 // REV6 [6.1.0+]

#define AUDREN_NODEID(_a,_b,_c)   ((((u32)(_a) & 0xF) << 28) | (((u32)(_b) & 0xFFF) << 16) | ((u32)(_c) & 0xFFFF))
#define AUDREN_FINAL_MIX_ID       0
#define AUDREN_UNUSED_MIX_ID      0x7FFFFFFF
#define AUDREN_UNUSED_SPLITTER_ID 0xFFFFFFFF

#define AUDREN_DEFAULT_DEVICE_NAME "MainAudioOut"

typedef enum {
    AudioRendererOutputRate_32kHz,
    AudioRendererOutputRate_48kHz,
} AudioRendererOutputRate;

typedef struct {
    AudioRendererOutputRate output_rate;
    int num_voices;
    int num_effects;
    int num_sinks;
    int num_mix_objs;
    int num_mix_buffers;
} AudioRendererConfig;

/*
Input buffer layout:

AudioRendererUpdateDataHeader
AudioRendererBehaviorInfoIn
AudioRendererMemPoolInfoIn * mempool_count
AudioRendererChannelInfoIn * channel_count
AudioRendererVoiceInfoIn * voice_count
(effects would go here)
(splitters would go here)
AudioRendererMixInfoIn * mix_count (i.e. submix_count+1)
AudioRendererSinkInfoIn * sink_count
AudioRendererPerformanceBufferInfoIn
*/

/*
Output buffer layout:

AudioRendererUpdateDataHeader
AudioRendererMemPoolInfoOut * mempool_count
AudioRendererVoiceInfoOut * voice_count
(effects would go here)
AudioRendererSinkInfoOut * sink_count
AudioRendererPerformanceBufferInfoOut
AudioRendererBehaviorInfoOut
*/

typedef struct {
    u32 revision;
    u32 behavior_sz;
    u32 mempools_sz;
    u32 voices_sz;
    u32 channels_sz;
    u32 effects_sz;
    u32 mixes_sz;
    u32 sinks_sz;
    u32 perfmgr_sz;
    u32 _padding[6];
    u32 total_sz;
} AudioRendererUpdateDataHeader;

typedef struct {
    u32 revision;
    u32 _padding1;
    u64 flags;
} AudioRendererBehaviorInfoIn;

typedef struct {
    u64 unknown[20];
    u64 _padding1[2];
} AudioRendererBehaviorInfoOut;

typedef enum {
    AudioRendererMemPoolState_Invalid,
    AudioRendererMemPoolState_New,
    AudioRendererMemPoolState_RequestDetach,
    AudioRendererMemPoolState_Detached,
    AudioRendererMemPoolState_RequestAttach,
    AudioRendererMemPoolState_Attached,
    AudioRendererMemPoolState_Released,
} AudioRendererMemPoolState;

typedef struct {
    const void* address;
    u64 size;
    AudioRendererMemPoolState state;
    u32 _padding2[3];
} AudioRendererMemPoolInfoIn;

typedef struct
{
    AudioRendererMemPoolState new_state;
    u32 _padding2[3];
} AudioRendererMemPoolInfoOut;

typedef struct {
    u32 id;
    float mix[24];
    bool is_used;
    u8 _padding1[11];
} AudioRendererChannelInfoIn;

typedef struct {
    bool enable;
    u8 _padding;
    s16 numerator[3];
    s16 denominator[2];
} AudioRendererBiquadFilter;

typedef struct {
    u16 coefficients[16];
} AudioRendererAdpcmParameters;

typedef struct {
    u16 index;
    s16 history0;
    s16 history1;
} AudioRendererAdpcmContext;

typedef struct {
    const void* address;
    u64 size;
    s32 start_sample_offset;
    s32 end_sample_offset;
    bool is_looping;
    bool end_of_stream;
    bool sent_to_server;
    u8 _padding1[5];
    const void* context_addr;
    u64 context_sz;
    u64 _padding2;
} AudioRendererWaveBuf;

typedef enum {
    AudioRendererVoicePlayState_Started,
    AudioRendererVoicePlayState_Stopped,
    AudioRendererVoicePlayState_Paused,
} AudioRendererVoicePlayState;

typedef struct {
    u32 id;
    u32 node_id;
    bool is_new;
    bool is_used;
    AudioRendererVoicePlayState state : 8;
    PcmFormat sample_format : 8;
    u32 sample_rate;
    u32 priority;
    u32 sorting_order;
    u32 channel_count;
    float pitch;
    float volume;
    AudioRendererBiquadFilter biquads[2];
    u32 wavebuf_count;
    s16 wavebuf_head;
    u16 _padding1;
    u32 _padding2;
    const void* extra_params_ptr;
    u64 extra_params_sz;
    u32 dest_mix_id;
    u32 dest_splitter_id;
    AudioRendererWaveBuf wavebufs[4];
    u32 channel_ids[6];
    u8 _padding3[24];
} AudioRendererVoiceInfoIn;

typedef struct {
    u64 played_sample_count;
    u32 num_wavebufs_consumed;
    u32 voice_drops_count;
} AudioRendererVoiceInfoOut;

typedef struct {
    float volume;
    u32 sample_rate;
    u32 buffer_count;
    bool is_used;
    u8 _padding1[3];
    u32 mix_id;
    u32 _padding2;
    u32 node_id;
    u32 _padding3[2];
    float mix[24][24]; // [src_index][dest_index]
    u32 dest_mix_id;
    u32 dest_splitter_id;
    u32 _padding4;
} AudioRendererMixInfoIn;

typedef struct {
    u8 coefficients[16];
} AudioRendererDownMixParameters;

typedef enum {
    AudioRendererSinkType_Invalid,
    AudioRendererSinkType_Device,
    AudioRendererSinkType_CircularBuffer,
} AudioRendererSinkType;

typedef struct {
    char name[255];
    u8 _padding1;
    u32 input_count;
    u8 inputs[6];
    u8 _padding2;
    bool downmix_params_enabled;
    AudioRendererDownMixParameters downmix_params;
} AudioRendererDeviceSinkInfoIn;

typedef struct {
    void* buffer_ptr;
    u32 buffer_sz;
    u32 input_count;
    u32 sample_count;
    u32 last_read_offset;
    PcmFormat sample_format;
    u8 inputs[6];
    u8 _padding2[6];
} AudioRendererCircularBufferSinkInfoIn;

typedef struct {
    AudioRendererSinkType type : 8;
    bool is_used;
    u8 _padding1[2];
    u32 node_id;
    u64 _padding2[3];
    union {
        AudioRendererDeviceSinkInfoIn device_sink;
        AudioRendererCircularBufferSinkInfoIn circular_buffer_sink;
    };
} AudioRendererSinkInfoIn;

typedef struct {
    u32 last_written_offset;
    u32 unk1;
    u64 unk2;
    u64 _padding1[2];
} AudioRendererSinkInfoOut;

typedef struct {
    u32 detail_target;
    u32 _padding1[3];
} AudioRendererPerformanceBufferInfoIn;

typedef struct {
    u32 written_sz;
    u32 _padding1[3];
} AudioRendererPerformanceBufferInfoOut;

static inline u32 audrenGetRevision(void)
{
    extern u32 g_audrenRevision;
    return g_audrenRevision;
}

NX_CONSTEXPR int audrenGetMemPoolCount(const AudioRendererConfig* config)
{
    return config->num_effects + 4 * config->num_voices;
}

NX_CONSTEXPR size_t audrenGetInputParamSize(const AudioRendererConfig* config)
{
    size_t size = 0;
    size += sizeof(AudioRendererUpdateDataHeader);
    size += sizeof(AudioRendererBehaviorInfoIn);
    size += sizeof(AudioRendererMemPoolInfoIn) * audrenGetMemPoolCount(config);
    size += sizeof(AudioRendererChannelInfoIn) * config->num_voices;
    size += sizeof(AudioRendererVoiceInfoIn) * config->num_voices;
    // todo: effects, splitters
    size += sizeof(AudioRendererMixInfoIn) * config->num_mix_objs;
    size += sizeof(AudioRendererSinkInfoIn) * config->num_sinks;
    size += sizeof(AudioRendererPerformanceBufferInfoIn);
    return size;
}

NX_CONSTEXPR size_t audrenGetOutputParamSize(const AudioRendererConfig* config)
{
    size_t size = 0;
    size += sizeof(AudioRendererUpdateDataHeader);
    size += sizeof(AudioRendererMemPoolInfoOut) * audrenGetMemPoolCount(config);
    size += sizeof(AudioRendererVoiceInfoOut) * config->num_voices;
    // todo: effects
    size += sizeof(AudioRendererSinkInfoOut) * config->num_sinks;
    size += sizeof(AudioRendererPerformanceBufferInfoOut);
    size += sizeof(AudioRendererBehaviorInfoOut);
    return size;
}

/// Initialize audren.
Result audrenInitialize(const AudioRendererConfig* config);

/// Exit audren.
void audrenExit(void);

/// Gets the Service object for IAudioRenderer.
Service* audrenGetServiceSession_AudioRenderer(void);

/// Returns event that is signalled on new frame (autoclear=true).
Event* audrenGetFrameEvent(void);

void audrenWaitFrame(void);
Result audrenGetState(u32* out_state);
Result audrenRequestUpdateAudioRenderer(const void* in_param_buf, size_t in_param_buf_size, void* out_param_buf, size_t out_param_buf_size, void* perf_buf, size_t perf_buf_size);
Result audrenStartAudioRenderer(void);
Result audrenStopAudioRenderer(void);
Result audrenSetAudioRendererRenderingTimeLimit(int percent);
