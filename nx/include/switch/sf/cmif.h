/**
 * @file cmif.h
 * @brief Common Message Interface Framework protocol
 * @author fincs
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "hipc.h"

#define CMIF_IN_HEADER_MAGIC  0x49434653 // "SFCI"
#define CMIF_OUT_HEADER_MAGIC 0x4F434653 // "SFCO"

typedef enum CmifCommandType {
    CmifCommandType_Invalid            = 0,
    CmifCommandType_LegacyRequest      = 1,
    CmifCommandType_Close              = 2,
    CmifCommandType_LegacyControl      = 3,
    CmifCommandType_Request            = 4,
    CmifCommandType_Control            = 5,
    CmifCommandType_RequestWithContext = 6,
    CmifCommandType_ControlWithContext = 7,
} CmifCommandType;

typedef enum CmifDomainRequestType {
    CmifDomainRequestType_Invalid     = 0,
    CmifDomainRequestType_SendMessage = 1,
    CmifDomainRequestType_Close       = 2,
} CmifDomainRequestType;

typedef struct CmifInHeader {
    u32 magic;
    u32 version;
    u32 command_id;
    u32 token;
} CmifInHeader;

typedef struct CmifOutHeader {
    u32 magic;
    u32 version;
    Result result;
    u32 token;
} CmifOutHeader;

typedef struct CmifDomainInHeader {
    u8  type;
    u8  num_in_objects;
    u16 data_size;
    u32 object_id;
    u32 padding;
    u32 token;
} CmifDomainInHeader;

typedef struct CmifDomainOutHeader {
    u32 num_out_objects;
    u32 padding[3];
} CmifDomainOutHeader;

typedef struct CmifRequestFormat {
    u32 object_id;
    u32 request_id;
    u32 context;
    u32 data_size;
    u32 server_pointer_size;
    u32 num_in_auto_buffers;
    u32 num_out_auto_buffers;
    u32 num_in_buffers;
    u32 num_out_buffers;
    u32 num_inout_buffers;
    u32 num_in_pointers;
    u32 num_out_pointers;
    u32 num_out_fixed_pointers;
    u32 num_objects;
    u32 num_handles;
    u32 send_pid;
} CmifRequestFormat;

typedef struct CmifRequest {
    HipcRequest hipc;
    void* data;
    u16* out_pointer_sizes;
    u32* objects;
    u32 server_pointer_size;
    u32 cur_in_ptr_id;
} CmifRequest;

typedef struct CmifResponse {
    void* data;
    u32* objects;
    Handle* copy_handles;
    Handle* move_handles;
} CmifResponse;

NX_CONSTEXPR void* cmifGetAlignedDataStart(u32* data_words, void* base)
{
    intptr_t data_start = ((u8*)data_words - (u8*)base + 15) &~ 15;
    return (u8*)base + data_start;
}

NX_CONSTEXPR CmifRequest cmifMakeRequest(void* base, CmifRequestFormat fmt)
{
    // First of all, we need to figure out what size we need.
    u32 actual_size = 16;
    if (fmt.object_id)
        actual_size += sizeof(CmifDomainInHeader) + fmt.num_objects*sizeof(u32);
    actual_size += sizeof(CmifInHeader) + fmt.data_size;
    actual_size = (actual_size + 1) &~ 1; // hword-align
    u32 out_pointer_size_table_offset = actual_size;
    u32 out_pointer_size_table_size = fmt.num_out_auto_buffers + fmt.num_out_pointers;
    actual_size += sizeof(u16)*out_pointer_size_table_size;
    u32 num_data_words = (actual_size + 3) / 4;

    CmifRequest req = {};
    req.hipc = hipcMakeRequestInline(base,
        .type             = fmt.context ? CmifCommandType_RequestWithContext : CmifCommandType_Request,
        .num_send_statics = fmt.num_in_auto_buffers  + fmt.num_in_pointers,
        .num_send_buffers = fmt.num_in_auto_buffers  + fmt.num_in_buffers,
        .num_recv_buffers = fmt.num_out_auto_buffers + fmt.num_out_buffers,
        .num_exch_buffers = fmt.num_inout_buffers,
        .num_data_words   = num_data_words,
        .num_recv_statics = out_pointer_size_table_size + fmt.num_out_fixed_pointers,
        .send_pid         = fmt.send_pid,
        .num_copy_handles = fmt.num_handles,
        .num_move_handles = 0,
    );

    CmifInHeader* hdr = NULL;
    void* start = cmifGetAlignedDataStart(req.hipc.data_words, base);
    if (fmt.object_id) {
        CmifDomainInHeader* domain_hdr = (CmifDomainInHeader*)start;
        u32 payload_size = sizeof(CmifInHeader) + fmt.data_size;
        *domain_hdr = (CmifDomainInHeader){
            .type           = CmifDomainRequestType_SendMessage,
            .num_in_objects = (u8)fmt.num_objects,
            .data_size      = (u16)payload_size,
            .object_id      = fmt.object_id,
            .padding        = 0,
            .token          = fmt.context,
        };
        hdr = (CmifInHeader*)(domain_hdr+1);
        req.objects = (u32*)((u8*)hdr + payload_size);
    } else
        hdr = (CmifInHeader*)start;

    *hdr = (CmifInHeader){
        .magic      = CMIF_IN_HEADER_MAGIC,
        .version    = fmt.context ? 1U : 0U,
        .command_id = fmt.request_id,
        .token      = fmt.object_id ? 0U : fmt.context,
    };

    req.data = hdr+1;
    req.out_pointer_sizes = (u16*)(void*)((u8*)(void*)req.hipc.data_words + out_pointer_size_table_offset);
    req.server_pointer_size = fmt.server_pointer_size;

    return req;
}

NX_CONSTEXPR void* cmifMakeControlRequest(void* base, u32 request_id, u32 size)
{
    u32 actual_size = 16 + sizeof(CmifInHeader) + size;
    HipcRequest hipc = hipcMakeRequestInline(base,
        .type = CmifCommandType_Control,
        .num_data_words = (actual_size + 3) / 4,
    );
    CmifInHeader* hdr = (CmifInHeader*)cmifGetAlignedDataStart(hipc.data_words, base);
    *hdr = (CmifInHeader){
        .magic      = CMIF_IN_HEADER_MAGIC,
        .version    = 0,
        .command_id = request_id,
        .token      = 0,
    };
    return hdr+1;
}

NX_CONSTEXPR void cmifMakeCloseRequest(void* base, u32 object_id)
{
    if (object_id) {
        HipcRequest hipc = hipcMakeRequestInline(base,
            .type           = CmifCommandType_Request,
            .num_data_words = (16 + sizeof(CmifDomainInHeader)) / 4,
        );
        CmifDomainInHeader* domain_hdr = (CmifDomainInHeader*)cmifGetAlignedDataStart(hipc.data_words, base);
        *domain_hdr = (CmifDomainInHeader){
            .type      = CmifDomainRequestType_Close,
            .object_id = object_id,
        };
    } else {
        hipcMakeRequestInline(base,
            .type = CmifCommandType_Close,
        );
    }
}

NX_CONSTEXPR void cmifRequestInBuffer(CmifRequest* req, const void* buffer, size_t size, HipcBufferMode mode)
{
    *req->hipc.send_buffers++ = hipcMakeBuffer(buffer, size, mode);
}

NX_CONSTEXPR void cmifRequestOutBuffer(CmifRequest* req, void* buffer, size_t size, HipcBufferMode mode)
{
    *req->hipc.recv_buffers++ = hipcMakeBuffer(buffer, size, mode);
}

NX_CONSTEXPR void cmifRequestInOutBuffer(CmifRequest* req, void* buffer, size_t size, HipcBufferMode mode)
{
    *req->hipc.exch_buffers++ = hipcMakeBuffer(buffer, size, mode);
}

NX_CONSTEXPR void cmifRequestInPointer(CmifRequest* req, const void* buffer, size_t size)
{
    *req->hipc.send_statics++ = hipcMakeSendStatic(buffer, size, req->cur_in_ptr_id++);
    req->server_pointer_size -= size;
}

NX_CONSTEXPR void cmifRequestOutFixedPointer(CmifRequest* req, void* buffer, size_t size)
{
    *req->hipc.recv_list++ = hipcMakeRecvStatic(buffer, size);
    req->server_pointer_size -= size;
}

NX_CONSTEXPR void cmifRequestOutPointer(CmifRequest* req, void* buffer, size_t size)
{
    cmifRequestOutFixedPointer(req, buffer, size);
    *req->out_pointer_sizes++ = size;
}

NX_CONSTEXPR void cmifRequestInAutoBuffer(CmifRequest* req, const void* buffer, size_t size)
{
    if (req->server_pointer_size && size <= req->server_pointer_size) {
        cmifRequestInPointer(req, buffer, size);
        cmifRequestInBuffer(req, NULL, 0, HipcBufferMode_Normal);
    } else {
        cmifRequestInPointer(req, NULL, 0);
        cmifRequestInBuffer(req, buffer, size, HipcBufferMode_Normal);
    }
}

NX_CONSTEXPR void cmifRequestOutAutoBuffer(CmifRequest* req, void* buffer, size_t size)
{
    if (req->server_pointer_size && size <= req->server_pointer_size) {
        cmifRequestOutPointer(req, buffer, size);
        cmifRequestOutBuffer(req, NULL, 0, HipcBufferMode_Normal);
    } else {
        cmifRequestOutPointer(req, NULL, 0);
        cmifRequestOutBuffer(req, buffer, size, HipcBufferMode_Normal);
    }
}

NX_CONSTEXPR void cmifRequestObject(CmifRequest* req, u32 object_id)
{
    *req->objects++ = object_id;
}

NX_CONSTEXPR void cmifRequestHandle(CmifRequest* req, Handle handle)
{
    *req->hipc.copy_handles++ = handle;
}

NX_CONSTEXPR Result cmifParseResponse(CmifResponse* res, void* base, bool is_domain, u32 size)
{
    HipcResponse hipc = hipcParseResponse(base);
    void* start = cmifGetAlignedDataStart(hipc.data_words, base);

    CmifOutHeader* hdr = NULL;
    u32* objects = NULL;
    if (is_domain) {
        CmifDomainOutHeader* domain_hdr = (CmifDomainOutHeader*)start;
        hdr = (CmifOutHeader*)(domain_hdr+1);
        objects = (u32*)((u8*)hdr + sizeof(CmifOutHeader) + size);
    }
    else
        hdr = (CmifOutHeader*)start;

    if (hdr->magic != CMIF_OUT_HEADER_MAGIC)
        return MAKERESULT(Module_Libnx, LibnxError_InvalidCmifOutHeader);
    if (R_FAILED(hdr->result))
        return hdr->result;

    *res = (CmifResponse){
        .data         = hdr+1,
        .objects      = objects,
        .copy_handles = hipc.copy_handles,
        .move_handles = hipc.move_handles,
    };

    return 0;
}

NX_CONSTEXPR u32 cmifResponseGetObject(CmifResponse* res)
{
    return *res->objects++;
}

NX_CONSTEXPR Handle cmifResponseGetCopyHandle(CmifResponse* res)
{
    return *res->copy_handles++;
}

NX_CONSTEXPR Handle cmifResponseGetMoveHandle(CmifResponse* res)
{
    return *res->move_handles++;
}

NX_INLINE Result cmifConvertCurrentObjectToDomain(Handle h, u32* out_object_id)
{
    cmifMakeControlRequest(armGetTls(), 0, 0);
    Result rc = svcSendSyncRequest(h);
    if (R_SUCCEEDED(rc)) {
        CmifResponse resp = {};
        rc = cmifParseResponse(&resp, armGetTls(), false, sizeof(u32));
        if (R_SUCCEEDED(rc) && out_object_id)
            *out_object_id = *(u32*)resp.data;
    }
    return rc;
}

NX_INLINE Result cmifCopyFromCurrentDomain(Handle h, u32 object_id, Handle* out_h)
{
    void* raw = cmifMakeControlRequest(armGetTls(), 1, sizeof(u32));
    *(u32*)raw = object_id;
    Result rc = svcSendSyncRequest(h);
    if (R_SUCCEEDED(rc)) {
        CmifResponse resp = {};
        rc = cmifParseResponse(&resp, armGetTls(), false, 0);
        if (R_SUCCEEDED(rc) && out_h)
            *out_h = resp.move_handles[0];
    }
    return rc;
}

NX_INLINE Result cmifCloneCurrentObject(Handle h, Handle* out_h)
{
    cmifMakeControlRequest(armGetTls(), 2, 0);
    Result rc = svcSendSyncRequest(h);
    if (R_SUCCEEDED(rc)) {
        CmifResponse resp = {};
        rc = cmifParseResponse(&resp, armGetTls(), false, 0);
        if (R_SUCCEEDED(rc) && out_h)
            *out_h = resp.move_handles[0];
    }
    return rc;
}

NX_INLINE Result cmifQueryPointerBufferSize(Handle h, u16* out_size)
{
    cmifMakeControlRequest(armGetTls(), 3, 0);
    Result rc = svcSendSyncRequest(h);
    if (R_SUCCEEDED(rc)) {
        CmifResponse resp = {};
        rc = cmifParseResponse(&resp, armGetTls(), false, sizeof(u16));
        if (R_SUCCEEDED(rc) && out_size)
            *out_size = *(u16*)resp.data;
    }
    return rc;
}

NX_INLINE Result cmifCloneCurrentObjectEx(Handle h, u32 tag, Handle* out_h)
{
    void* raw = cmifMakeControlRequest(armGetTls(), 4, sizeof(u32));
    *(u32*)raw = tag;
    Result rc = svcSendSyncRequest(h);
    if (R_SUCCEEDED(rc)) {
        CmifResponse resp = {};
        rc = cmifParseResponse(&resp, armGetTls(), false, 0);
        if (R_SUCCEEDED(rc) && out_h)
            *out_h = resp.move_handles[0];
    }
    return rc;
}
