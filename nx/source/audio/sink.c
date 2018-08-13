#include "driver_internal.h"

int audrvDeviceSinkAdd(AudioDriver* d, const char* device_name, int num_channels, const u8* channel_ids)
{
    if (!device_name || num_channels < 1 || num_channels > 6 || !channel_ids)
        return -1;

    int id = d->etc->first_free_sink;
    if (id < 0)
        return -1;

    d->etc->first_free_sink = d->etc->sinks[id].next_free;
    d->in_sinks[id].type = AudioRendererSinkType_Device;
    d->in_sinks[id].is_used = true;
    d->in_sinks[id].node_id = AUDREN_NODEID(3,id,0);
    strncpy(d->in_sinks[id].device_sink.name, device_name, sizeof(d->in_sinks[id].device_sink.name)-1);
    d->in_sinks[id].device_sink.input_count = num_channels;
    memcpy(d->in_sinks[id].device_sink.inputs, channel_ids, num_channels);

    return id;
}

void audrvSinkRemove(AudioDriver* d, int id)
{
    if (id < 0 || id >= d->config.num_sinks)
        return;
    if (!d->in_sinks[id].is_used)
        return;

    d->etc->sinks[id].next_free = d->etc->first_free_sink;
    d->etc->first_free_sink = id;
    memset(&d->in_sinks[id], 0, sizeof(AudioRendererSinkInfoIn));
}
