#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "types.h"
#include "result.h"
#include "sf/service.h"
#include "services/sm.h"
#include "services/sfdnsres.h"

#include <string.h>

NX_INLINE Result _sfdnsresDispatchImpl(
    u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    const SfDispatchParams disp
) {
    Handle h = INVALID_HANDLE;
    Result rc = smGetServiceOriginal(&h, smEncodeName("sfdnsres"));
    if (R_FAILED(rc))
        return rc;

    // Construct service struct
    Service srv = {
        .session             = h,
        .own_handle          = 1,
        .object_id           = 0,
        .pointer_buffer_size = 0, // No sfdnsres commands use HipcAutoSelect so this is left as 0
    };

    void* in = serviceMakeRequest(&srv, request_id, disp.context,
        in_data_size, disp.in_send_pid,
        disp.buffer_attrs, disp.buffers,
        disp.in_num_objects, disp.in_objects,
        disp.in_num_handles, disp.in_handles);

    if (in_data_size)
        __builtin_memcpy(in, in_data, in_data_size);

    rc = svcSendSyncRequest(h);
    if (R_SUCCEEDED(rc)) {
        void* out = NULL;
        rc = serviceParseResponse(&srv,
            out_data_size, &out,
            disp.out_num_objects, disp.out_objects,
            disp.out_handle_attrs, disp.out_handles);

        if (R_SUCCEEDED(rc) && out_data && out_data_size)
            __builtin_memcpy(out_data, out, out_data_size);
    }

    serviceClose(&srv);
    return rc;
}

#define _sfdnsresDispatch(_rid,...) \
    _sfdnsresDispatchImpl((_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _sfdnsresDispatchIn(_rid,_in,...) \
    _sfdnsresDispatchImpl((_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _sfdnsresDispatchOut(_rid,_out,...) \
    _sfdnsresDispatchImpl((_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _sfdnsresDispatchInOut(_rid,_in,_out,...) \
    _sfdnsresDispatchImpl((_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

static Result _sfdnsresCmdInErrOutStr(u32 err, char *out_str, size_t out_str_size, u32 cmd_id) {
    return _sfdnsresDispatchIn(cmd_id, err,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_str, out_str_size } },
    );
}

Result sfdnsresGetHostByNameRequest(u32 cancel_handle, bool use_nsd, const char *name, u32 *h_errno_, u32 *errno_, void *out_buffer, size_t out_buffer_size, u32 *out_serialized_size) {
    const struct {
        u32 use_nsd; // actually u8 bool, but promoted to u32 for convenience
        u32 cancel_handle;
        u64 pid_placeholder;
    } in = { use_nsd ? 1 : 0, cancel_handle, 0 };

    struct {
        u32 h_errno_;
        u32 errno_;
        u32 serialized_size;
    } out = {};

    Result rc = _sfdnsresDispatchInOut(2, in, out,
        .in_send_pid = true,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { name,       name ? strlen(name) + 1 : 0 },
            { out_buffer, out_buffer_size             },
        },
    );

    if (R_SUCCEEDED(rc)) {
        if (h_errno_) *h_errno_ = out.h_errno_;
        if (errno_) *errno_ = out.errno_;
        if (out_serialized_size) *out_serialized_size = out.serialized_size;
    }

    return rc;
}

Result sfdnsresGetHostByAddrRequest(const void *in_addr, size_t in_addr_len, u32 type, u32 cancel_handle, u32 *h_errno_, u32 *errno_, void *out_buffer, size_t out_buffer_size, u32 *out_serialized_size) {
    const struct {
        u32 len; // wtf nintendo
        u32 type;
        u32 cancel_handle;
        u32 _padding;
        u64 pid_placeholder;
    } in = { (u32)in_addr_len, type, cancel_handle, 0, 0 };

    struct {
        u32 h_errno_;
        u32 errno_;
        u32 serialized_size;
    } out = {};

    Result rc = _sfdnsresDispatchInOut(3, in, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { in_addr,    in_addr_len     },
            { out_buffer, out_buffer_size },
        },
    );

    if (R_SUCCEEDED(rc)) {
        if (h_errno_) *h_errno_ = out.h_errno_;
        if (errno_) *errno_ = out.errno_;
        if (out_serialized_size) *out_serialized_size = out.serialized_size;
    }

    return rc;
}

Result sfdnsresGetHostStringErrorRequest(u32 err, char *out_str, size_t out_str_size) {
    return _sfdnsresCmdInErrOutStr(err, out_str, out_str_size, 4);
}

Result sfdnsresGetGaiStringErrorRequest(u32 err, char *out_str, size_t out_str_size) {
    return _sfdnsresCmdInErrOutStr(err, out_str, out_str_size, 5);
}

Result sfdnsresGetAddrInfoRequest(u32 cancel_handle, bool use_nsd, const char *node, const char *service, const void *in_hints, size_t in_hints_size, void *out_buffer, size_t out_buffer_size, u32 *errno_, s32 *ret, u32 *out_serialized_size) {
    const struct {
        u32 use_nsd; // actually u8 bool, but promoted to u32 for convenience
        u32 cancel_handle;
        u64 pid_placeholder;
    } in = { use_nsd ? 1 : 0, cancel_handle, 0 };

    struct {
        u32 errno_;
        s32 ret;
        u32 serialized_size;
    } out = {};

    Result rc = _sfdnsresDispatchInOut(6, in, out,
        .in_send_pid = true,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { node,       node    ? strlen(node)    + 1 : 0 },
            { service,    service ? strlen(service) + 1 : 0 },
            { in_hints,   in_hints_size                     },
            { out_buffer, out_buffer_size                   },
        },
    );

    if (R_SUCCEEDED(rc)) {
        if (errno_) *errno_ = out.errno_;
        if (ret) *ret = out.ret;
        if (out_serialized_size) *out_serialized_size = out.serialized_size;
    }

    return rc;
}

Result sfdnsresGetNameInfoRequest(u32 flags, const void *in_sa, size_t in_sa_size, char *out_host, size_t out_host_size, char *out_serv, size_t out_serv_len, u32 cancel_handle, u32 *errno_, s32 *ret) {
    const struct {
        u32 flags;
        u32 cancel_handle;
        u64 pid_placeholder;
    } in = { flags, cancel_handle, 0 };

    struct {
        u32 errno_;
        s32 ret;
    } out;

    Result rc = _sfdnsresDispatchInOut(7, in, out,
        .in_send_pid = true,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { in_sa,    in_sa_size    },
            { out_host, out_host_size },
            { out_serv, out_serv_len  },
        },
    );

    if (R_SUCCEEDED(rc)) {
        if (errno_) *errno_ = out.errno_;
        if (ret) *ret = out.ret;
    }

    return rc;
}

Result sfdnsresGetCancelHandleRequest(u32 *out_handle) {
    u64 pid_placeholder = 0;
    return _sfdnsresDispatchInOut(8, pid_placeholder, *out_handle, .in_send_pid = true);
}

Result sfdnsresCancelRequest(u32 handle) {
    const struct {
        u32 handle;
        u32 _padding;
        u64 pid_placeholder;
    } in = { handle, 0, 0 };

    return _sfdnsresDispatchIn(9, in, .in_send_pid = true);
}
