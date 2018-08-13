/**
 * @file driver.h
 * @brief Audio driver (audren wrapper).
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../services/audren.h"

typedef struct AudioDriverEtc AudioDriverEtc;

typedef struct {
    AudioDriverEtc* etc;
    AudioRendererConfig config;
    AudioRendererMemPoolInfoIn* in_mempools;
    AudioRendererChannelInfoIn* in_channels;
    AudioRendererVoiceInfoIn* in_voices;
    AudioRendererMixInfoIn* in_mixes;
    AudioRendererSinkInfoIn* in_sinks;
} AudioDriver;

Result audrvCreate(AudioDriver* d, const AudioRendererConfig* config, int num_final_mix_channels);
Result audrvUpdate(AudioDriver* d);
void audrvClose(AudioDriver* d);

//-----------------------------------------------------------------------------

int audrvMemPoolAdd(AudioDriver* d, void* buffer, size_t size);
bool audrvMemPoolRemove(AudioDriver* d, int id);
bool audrvMemPoolAttach(AudioDriver* d, int id);
bool audrvMemPoolDetach(AudioDriver* d, int id);

//-----------------------------------------------------------------------------

typedef enum {
    AudioDriverWaveBufState_Free,
    AudioDriverWaveBufState_Waiting,
    AudioDriverWaveBufState_Queued,
    AudioDriverWaveBufState_Playing,
    AudioDriverWaveBufState_Done,
} AudioDriverWaveBufState;

typedef struct AudioDriverWaveBuf AudioDriverWaveBuf;

struct AudioDriverWaveBuf {
    union {
        s16*        data_pcm16;
        u8*         data_adpcm;
        const void* data_raw;
    };
    u64 size;
    s32 start_sample_offset;
    s32 end_sample_offset;
    const void* context_addr;
    u64 context_sz;
    AudioDriverWaveBufState state : 8;
    bool is_looping;
    u32 sequence_id;
    AudioDriverWaveBuf* next;
};

bool audrvVoiceInit(AudioDriver* d, int id, int num_channels, PcmFormat format, int sample_rate);
void audrvVoiceDrop(AudioDriver* d, int id);
void audrvVoiceStop(AudioDriver* d, int id);
bool audrvVoiceIsPlaying(AudioDriver* d, int id);
bool audrvVoiceAddWaveBuf(AudioDriver* d, int id, AudioDriverWaveBuf* wavebuf);
u32 audrvVoiceGetWaveBufSeq(AudioDriver* d, int id);
u32 audrvVoiceGetPlayedSampleCount(AudioDriver* d, int id);
u32 audrvVoiceGetVoiceDropsCount(AudioDriver* d, int id);
void audrvVoiceSetBiquadFilter(AudioDriver* d, int id, int biquad_id, float a0, float a1, float a2, float b0, float b1, float b2);

static inline void audrvVoiceSetExtraParams(AudioDriver* d, int id, const void* params, size_t params_size)
{
    d->in_voices[id].extra_params_ptr = params;
    d->in_voices[id].extra_params_sz = params_size;
}

static inline void audrvVoiceSetDestinationMix(AudioDriver* d, int id, int mix_id)
{
    d->in_voices[id].dest_mix_id = mix_id;
    d->in_voices[id].dest_splitter_id = AUDREN_UNUSED_SPLITTER_ID;
}

static inline void audrvVoiceSetMixFactor(AudioDriver* d, int id, float factor, int src_channel_id, int dest_channel_id)
{
    int channel_id = d->in_voices[id].channel_ids[src_channel_id];
    d->in_channels[channel_id].mix[dest_channel_id] = factor;
}

static inline void audrvVoiceSetVolume(AudioDriver* d, int id, float volume)
{
    d->in_voices[id].volume = volume;
}

static inline void audrvVoiceSetPitch(AudioDriver* d, int id, float pitch)
{
    d->in_voices[id].pitch = pitch;
}

static inline void audrvVoiceSetPriority(AudioDriver* d, int id, int priority)
{
    d->in_voices[id].priority = priority;
}

static inline void audrvVoiceClearBiquadFilter(AudioDriver* d, int id, int biquad_id)
{
    d->in_voices[id].biquads[biquad_id].enable = false;
}

static inline void audrvVoiceSetPaused(AudioDriver* d, int id, bool paused)
{
    d->in_voices[id].state = paused ? AudioRendererVoicePlayState_Paused : AudioRendererVoicePlayState_Started;
}

static inline void audrvVoiceStart(AudioDriver* d, int id)
{
    audrvVoiceSetPaused(d, id, false);
}

//-----------------------------------------------------------------------------

int audrvMixAdd(AudioDriver* d, int sample_rate, int num_channels);
void audrvMixRemove(AudioDriver* d, int id);

static inline void audrvMixSetDestinationMix(AudioDriver* d, int id, int mix_id)
{
    d->in_mixes[id].dest_mix_id = mix_id;
    d->in_mixes[id].dest_splitter_id = AUDREN_UNUSED_SPLITTER_ID;
}

static inline void audrvMixSetMixFactor(AudioDriver* d, int id, float factor, int src_channel_id, int dest_channel_id)
{
    d->in_mixes[id].mix[src_channel_id][dest_channel_id] = factor;
}

static inline void audrvMixSetVolume(AudioDriver* d, int id, float volume)
{
    d->in_mixes[id].volume = volume;
}

//-----------------------------------------------------------------------------

int audrvDeviceSinkAdd(AudioDriver* d, const char* device_name, int num_channels, const u8* channel_ids);
void audrvSinkRemove(AudioDriver* d, int id);
