#include "driver_internal.h"

bool audrvVoiceInit(AudioDriver* d, int id, int num_channels, PcmFormat format, int sample_rate)
{
    // Validation
    if (id < 0 || id >= d->config.num_voices || num_channels < 1 || (format != PcmFormat_Int16 && format != PcmFormat_Adpcm))
        return false;

    // Drop the voice if it's used
    if (d->in_voices[id].is_used)
        audrvVoiceDrop(d, id);

    // Make sure there are enough free channels
    if (d->etc->free_channel_count < num_channels)
        return false;

    // Initialize the voice
    d->in_voices[id].id = id;
    d->in_voices[id].node_id = AUDREN_NODEID(1,id,d->etc->voices[id].node_counter++);
    d->in_voices[id].is_new = true;
    d->in_voices[id].is_used = true;
    d->in_voices[id].state = AudioRendererVoicePlayState_Stopped;
    d->in_voices[id].sample_format = format;
    d->in_voices[id].sample_rate = sample_rate;
    d->in_voices[id].channel_count = num_channels;
    d->in_voices[id].pitch = 1.0f;
    d->in_voices[id].volume = 1.0f;
    d->in_voices[id].dest_mix_id = AUDREN_UNUSED_MIX_ID;
    d->in_voices[id].dest_splitter_id = AUDREN_UNUSED_SPLITTER_ID;
    for (int i = 0; i < 4; i ++)
        d->in_voices[id].wavebufs[i].sent_to_server = true;

    // Add the voice to the list of used voices
    int next_voice = d->etc->first_used_voice;
    d->etc->voices[id].next_used_voice = next_voice;
    d->etc->voices[id].prev_next_voice = &d->etc->first_used_voice;
    d->etc->first_used_voice = id;
    if (next_voice >= 0)
        d->etc->voices[next_voice].prev_next_voice = &d->etc->voices[id].next_used_voice;

    // Allocate the channels
    d->etc->free_channel_count -= num_channels;
    for (int i = 0; i < num_channels; i ++) {
        int channel_id = d->etc->first_free_channel;
        d->etc->first_free_channel = d->etc->voices[channel_id].next_free_channel;
        d->in_voices[id].channel_ids[i] = channel_id;
        d->in_channels[channel_id].is_used = true;
        memset(d->in_channels[channel_id].mix, 0, 24*sizeof(float));
    }

    return true;
}

static void _audrvVoiceResetInternalState(AudioDriver* d, int id)
{
    // Clear state
    d->etc->voices[id].num_wavebufs_consumed = 0;
    d->etc->voices[id].voice_drops_count = 0;
    d->etc->voices[id].played_sample_count = 0;

    // Clear wavebuf queue, flagging all wavebuf that were on it as 'done'
    for (AudioDriverWaveBuf* wavebuf = d->etc->voices[id].first_wavebuf; wavebuf; wavebuf = wavebuf->next)
        wavebuf->state = AudioDriverWaveBufState_Done;
    d->etc->voices[id].first_wavebuf = NULL;
    d->etc->voices[id].waiting_wavebuf = NULL;
    d->etc->voices[id].last_wavebuf = NULL;
}

void audrvVoiceDrop(AudioDriver* d, int id)
{
    // Release the channels
    d->etc->free_channel_count += d->in_voices[id].channel_count;
    for (int i = d->in_voices[id].channel_count-1; i >= 0; i --)
    {
        int channel_id = d->in_voices[id].channel_ids[i];
        d->in_channels[channel_id].is_used = false;
        d->etc->voices[channel_id].next_free_channel = d->etc->first_free_channel;
        d->etc->first_free_channel = channel_id;
    }

    // Remove the voice from the list of used voices
    int next_voice = d->etc->voices[id].next_used_voice;
    *d->etc->voices[id].prev_next_voice = next_voice;
    if (next_voice >= 0)
        d->etc->voices[next_voice].prev_next_voice = d->etc->voices[id].prev_next_voice;

    // Clear out state
    memset(&d->in_voices[id], 0, sizeof(AudioRendererVoiceInfoIn));
    _audrvVoiceResetInternalState(d, id);
}

void audrvVoiceStop(AudioDriver* d, int id)
{
    // Reset voice state
    d->in_voices[id].state = AudioRendererVoicePlayState_Stopped;
    d->in_voices[id].is_new = true;
    d->in_voices[id].wavebuf_count = 0;
    d->in_voices[id].wavebuf_head = 0;

    // Reset wavebuf state
    memset(d->in_voices[id].wavebufs, 0, 4*sizeof(AudioRendererWaveBuf));
    for (int i = 0; i < 4; i ++)
        d->in_voices[id].wavebufs[i].sent_to_server = true;

    // Reset internal state
    _audrvVoiceResetInternalState(d, id);
}

bool audrvVoiceIsPlaying(AudioDriver* d, int id)
{
    return d->in_voices[id].state == AudioRendererVoicePlayState_Started && d->etc->voices[id].first_wavebuf;
}

static void _audrvVoiceQueueWaveBufs(AudioDriver* d, int id)
{
    AudioDriverWaveBuf* wavebuf = d->etc->voices[id].waiting_wavebuf;

    while (wavebuf && d->in_voices[id].wavebuf_count < 4) {
        int head = (d->in_voices[id].wavebuf_head + d->in_voices[id].wavebuf_count) & 3;

        wavebuf->state = AudioDriverWaveBufState_Queued;
        wavebuf->sequence_id = d->etc->voices[id].num_wavebufs_consumed + d->in_voices[id].wavebuf_count;
        d->in_voices[id].wavebufs[head].address = wavebuf->data_raw;
        d->in_voices[id].wavebufs[head].size = wavebuf->size;
        d->in_voices[id].wavebufs[head].start_sample_offset = wavebuf->start_sample_offset;
        d->in_voices[id].wavebufs[head].end_sample_offset = wavebuf->end_sample_offset;
        d->in_voices[id].wavebufs[head].is_looping = wavebuf->is_looping;
        d->in_voices[id].wavebufs[head].end_of_stream = false;
        d->in_voices[id].wavebufs[head].sent_to_server = false;
        d->in_voices[id].wavebufs[head].context_addr = wavebuf->context_addr;
        d->in_voices[id].wavebufs[head].context_sz = wavebuf->context_sz;
        d->in_voices[id].wavebuf_count ++;

        wavebuf = wavebuf->next;
    }

    d->etc->voices[id].waiting_wavebuf = wavebuf;
}

bool audrvVoiceAddWaveBuf(AudioDriver* d, int id, AudioDriverWaveBuf* wavebuf)
{
    if (wavebuf->state != AudioDriverWaveBufState_Free && wavebuf->state != AudioDriverWaveBufState_Done)
        return false;
    if (!wavebuf->data_raw || ((uintptr_t)wavebuf->data_raw & (AUDREN_BUFFER_ALIGNMENT-1)))
        return false;
    if ((uintptr_t)wavebuf->context_addr & (AUDREN_BUFFER_ALIGNMENT-1))
        return false;

    wavebuf->state = AudioDriverWaveBufState_Waiting;
    wavebuf->sequence_id = ~(u32)0;
    wavebuf->next = NULL;

    if (d->etc->voices[id].first_wavebuf)
        d->etc->voices[id].last_wavebuf->next = wavebuf;
    else
        d->etc->voices[id].first_wavebuf = wavebuf;
    if (!d->etc->voices[id].waiting_wavebuf)
        d->etc->voices[id].waiting_wavebuf = wavebuf;
    d->etc->voices[id].last_wavebuf = wavebuf;

    _audrvVoiceQueueWaveBufs(d, id);
    return true;
}

u32 audrvVoiceGetWaveBufSeq(AudioDriver* d, int id)
{
    return d->etc->voices[id].num_wavebufs_consumed;
}

u32 audrvVoiceGetPlayedSampleCount(AudioDriver* d, int id)
{
    return d->etc->voices[id].played_sample_count;
}

u32 audrvVoiceGetVoiceDropsCount(AudioDriver* d, int id)
{
    return d->etc->voices[id].voice_drops_count;
}

static inline s16 _audrvIirParamClamp(float param)
{
	float scaled = param*16384.0f;
	s16 result = (s16)scaled;
	if (scaled > 0x7FFF)
		result = 0x7FFF;
	else if (scaled < -0x8000)
		result = -0x8000;
	return result;
}

void audrvVoiceSetBiquadFilter(AudioDriver* d, int id, int biquad_id, float a0, float a1, float a2, float b0, float b1, float b2)
{
    d->in_voices[id].biquads[biquad_id].enable = true;
    d->in_voices[id].biquads[biquad_id].numerator[0] = _audrvIirParamClamp(b0 / a0);
    d->in_voices[id].biquads[biquad_id].numerator[1] = _audrvIirParamClamp(b1 / a0);
    d->in_voices[id].biquads[biquad_id].numerator[2] = _audrvIirParamClamp(b2 / a0);
    d->in_voices[id].biquads[biquad_id].denominator[0] = _audrvIirParamClamp(a1 / a0);
    d->in_voices[id].biquads[biquad_id].denominator[1] = _audrvIirParamClamp(a2 / a0);
}

void _audrvVoiceUpdate(AudioDriver* d, int id, AudioRendererVoiceInfoOut* out_voice)
{
    // Update single-frame flags
    d->in_voices[id].is_new = false;
    for (int i = 0; i < 4; i ++)
        d->in_voices[id].wavebufs[i].sent_to_server = true;

    // Copy state vars
    d->etc->voices[id].played_sample_count = out_voice->played_sample_count;
    d->etc->voices[id].voice_drops_count = out_voice->voice_drops_count;

    // Update wavebuf progress state
    u32 num_wavebufs = out_voice->num_wavebufs_consumed - d->etc->voices[id].num_wavebufs_consumed;
    if (num_wavebufs) {
        d->in_voices[id].wavebuf_count -= num_wavebufs;
        d->in_voices[id].wavebuf_head = (d->in_voices[id].wavebuf_head + num_wavebufs) & 3;
        d->etc->voices[id].num_wavebufs_consumed = out_voice->num_wavebufs_consumed;
    }

    // Dequeue wavebufs
    AudioDriverWaveBuf* wavebuf = d->etc->voices[id].first_wavebuf;
    for (u32 i = 0; wavebuf && i < num_wavebufs; i ++) {
        wavebuf->state = AudioDriverWaveBufState_Done;
        wavebuf = wavebuf->next;
    }

    // Update status of current wavebuf (if exists)
    if (wavebuf) {
        wavebuf->state = AudioDriverWaveBufState_Playing;
        d->etc->voices[id].first_wavebuf = wavebuf;
        _audrvVoiceQueueWaveBufs(d, id);
    } else {
        d->etc->voices[id].first_wavebuf = NULL;
        d->etc->voices[id].last_wavebuf = NULL;
    }
}
