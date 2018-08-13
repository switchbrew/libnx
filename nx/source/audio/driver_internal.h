#pragma once
#include <malloc.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "services/audren.h"
#include "audio/driver.h"

typedef struct {
    int next_free;
    int padding;
} AudioDriverEtcMemPool;

typedef struct {
    int* prev_next_voice;
    int next_free_channel;
    int next_used_voice;
    u16 node_counter;
    u32 num_wavebufs_consumed;
    u32 voice_drops_count;
    u64 played_sample_count;
    AudioDriverWaveBuf* first_wavebuf;
    AudioDriverWaveBuf* waiting_wavebuf;
    AudioDriverWaveBuf* last_wavebuf;
} AudioDriverEtcVoice;

typedef struct {
    int next_free;
    int padding;
} AudioDriverEtcMix;

typedef struct {
    int next_free;
    int padding;
} AudioDriverEtcSink;

struct AudioDriverEtc {
    int mempool_count;
    int free_channel_count;
    int free_mix_buffer_count;
    int first_used_voice;
    int first_free_mempool;
    int first_free_channel;
    int first_free_mix;
    int first_free_sink;
    AudioDriverEtcMemPool* mempools;
    AudioDriverEtcVoice* voices;
    AudioDriverEtcMix* mixes;
    AudioDriverEtcSink* sinks;
    AudioRendererBehaviorInfoIn* in_behavior;
    AudioRendererPerformanceBufferInfoIn* in_perfbuf;
    void* in_buf;
    size_t in_buf_size;
    void* out_buf;
    size_t out_buf_size;
};

static inline size_t _audrvGetEtcSize(const AudioRendererConfig* config)
{
    size_t size = 0;
    size += sizeof(AudioDriverEtc);
    size += sizeof(AudioDriverEtcMemPool) * audrenGetMemPoolCount(config);
    size += sizeof(AudioDriverEtcVoice) * config->num_voices;
    size += sizeof(AudioDriverEtcMix) * config->num_mix_objs;
    size += sizeof(AudioDriverEtcSink) * config->num_sinks;
    return size;
}

void _audrvVoiceUpdate(AudioDriver* d, int id, AudioRendererVoiceInfoOut* out_voice);
