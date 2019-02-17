#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/tmem.h"
#include "services/hwopus.h"
#include "services/sm.h"
#include "runtime/hosversion.h"

static Result _hwopusInitialize(Service* srv, Service* out_srv, TransferMemory *tmem, s32 SampleRate, s32 ChannelCount);
static Result _hwopusGetWorkBufferSize(Service* srv, u32 *size, s32 SampleRate, s32 ChannelCount);
static Result _hwopusOpenHardwareOpusDecoderForMultiStream(Service* srv, Service* out_srv, TransferMemory *tmem, HwopusMultistreamState *state);
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

static Result _hwopusInitialize(Service* srv, Service* out_srv, TransferMemory *tmem, s32 SampleRate, s32 ChannelCount) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 val;
        u32 size;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->val = (u64)SampleRate | ((u64)ChannelCount<<32);
    raw->size = tmem->size;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(out_srv, srv, &r, 0);
        }
    }

    return rc;
}

static Result _hwopusGetWorkBufferSize(Service* srv, u32 *size, s32 SampleRate, s32 ChannelCount) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 val;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->val = (u64)SampleRate | ((u64)ChannelCount<<32);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 size;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

static Result _hwopusOpenHardwareOpusDecoderForMultiStream(Service* srv, Service* out_srv, TransferMemory *tmem, HwopusMultistreamState *state) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, tmem->handle);
    ipcAddSendStatic(&c, state, sizeof(HwopusMultistreamState), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 size;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->size = tmem->size;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(out_srv, srv, &r, 0);
        }
    }

    return rc;
}

static Result _hwopusGetWorkBufferSizeForMultiStream(Service* srv, u32 *size, HwopusMultistreamState *state) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendStatic(&c, state, sizeof(HwopusMultistreamState), 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 size;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size) *size = resp->size;
    }

    return rc;
}

Result hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
    if (hosversionAtLeast(6,0,0)) return _hwopusDecodeInterleaved(decoder, DecodedDataSize, DecodedSampleCount, NULL, 0, opusin, opusin_size, pcmbuf, pcmbuf_size);
    if (hosversionAtLeast(4,0,0)) return _hwopusDecodeInterleavedWithPerfOld(decoder, DecodedDataSize, DecodedSampleCount, NULL, opusin, opusin_size, pcmbuf, pcmbuf_size);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, opusin, opusin_size, BufferType_Normal);
    ipcAddRecvBuffer(&c, pcmbuf, pcmbuf_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&decoder->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = decoder->multistream==0 ? 0 : 2;

    Result rc = serviceIpcDispatch(&decoder->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 DecodedDataSize;
            s32 DecodedSampleCount;
        } *resp;

        serviceIpcParse(&decoder->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = resp->DecodedSampleCount;
        if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = resp->DecodedDataSize;
    }

    return rc;
}

static Result _hwopusDecodeInterleavedWithPerfOld(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, u64 *perf, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, opusin, opusin_size, BufferType_Normal);
    ipcAddRecvBuffer(&c, pcmbuf, pcmbuf_size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&decoder->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = decoder->multistream==0 ? 4 : 5;

    Result rc = serviceIpcDispatch(&decoder->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 DecodedDataSize;
            s32 DecodedSampleCount;
            u64 perf;
        } *resp;

        serviceIpcParse(&decoder->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = resp->DecodedSampleCount;
        if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = resp->DecodedDataSize;
        if (R_SUCCEEDED(rc) && perf) *perf = resp->perf;
    }

    return rc;
}

static Result _hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, u64 *perf, bool reset_context, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, opusin, opusin_size, BufferType_Normal);
    ipcAddRecvBuffer(&c, pcmbuf, pcmbuf_size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = serviceIpcPrepareHeader(&decoder->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = decoder->multistream==0 ? 6 : 7;
    raw->flag = reset_context!=0;

    Result rc = serviceIpcDispatch(&decoder->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 DecodedDataSize;
            s32 DecodedSampleCount;
            u64 perf;
        } *resp;

        serviceIpcParse(&decoder->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = resp->DecodedSampleCount;
        if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = resp->DecodedDataSize;
        if (R_SUCCEEDED(rc) && perf) *perf = resp->perf;
    }

    return rc;
}
