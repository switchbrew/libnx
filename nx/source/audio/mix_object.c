#include "driver_internal.h"

int audrvMixAdd(AudioDriver* d, int sample_rate, int num_channels)
{
    if (num_channels < 1 || num_channels > 24)
        return -1;

    if (d->etc->free_mix_buffer_count < num_channels)
        return -1;

    int id = d->etc->first_free_mix;
    if (id < 0)
        return -1;

    d->etc->free_mix_buffer_count -= num_channels;
    d->etc->first_free_mix = d->etc->mixes[id].next_free;
    d->in_mixes[id].volume = 1.0f;
    d->in_mixes[id].sample_rate = sample_rate;
    d->in_mixes[id].buffer_count = num_channels;
    d->in_mixes[id].is_used = true;
    d->in_mixes[id].mix_id = id;
    d->in_mixes[id].node_id = AUDREN_NODEID(2,id,0);
    d->in_mixes[id].dest_mix_id = AUDREN_UNUSED_MIX_ID;
    d->in_mixes[id].dest_splitter_id = AUDREN_UNUSED_SPLITTER_ID;

    return id;
}

void audrvMixRemove(AudioDriver* d, int id)
{
    if (id <= AUDREN_FINAL_MIX_ID || id >= d->config.num_mix_objs)
        return;
    if (!d->in_mixes[id].is_used)
        return;

    d->etc->free_mix_buffer_count += d->in_mixes[id].buffer_count;
    d->etc->mixes[id].next_free = d->etc->first_free_mix;
    d->etc->first_free_mix = id;
    memset(&d->in_mixes[id], 0, sizeof(AudioRendererMixInfoIn));
}
