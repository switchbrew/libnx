#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "kernel/tmem.h"
#include "services/hwopus.h"
#include "services/sm.h"

static Result _hwopusInitialize(Service* srv, Service* out_srv, TransferMemory *tmem, s32 SampleRate, s32 ChannelCount);
static Result _hwopusGetWorkBufferSize(Service* srv, u32 *size, s32 SampleRate, s32 ChannelCount);

Result hwopusDecoderInitialize(HwopusDecoder* decoder, s32 SampleRate, s32 ChannelCount) {
    Result rc=0;
    u32 size=0;

    if (serviceIsActive(&decoder->s))
        return 0;

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

Result hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedSampleCount, u32 *DecodedDataSize, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size) {
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
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&decoder->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 DecodedSampleCount;
            u32 DecodedDataSize;
        } *resp;

        serviceIpcParse(&decoder->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && DecodedSampleCount) *DecodedSampleCount = resp->DecodedSampleCount;
        if (R_SUCCEEDED(rc) && DecodedDataSize) *DecodedDataSize = resp->DecodedDataSize;
    }

    return rc;
}
