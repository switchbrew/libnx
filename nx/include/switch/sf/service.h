/**
 * @file service.h
 * @brief Service wrapper object
 * @author fincs
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "hipc.h"
#include "cmif.h"

/// Service object structure
typedef struct Service {
    Handle session;
    u32 own_handle;
    u32 object_id;
    u16 pointer_buffer_size;
} Service;

enum {
    SfBufferAttr_In                             = BIT(0),
    SfBufferAttr_Out                            = BIT(1),
    SfBufferAttr_HipcMapAlias                   = BIT(2),
    SfBufferAttr_HipcPointer                    = BIT(3),
    SfBufferAttr_FixedSize                      = BIT(4),
    SfBufferAttr_HipcAutoSelect                 = BIT(5),
    SfBufferAttr_HipcMapTransferAllowsNonSecure = BIT(6),
    SfBufferAttr_HipcMapTransferAllowsNonDevice = BIT(7),
};

typedef struct SfBufferAttrs {
    u32 attr0;
    u32 attr1;
    u32 attr2;
    u32 attr3;
    u32 attr4;
    u32 attr5;
    u32 attr6;
    u32 attr7;
} SfBufferAttrs;

typedef struct SfBuffer {
    const void* ptr;
    size_t size;
} SfBuffer;

typedef enum SfOutHandleAttr {
    SfOutHandleAttr_None     = 0,
    SfOutHandleAttr_HipcCopy = 1,
    SfOutHandleAttr_HipcMove = 2,
} SfOutHandleAttr;

typedef struct SfOutHandleAttrs {
    SfOutHandleAttr attr0;
    SfOutHandleAttr attr1;
    SfOutHandleAttr attr2;
    SfOutHandleAttr attr3;
    SfOutHandleAttr attr4;
    SfOutHandleAttr attr5;
    SfOutHandleAttr attr6;
    SfOutHandleAttr attr7;
} SfOutHandleAttrs;

typedef struct SfDispatchParams {
    Handle target_session;
    u32 context;

    SfBufferAttrs buffer_attrs;
    SfBuffer buffers[8];

    bool in_send_pid;

    u32 in_num_objects;
    const Service* in_objects[8];

    u32 in_num_handles;
    Handle in_handles[8];

    u32 out_num_objects;
    Service* out_objects;

    SfOutHandleAttrs out_handle_attrs;
    Handle* out_handles;
} SfDispatchParams;

/**
 * @brief Returns whether a service has been initialized.
 * @param[in] s Service object.
 * @return true if initialized.
 */
NX_CONSTEXPR bool serviceIsActive(Service* s) {
    return s->session != INVALID_HANDLE;
}

/**
 * @brief Returns whether a service is overriden in the homebrew environment.
 * @param[in] s Service object.
 * @return true if overriden.
 */
NX_CONSTEXPR bool serviceIsOverride(Service* s) {
    return serviceIsActive(s) && !s->own_handle && !s->object_id;
}

/**
 * @brief Returns whether a service is a domain.
 * @param[in] s Service object.
 * @return true if a domain.
 */
NX_CONSTEXPR bool serviceIsDomain(Service* s) {
    return serviceIsActive(s) && s->own_handle && s->object_id;
}

/**
 * @brief Returns whether a service is a domain subservice.
 * @param[in] s Service object.
 * @return true if a domain subservice.
 */
NX_CONSTEXPR bool serviceIsDomainSubservice(Service* s) {
    return serviceIsActive(s) && !s->own_handle && s->object_id;
}

/**
 * @brief For a domain/domain subservice, return the associated object ID.
 * @param[in] s Service object, necessarily a domain or domain subservice.
 * @return The object ID.
 */
NX_CONSTEXPR u32 serviceGetObjectId(Service* s) {
    return s->object_id;
}

/**
 * @brief Creates a service object from an IPC session handle.
 * @param[out] s Service object.
 * @param[in] h IPC session handle.
 */
NX_INLINE void serviceCreate(Service* s, Handle h)
{
    s->session = h;
    s->own_handle = 1;
    s->object_id = 0;
    s->pointer_buffer_size = 0;
    cmifQueryPointerBufferSize(h, &s->pointer_buffer_size);
}

/**
 * @brief Creates a non-domain subservice object from a parent service.
 * @param[out] s Service object.
 * @param[in] parent Parent service.
 * @param[in] h IPC session handle for this subservice.
 */
NX_INLINE void serviceCreateNonDomainSubservice(Service* s, Service* parent, Handle h)
{
    s->session = h;
    s->own_handle = 1;
    s->object_id = 0;
    s->pointer_buffer_size = parent->pointer_buffer_size;
}

/**
 * @brief Creates a domain subservice object from a parent service.
 * @param[out] s Service object.
 * @param[in] parent Parent service, necessarily a domain or domain subservice.
 * @param[in] object_id Object ID for this subservice.
 */
NX_CONSTEXPR void serviceCreateDomainSubservice(Service* s, Service* parent, u32 object_id)
{
    s->session = parent->session;
    s->own_handle = 0;
    s->object_id = object_id;
    s->pointer_buffer_size = parent->pointer_buffer_size;
}

/**
 * @brief Hints the compiler that a service will always contain a domain object.
 * @param[in] _s Service object.
 */
#define serviceAssumeDomain(_s) do { \
    if (!(_s)->object_id) \
        __builtin_unreachable(); \
} while(0)

/**
 * @brief Closes a service.
 * @param[in] s Service object.
 */
NX_INLINE void serviceClose(Service* s)
{
#if defined(NX_SERVICE_ASSUME_NON_DOMAIN)
    if (s->object_id)
        __builtin_unreachable();
#endif

    if (s->own_handle || s->object_id) {
        cmifMakeCloseRequest(armGetTls(), s->own_handle ? 0 : s->object_id);
        svcSendSyncRequest(s->session);
        if (s->own_handle)
            svcCloseHandle(s->session);
    }
    *s = (Service){};
}

/**
 * @brief Clones a service.
 * @param[in] s Service object.
 * @param[out] out_s Output service object.
 */
NX_INLINE Result serviceClone(Service* s, Service* out_s)
{
#if defined(NX_SERVICE_ASSUME_NON_DOMAIN)
    if (s->object_id)
        __builtin_unreachable();
#endif

    out_s->session = 0;
    out_s->own_handle = 1;
    out_s->object_id = s->object_id;
    out_s->pointer_buffer_size = s->pointer_buffer_size;
    return cmifCloneCurrentObject(s->session, &out_s->session);
}

/**
 * @brief Clones a service with a session manager tag.
 * @param[in] s Service object.
 * @param[in] tag Session manager tag (unused in current official server code)
 * @param[out] out_s Output service object.
 */
NX_INLINE Result serviceCloneEx(Service* s, u32 tag, Service* out_s)
{
#if defined(NX_SERVICE_ASSUME_NON_DOMAIN)
    if (s->object_id)
        __builtin_unreachable();
#endif

    out_s->session = 0;
    out_s->own_handle = 1;
    out_s->object_id = s->object_id;
    out_s->pointer_buffer_size = s->pointer_buffer_size;
    return cmifCloneCurrentObjectEx(s->session, tag, &out_s->session);
}

/**
 * @brief Converts a regular service to a domain.
 * @param[in] s Service object.
 * @return Result code.
 */
NX_INLINE Result serviceConvertToDomain(Service* s)
{
    if (!s->own_handle) {
        // For overridden services, create a clone first.
        Result rc = cmifCloneCurrentObjectEx(s->session, 0, &s->session);
        if (R_FAILED(rc))
            return rc;
        s->own_handle = 1;
    }

    return cmifConvertCurrentObjectToDomain(s->session, &s->object_id);
}

NX_CONSTEXPR void _serviceRequestFormatProcessBuffer(CmifRequestFormat* fmt, u32 attr)
{
    if (!attr) return;
    const bool is_in  = (attr & SfBufferAttr_In)  != 0;
    const bool is_out = (attr & SfBufferAttr_Out) != 0;

    if (attr & SfBufferAttr_HipcAutoSelect) {
        if (is_in)
            fmt->num_in_auto_buffers ++;
        if (is_out)
            fmt->num_out_auto_buffers ++;
    } else if (attr & SfBufferAttr_HipcPointer) {
        if (is_in)
            fmt->num_in_pointers ++;
        if (is_out) {
            if (attr & SfBufferAttr_FixedSize)
                fmt->num_out_fixed_pointers ++;
            else
                fmt->num_out_pointers ++;
        }
    } else if (attr & SfBufferAttr_HipcMapAlias) {
        if (is_in && is_out)
            fmt->num_inout_buffers ++;
        else if (is_in)
            fmt->num_in_buffers ++;
        else if (is_out)
            fmt->num_out_buffers ++;
    }
}

NX_CONSTEXPR void _serviceRequestProcessBuffer(CmifRequest* req, const SfBuffer* buf, u32 attr)
{
    if (!attr) return;
    const bool is_in  = (attr & SfBufferAttr_In);
    const bool is_out = (attr & SfBufferAttr_Out);

    if (attr & SfBufferAttr_HipcAutoSelect) {
        if (is_in)
            cmifRequestInAutoBuffer(req, buf->ptr, buf->size);
        if (is_out)
            cmifRequestOutAutoBuffer(req, (void*)buf->ptr, buf->size);
    } else if (attr & SfBufferAttr_HipcPointer) {
        if (is_in)
            cmifRequestInPointer(req, buf->ptr, buf->size);
        if (is_out) {
            if (attr & SfBufferAttr_FixedSize)
                cmifRequestOutFixedPointer(req, (void*)buf->ptr, buf->size);
            else
                cmifRequestOutPointer(req, (void*)buf->ptr, buf->size);
        }
    } else if (attr & SfBufferAttr_HipcMapAlias) {
        HipcBufferMode mode = HipcBufferMode_Normal;
        if (attr & SfBufferAttr_HipcMapTransferAllowsNonSecure)
            mode = HipcBufferMode_NonSecure;
        if (attr & SfBufferAttr_HipcMapTransferAllowsNonDevice)
            mode = HipcBufferMode_NonDevice;

        if (is_in && is_out)
            cmifRequestInOutBuffer(req, (void*)buf->ptr, buf->size, mode);
        else if (is_in)
            cmifRequestInBuffer(req, buf->ptr, buf->size, mode);
        else if (is_out)
            cmifRequestOutBuffer(req, (void*)buf->ptr, buf->size, mode);
    }
}

NX_INLINE void* serviceMakeRequest(
    Service* s, u32 request_id, u32 context, u32 data_size, bool send_pid,
    const SfBufferAttrs buffer_attrs, const SfBuffer* buffers,
    u32 num_objects, const Service* const* objects,
    u32 num_handles, const Handle* handles
) {
#if defined(NX_SERVICE_ASSUME_NON_DOMAIN)
    if (s->object_id)
        __builtin_unreachable();
#endif

    CmifRequestFormat fmt = {};
    fmt.object_id = s->object_id;
    fmt.request_id = request_id;
    fmt.context = context;
    fmt.data_size = data_size;
    fmt.server_pointer_size = s->pointer_buffer_size;
    fmt.num_objects = num_objects;
    fmt.num_handles = num_handles;
    fmt.send_pid = send_pid;

    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr0);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr1);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr2);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr3);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr4);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr5);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr6);
    _serviceRequestFormatProcessBuffer(&fmt, buffer_attrs.attr7);

    CmifRequest req = cmifMakeRequest(armGetTls(), fmt);

    if (s->object_id) // TODO: Check behavior of input objects in non-domain sessions
        for (u32 i = 0; i < num_objects; i ++)
            cmifRequestObject(&req, objects[i]->object_id);

    for (u32 i = 0; i < num_handles; i ++)
        cmifRequestHandle(&req, handles[i]);

    _serviceRequestProcessBuffer(&req, &buffers[0], buffer_attrs.attr0);
    _serviceRequestProcessBuffer(&req, &buffers[1], buffer_attrs.attr1);
    _serviceRequestProcessBuffer(&req, &buffers[2], buffer_attrs.attr2);
    _serviceRequestProcessBuffer(&req, &buffers[3], buffer_attrs.attr3);
    _serviceRequestProcessBuffer(&req, &buffers[4], buffer_attrs.attr4);
    _serviceRequestProcessBuffer(&req, &buffers[5], buffer_attrs.attr5);
    _serviceRequestProcessBuffer(&req, &buffers[6], buffer_attrs.attr6);
    _serviceRequestProcessBuffer(&req, &buffers[7], buffer_attrs.attr7);

    return req.data;
}

NX_CONSTEXPR void _serviceResponseGetHandle(CmifResponse* res, SfOutHandleAttr type, Handle* out)
{
    switch (type) {
        default:
        case SfOutHandleAttr_None:
            break;
        case SfOutHandleAttr_HipcCopy:
            *out = cmifResponseGetCopyHandle(res);
            break;
        case SfOutHandleAttr_HipcMove:
            *out = cmifResponseGetMoveHandle(res);
            break;
    }
}

NX_INLINE Result serviceParseResponse(
    Service* s, u32 out_size, void** out_data,
    u32 num_out_objects, Service* out_objects,
    const SfOutHandleAttrs out_handle_attrs, Handle* out_handles
) {
#if defined(NX_SERVICE_ASSUME_NON_DOMAIN)
    if (s->object_id)
        __builtin_unreachable();
#endif

    CmifResponse res = {};
    bool is_domain = s->object_id != 0;
    Result rc = cmifParseResponse(&res, armGetTls(), is_domain, out_size);
    if (R_FAILED(rc))
        return rc;

    if (out_size)
        *out_data = res.data;

    for (u32 i = 0; i < num_out_objects; i ++) {
        if (is_domain)
            serviceCreateDomainSubservice(&out_objects[i], s, cmifResponseGetObject(&res));
        else // Output objects are marshalled as move handles at the beginning of the list.
            serviceCreateNonDomainSubservice(&out_objects[i], s, cmifResponseGetMoveHandle(&res));
    }

    _serviceResponseGetHandle(&res, out_handle_attrs.attr0, &out_handles[0]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr1, &out_handles[1]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr2, &out_handles[2]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr3, &out_handles[3]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr4, &out_handles[4]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr5, &out_handles[5]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr6, &out_handles[6]);
    _serviceResponseGetHandle(&res, out_handle_attrs.attr7, &out_handles[7]);

    return 0;
}

NX_INLINE Result serviceDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
)
{
    // Make a copy of the service struct, so that the compiler can assume that it won't be modified by function calls.
    Service srv = *s;

    void* in = serviceMakeRequest(&srv, request_id, disp.context,
        in_data_size, disp.in_send_pid,
        disp.buffer_attrs, disp.buffers,
        disp.in_num_objects, disp.in_objects,
        disp.in_num_handles, disp.in_handles);

    if (in_data_size)
        __builtin_memcpy(in, in_data, in_data_size);

    Result rc = svcSendSyncRequest(disp.target_session == INVALID_HANDLE ? s->session : disp.target_session);
    if (R_SUCCEEDED(rc)) {
        void* out = NULL;
        rc = serviceParseResponse(&srv,
            out_data_size, &out,
            disp.out_num_objects, disp.out_objects,
            disp.out_handle_attrs, disp.out_handles);

        if (R_SUCCEEDED(rc) && out_data && out_data_size)
            __builtin_memcpy(out_data, out, out_data_size);
    }

    return rc;
}

#define serviceDispatch(_s,_rid,...) \
    serviceDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define serviceDispatchIn(_s,_rid,_in,...) \
    serviceDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define serviceDispatchOut(_s,_rid,_out,...) \
    serviceDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define serviceDispatchInOut(_s,_rid,_in,_out,...) \
    serviceDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })
