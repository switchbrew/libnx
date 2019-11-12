#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/hwopus.h"
#include "runtime/hosversion.h"

static Result _hwopusInitialize(Service* srv, Service* srv_out, TransferMemory *tmem, s32 SampleRate, s32 ChannelCount);
static Result _hwopusGetWorkBufferSize(Service* srv, u32 *size, s32 SampleRate, s32 ChannelCount);
static Result _hwopusOpenHardwareOpusDecoderForMultiStream(Service* srv, Service* srv_out, TransferMemory *tmem, HwopusMultistreamState *state);
static Result _hwopusGetWorkBufferSizeForMultiStream(Service* srv, u32 *size, HwopusMultistreamState *state);

static Result _hwopusDecodeInterleavedWithPerfOld(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, u64 *perf, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size);
static Result _hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, u64 *perf, bool reset_context, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size);

Result hwopusDecoderInitialize(HwopusDecoder* decoder, s32 SampleRate, s32 ChannelCount) {
    Result rc=0;
    u32 size=0;

    if (serviceIsActive(&decoder->s))
        return 0;

    decoder->multistream = false;

    Service hwopusMgrSrv;
    rc = smGetService(&hwopusMgrSrv, "hwopus");
    if (R_SUCCEEDED(rc)) {
        rc = _hwopusGetWorkBufferSize(&hwopusMgrSrv, &size, SampleRate, ChannelCount);
        if (R_SUCCEEDED(rc)) size = (size + 0xfff) & ~0xfff;

        if (R_SUCCEEDED(rc)) rc = tmemCreate(&decoder->tmem, size, Perm_None);

        if (R_SUCCEEDED(rc)) {
            rc = _hwopusInitialize(&hwopusMgrSrv, &decoder->s, &decoder->tmem, SampleRate, ChannelCount);
            if (R_FAILED(rc)) tmemClose(&decoder->tmem);
        }

        serviceClose(&hwopusMgrSrv);
    }

    return rc;
}

Result hwopusDecoderMultistreamInitialize(HwopusDecoder* decoder, s32 SampleRate, s32 ChannelCount, s32 TotalStreamCount, s32 StereoStreamCount, u8 *channel_mapping) {
    Result rc=0;
    u32 size=0;
    HwopusMultistreamState state;

    if (serviceIsActive(&decoder->s))
        return 0;

    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (ChannelCount < 0 || ChannelCount > sizeof(state.channel_mapping))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    decoder->multistream = true;

    memset(&state, 0, sizeof(HwopusMultistreamState));
    state.SampleRate = SampleRate;
    state.ChannelCount = ChannelCount;
    state.TotalStreamCount = TotalStreamCount;
    state.StereoStreamCount = StereoStreamCount;

    Service hwopusMgrSrv;
    rc = smGetService(&hwopusMgrSrv, "hwopus");
    if (R_SUCCEEDED(rc)) {
        rc = _hwopusGetWorkBufferSizeForMultiStream(&hwopusMgrSrv, &size, &state);
        if (R_SUCCEEDED(rc)) size = (size + 0xfff) & ~0xfff;

        if (R_SUCCEEDED(rc)) rc = tmemCreate(&decoder->tmem, size, Perm_None);

        if (R_SUCCEEDED(rc)) {
            memcpy(state.channel_mapping, channel_mapping, ChannelCount);

            rc = _hwopusOpenHardwareOpusDecoderForMultiStream(&hwopusMgrSrv, &decoder->s, &decoder->tmem, &state);
            if (R_FAILED(rc)) tmemClose(&decoder->tmem);
        }

        serviceClose(&hwopusMgrSrv);
    }

    return rc;
}

void hwopusDecoderExit(HwopusDecoder* decoder) {
    serviceClose(&decoder->s);
    tmemClose(&decoder->tmem);
}

static Result _hwopusInitialize(Service* srv, Service* srv_out, TransferMemory *tmem, s32 SampleRate, s32 ChannelCount) {
    const struct {
        u64 val;
        u32 size;
    } in = { (u64)SampleRate | ((u64)ChannelCount<<32), tmem->size };

    return serviceDispatchIn(srv, 0, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _hwopusGetWorkBufferSize(Service* srv, u32 *size, s32 SampleRate, s32 ChannelCount) {
    u64 val = (u64)SampleRate | ((u64)ChannelCount<<32);;
    return serviceDispatchInOut(srv, 1, val, *size);
}

static Result _hwopusOpenHardwareOpusDecoderForMultiStream(Service* srv, Service* srv_out, TransferMemory *tmem, HwopusMultistreamState *state) {
    u64 tmp=tmem->size;
    return serviceDispatchIn(srv, 2, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { state, sizeof(HwopusMultistreamState) } },
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _hwopusGetWorkBufferSizeForMultiStream(Service* srv, u32 *size, HwopusMultistreamState *state) {
    return serviceDispatchOut(srv, 3, *size,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { state, sizeof(HwopusMultistreamState) } },
    );
}

Result hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
    if (hosversionAtLeast(6,0,0)) return _hwopusDecodeInterleaved(decoder, DecodedDataSize, DecodedSampleCount, NULL, 0, opusin, opusin_size, pcmbuf, pcmbuf_size);
    if (hosversionAtLeast(4,0,0)) return _hwopusDecodeInterleavedWithPerfOld(decoder, DecodedDataSize, DecodedSampleCount, NULL, opusin, opusin_size, pcmbuf, pcmbuf_size);

    struct {
        s32 DecodedDataSize;
        s32 DecodedSampleCount;
    } out;

    Result rc = serviceDispatchOut(&decoder->s, decoder->multistream==0 ? 0 : 2, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { opusin, opusin_size },
            { pcmbuf, pcmbuf_size },
        },
    );
    if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = out.DecodedSampleCount;
    if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = out.DecodedDataSize;
    return rc;
}

static Result _hwopusDecodeInterleavedWithPerfOld(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, u64 *perf, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
    struct {
        s32 DecodedDataSize;
        s32 DecodedSampleCount;
        u64 perf;
    } out;

    Result rc = serviceDispatchOut(&decoder->s, decoder->multistream==0 ? 4 : 5, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { opusin, opusin_size },
            { pcmbuf, pcmbuf_size },
        },
    );
    if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = out.DecodedSampleCount;
    if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = out.DecodedDataSize;
    if (R_SUCCEEDED(rc) && perf) *perf = out.perf;
    return rc;
}

static Result _hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, u64 *perf, bool reset_context, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
    struct {
        s32 DecodedDataSize;
        s32 DecodedSampleCount;
        u64 perf;
    } out;

    u8 tmp = reset_context!=0;
    Result rc = serviceDispatchInOut(&decoder->s, decoder->multistream==0 ? 6 : 7, tmp, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { opusin, opusin_size },
            { pcmbuf, pcmbuf_size },
        },
    );
    if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = out.DecodedSampleCount;
    if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = out.DecodedDataSize;
    if (R_SUCCEEDED(rc) && perf) *perf = out.perf;
    return rc;
}
