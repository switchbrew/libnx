/**
 * @file tipc.h
 * @brief Tiny IPC protocol
 * @author fincs
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "hipc.h"
#include "cmif.h"
#include "service.h"

typedef enum TipcCommandType {
    TipcCommandType_Close = 15,
} TipcCommandType;

/// tipc Service object structure
typedef struct TipcService {
    Handle session;
} TipcService;

typedef struct TipcDispatchParams {
    SfBufferAttrs buffer_attrs;
    SfBuffer buffers[8];

    bool in_send_pid;

    u32 in_num_handles;
    Handle in_handles[8];

    u32 out_num_objects;
    TipcService* out_objects;

    SfOutHandleAttrs out_handle_attrs;
    Handle* out_handles;
} TipcDispatchParams;

typedef struct TipcRequestFormat {
    u32 request_id;
    u32 data_size;
    u32 num_in_buffers;
    u32 num_out_buffers;
    u32 num_inout_buffers;
    u32 num_handles;
    u32 send_pid;
} TipcRequestFormat;

/**
 * @brief Creates a tipc service object from an IPC session handle.
 * @param[out] s TIPC service object.
 * @param[in] h IPC session handle.
 */
NX_CONSTEXPR void tipcCreate(TipcService* s, Handle h) {
    s->session = h;
}

/**
 * @brief Closes a tipc service.
 * @param[in] s TIPC service object.
 */
NX_INLINE void tipcClose(TipcService* s)
{
    hipcMakeRequestInline(armGetTls(), .type = TipcCommandType_Close);
    svcSendSyncRequest(s->session);
    svcCloseHandle(s->session);
    *s = (TipcService){};
}

NX_CONSTEXPR void tipcRequestInBuffer(HipcRequest* req, const void* buffer, size_t size, HipcBufferMode mode)
{
    *req->send_buffers++ = hipcMakeBuffer(buffer, size, mode);
}

NX_CONSTEXPR void tipcRequestOutBuffer(HipcRequest* req, void* buffer, size_t size, HipcBufferMode mode)
{
    *req->recv_buffers++ = hipcMakeBuffer(buffer, size, mode);
}

NX_CONSTEXPR void tipcRequestInOutBuffer(HipcRequest* req, void* buffer, size_t size, HipcBufferMode mode)
{
    *req->exch_buffers++ = hipcMakeBuffer(buffer, size, mode);
}

NX_CONSTEXPR void tipcRequestHandle(HipcRequest* req, Handle handle)
{
    *req->copy_handles++ = handle;
}

NX_CONSTEXPR void _tipcRequestFormatProcessBuffer(TipcRequestFormat* fmt, u32 attr)
{
    if (!attr) return;
    const bool is_in  = (attr & SfBufferAttr_In)  != 0;
    const bool is_out = (attr & SfBufferAttr_Out) != 0;

    if (attr & SfBufferAttr_HipcMapAlias) {
        if (is_in && is_out)
            fmt->num_inout_buffers ++;
        else if (is_in)
            fmt->num_in_buffers ++;
        else if (is_out)
            fmt->num_out_buffers ++;
    }
}

NX_CONSTEXPR void _tipcRequestProcessBuffer(HipcRequest* req, const SfBuffer* buf, u32 attr)
{
    if (!attr) return;
    const bool is_in  = (attr & SfBufferAttr_In);
    const bool is_out = (attr & SfBufferAttr_Out);

    if (attr & SfBufferAttr_HipcMapAlias) {
        HipcBufferMode mode = HipcBufferMode_Normal;
        if (attr & SfBufferAttr_HipcMapTransferAllowsNonSecure)
            mode = HipcBufferMode_NonSecure;
        if (attr & SfBufferAttr_HipcMapTransferAllowsNonDevice)
            mode = HipcBufferMode_NonDevice;

        if (is_in && is_out)
            tipcRequestInOutBuffer(req, (void*)buf->ptr, buf->size, mode);
        else if (is_in)
            tipcRequestInBuffer(req, buf->ptr, buf->size, mode);
        else if (is_out)
            tipcRequestOutBuffer(req, (void*)buf->ptr, buf->size, mode);
    }
}

NX_INLINE void* tipcMakeRequest(
    u32 request_id, u32 data_size, bool send_pid,
    const SfBufferAttrs buffer_attrs, const SfBuffer* buffers,
    u32 num_handles, const Handle* handles) {
    TipcRequestFormat fmt = {};
    fmt.request_id = request_id + 16;
    fmt.data_size = data_size;
    fmt.num_handles = num_handles;
    fmt.send_pid = send_pid;

    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr0);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr1);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr2);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr3);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr4);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr5);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr6);
    _tipcRequestFormatProcessBuffer(&fmt, buffer_attrs.attr7);

    HipcRequest req = hipcMakeRequestInline(armGetTls(),
        .type             = fmt.request_id,
        .num_send_statics = 0,
        .num_send_buffers = fmt.num_in_buffers,
        .num_recv_buffers = fmt.num_out_buffers,
        .num_exch_buffers = fmt.num_inout_buffers,
        .num_data_words   = (data_size + 3) / 4,
        .num_recv_statics = 0,
        .send_pid         = fmt.send_pid,
        .num_copy_handles = fmt.num_handles,
        .num_move_handles = 0,
    );

    for (u32 i = 0; i < num_handles; i ++)
        tipcRequestHandle(&req, handles[i]);

    _tipcRequestProcessBuffer(&req, &buffers[0], buffer_attrs.attr0);
    _tipcRequestProcessBuffer(&req, &buffers[1], buffer_attrs.attr1);
    _tipcRequestProcessBuffer(&req, &buffers[2], buffer_attrs.attr2);
    _tipcRequestProcessBuffer(&req, &buffers[3], buffer_attrs.attr3);
    _tipcRequestProcessBuffer(&req, &buffers[4], buffer_attrs.attr4);
    _tipcRequestProcessBuffer(&req, &buffers[5], buffer_attrs.attr5);
    _tipcRequestProcessBuffer(&req, &buffers[6], buffer_attrs.attr6);
    _tipcRequestProcessBuffer(&req, &buffers[7], buffer_attrs.attr7);

    return req.data_words;
}

NX_CONSTEXPR Handle tipcResponseGetCopyHandle(HipcResponse* res)
{
    return *res->copy_handles++;
}

NX_CONSTEXPR Handle tipcResponseGetMoveHandle(HipcResponse* res)
{
    return *res->move_handles++;
}

NX_CONSTEXPR void _tipcResponseGetHandle(HipcResponse* res, SfOutHandleAttr type, Handle* out)
{
    switch (type) {
        default:
        case SfOutHandleAttr_None:
            break;
        case SfOutHandleAttr_HipcCopy:
            *out = tipcResponseGetCopyHandle(res);
            break;
        case SfOutHandleAttr_HipcMove:
            *out = tipcResponseGetMoveHandle(res);
            break;
    }
}

NX_INLINE Result tipcParseResponse(
    u32 out_size, void** out_data,
    u32 num_out_objects, TipcService* out_objects,
    const SfOutHandleAttrs out_handle_attrs, Handle* out_handles
) {

    HipcResponse res = hipcParseResponse(armGetTls());

    Result rc = *res.data_words++;
    if (R_FAILED(rc))
        return rc;

    if (out_size)
        *out_data = res.data_words;

    for (u32 i = 0; i < num_out_objects; i ++) {
        tipcCreate(&out_objects[i], tipcResponseGetMoveHandle(&res));
    }

    _tipcResponseGetHandle(&res, out_handle_attrs.attr0, &out_handles[0]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr1, &out_handles[1]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr2, &out_handles[2]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr3, &out_handles[3]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr4, &out_handles[4]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr5, &out_handles[5]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr6, &out_handles[6]);
    _tipcResponseGetHandle(&res, out_handle_attrs.attr7, &out_handles[7]);

    return 0;
}

NX_INLINE Result tipcDispatchImpl(
    TipcService* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    TipcDispatchParams disp
)
{
    void* in = tipcMakeRequest(request_id,
        in_data_size, disp.in_send_pid,
        disp.buffer_attrs, disp.buffers,
        disp.in_num_handles, disp.in_handles);

    if (in_data_size)
        __builtin_memcpy(in, in_data, in_data_size);

    Result rc = svcSendSyncRequest(s->session);
    if (R_SUCCEEDED(rc)) {
        void* out = NULL;
        rc = tipcParseResponse(out_data_size, &out,
            disp.out_num_objects, disp.out_objects,
            disp.out_handle_attrs, disp.out_handles);

        if (R_SUCCEEDED(rc) && out_data && out_data_size)
            __builtin_memcpy(out_data, out, out_data_size);
    }

    return rc;
}

#define tipcDispatch(_s,_rid,...) \
    tipcDispatchImpl((_s),(_rid),NULL,0,NULL,0,(TipcDispatchParams){ __VA_ARGS__ })

#define tipcDispatchIn(_s,_rid,_in,...) \
    tipcDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(TipcDispatchParams){ __VA_ARGS__ })

#define tipcDispatchOut(_s,_rid,_out,...) \
    tipcDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(TipcDispatchParams){ __VA_ARGS__ })

#define tipcDispatchInOut(_s,_rid,_in,_out,...) \
    tipcDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(TipcDispatchParams){ __VA_ARGS__ })
