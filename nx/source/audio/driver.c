#include "driver_internal.h"

static inline void _audrvInitConfig(AudioDriver* d, int num_final_mix_channels)
{
    memset(d->etc->in_buf, 0, d->etc->in_buf_size);
    AudioRendererUpdateDataHeader* in_hdr = (AudioRendererUpdateDataHeader*)d->etc->in_buf;
    in_hdr->revision = audrenGetRevision();
    in_hdr->behavior_sz = sizeof(AudioRendererBehaviorInfoIn);
    in_hdr->mempools_sz = sizeof(AudioRendererMemPoolInfoIn) * d->etc->mempool_count;
    in_hdr->channels_sz = sizeof(AudioRendererChannelInfoIn) * d->config.num_voices;
    in_hdr->voices_sz = sizeof(AudioRendererVoiceInfoIn) * d->config.num_voices;
    in_hdr->mixes_sz = sizeof(AudioRendererMixInfoIn) * d->config.num_mix_objs;
    in_hdr->sinks_sz = sizeof(AudioRendererSinkInfoIn) * d->config.num_sinks;
    in_hdr->perfmgr_sz = sizeof(AudioRendererPerformanceBufferInfoIn);
    in_hdr->total_sz = d->etc->in_buf_size;

    d->etc->in_behavior = (AudioRendererBehaviorInfoIn*)(in_hdr+1);
	d->etc->in_behavior->revision = audrenGetRevision();
	d->etc->in_behavior->flags = 0;

    d->in_mempools = (AudioRendererMemPoolInfoIn*)(d->etc->in_behavior+1);
    for (int i = 0; i < d->etc->mempool_count; i ++) {
        d->in_mempools[i].state = AudioRendererMemPoolState_Released;
        d->etc->mempools[i].next_free = i != d->etc->mempool_count-1 ? i+1 : -1;
    }

    d->in_channels = (AudioRendererChannelInfoIn*)(d->in_mempools+d->etc->mempool_count);
    d->in_voices = (AudioRendererVoiceInfoIn*)(d->in_channels+d->config.num_voices);
    for (int i = 0; i < d->config.num_voices; i ++) {
        d->in_channels[i].id = i;
        d->etc->voices[i].next_free_channel = i != d->config.num_voices-1 ? i+1 : -1;
    }

    d->in_mixes = (AudioRendererMixInfoIn*)(d->in_voices+d->config.num_voices);
    d->in_mixes[0].volume = 1.0f;
    d->in_mixes[0].sample_rate = d->config.output_rate == AudioRendererOutputRate_32kHz ? 32000 : 48000;
    d->in_mixes[0].buffer_count = num_final_mix_channels;
    d->in_mixes[0].is_used = true;
    d->in_mixes[0].mix_id = AUDREN_FINAL_MIX_ID;
    d->in_mixes[0].node_id = AUDREN_NODEID(2,0,0);
    d->in_mixes[0].dest_mix_id = AUDREN_UNUSED_MIX_ID;
    d->in_mixes[0].dest_splitter_id = AUDREN_UNUSED_SPLITTER_ID;
    if (d->config.num_mix_objs > 1) {
        d->etc->first_free_mix = 1;
        for (int i = 1; i < d->etc->first_free_mix; i ++)
            d->etc->mixes[i].next_free = i != d->etc->first_free_mix-1 ? i+1 : -1;
    } else {
        d->etc->first_free_mix = -1;
    }

    d->in_sinks = (AudioRendererSinkInfoIn*)(d->in_mixes+d->config.num_mix_objs);
    for (int i = 0; i < d->config.num_sinks; i ++)
        d->etc->sinks[i].next_free = i != d->config.num_sinks-1 ? i+1 : -1;

    d->etc->in_perfbuf = (AudioRendererPerformanceBufferInfoIn*)(d->in_sinks+d->config.num_sinks);
    d->etc->in_perfbuf->detail_target = AUDREN_NODEID(15,0,0); // whatever?
}

Result audrvCreate(AudioDriver* d, const AudioRendererConfig* config, int num_final_mix_channels)
{
    if (num_final_mix_channels < config->num_mix_buffers)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    size_t etc_size = _audrvGetEtcSize(config);
    memset(d, 0, sizeof(AudioDriver));

    d->etc = (AudioDriverEtc*)malloc(etc_size);
    if (!d->etc)
        goto _error0;

    memset(d->etc, 0, etc_size);
    d->config = *config;
    d->etc->mempool_count = audrenGetMemPoolCount(config);
    d->etc->free_channel_count = config->num_voices;
    d->etc->free_mix_buffer_count = config->num_mix_buffers - num_final_mix_channels;
    d->etc->first_used_voice = -1;
    d->etc->mempools = (AudioDriverEtcMemPool*)(d->etc+1);
    d->etc->voices = (AudioDriverEtcVoice*)(d->etc->mempools+d->etc->mempool_count);
    d->etc->mixes = (AudioDriverEtcMix*)(d->etc->voices+config->num_voices);
    d->etc->sinks = (AudioDriverEtcSink*)(d->etc->mixes+config->num_mix_objs);

    d->etc->out_buf_size = audrenGetOutputParamSize(config);
    d->etc->out_buf = memalign(AUDREN_OUTPUT_PARAM_ALIGNMENT, d->etc->out_buf_size);
    if (!d->etc->out_buf)
        goto _error1;

    d->etc->in_buf_size = audrenGetInputParamSize(config);
    d->etc->in_buf = memalign(AUDREN_INPUT_PARAM_ALIGNMENT, d->etc->in_buf_size);
    if (!d->etc->in_buf)
        goto _error2;

    _audrvInitConfig(d, num_final_mix_channels);
    return 0;

_error2:
    free(d->etc->out_buf);
_error1:
    free(d->etc);
    d->etc = NULL;
_error0:
    return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
}

Result audrvUpdate(AudioDriver* d)
{
    for (int i = d->etc->first_used_voice, j = 0; i >= 0; i = d->etc->voices[i].next_used_voice, j++)
        d->in_voices[i].sorting_order = j;

    Result rc = audrenRequestUpdateAudioRenderer(d->etc->in_buf, d->etc->in_buf_size, d->etc->out_buf, d->etc->out_buf_size, NULL, 0);
    if (R_FAILED(rc))
        return rc;

    AudioRendererUpdateDataHeader* out_hdr = (AudioRendererUpdateDataHeader*)d->etc->out_buf;

    AudioRendererMemPoolInfoOut* out_mempools = (AudioRendererMemPoolInfoOut*)(out_hdr+1);
    if (out_hdr->mempools_sz != d->etc->mempool_count*sizeof(AudioRendererMemPoolInfoOut))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    for (int i = 0; i < d->etc->mempool_count; i ++)
    {
        // todo: this is supposed to be more complex
        AudioRendererMemPoolState new_state = out_mempools[i].new_state;
        if (new_state != AudioRendererMemPoolState_Invalid)
            d->in_mempools[i].state = new_state;
    }

    AudioRendererVoiceInfoOut* out_voices = (AudioRendererVoiceInfoOut*)(out_mempools+d->etc->mempool_count);
    if (out_hdr->voices_sz != d->config.num_voices*sizeof(AudioRendererVoiceInfoOut))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    for (int i = d->etc->first_used_voice; i >= 0; i = d->etc->voices[i].next_used_voice)
        _audrvVoiceUpdate(d, i, &out_voices[i]);

    return 0;
}

void audrvClose(AudioDriver* d)
{
    free(d->etc->in_buf);
    free(d->etc->out_buf);
    free(d->etc);
    memset(d, 0, sizeof(AudioDriver));
}
