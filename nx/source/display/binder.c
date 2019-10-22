#include <string.h>
#include "types.h"
#include "result.h"
#include "sf/service.h"
#include "runtime/hosversion.h"
#include "display/binder.h"

void binderCreate(Binder* b, s32 id)
{
    memset(b, 0, sizeof(Binder));
    b->created = true;
    b->id = id;
}

Result binderInitSession(Binder* b, Service* relay)
{
    Result rc = 0;

    if (!b->created)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (b->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    b->relay = relay;

    rc = binderIncreaseWeakRef(b);
    if (R_FAILED(rc))
        return rc;

    rc = binderIncreaseStrongRef(b);
    if (R_FAILED(rc)) {
        binderDecreaseWeakRef(b);
        return rc;
    }

    b->initialized = true;

    return rc;
}

void binderClose(Binder* b)
{
    if (!b->created)
        return;

    if (b->initialized) {
        binderDecreaseStrongRef(b);
        binderDecreaseWeakRef(b);
    }

    b->id = 0;

    b->created = false;
    b->initialized = false;
}

Result binderTransactParcel(
    Binder* b, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    if (!b->created || !b->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const bool use_auto = hosversionAtLeast(3,0,0);
    const u32 cmd_id = use_auto ? 3 : 0;
    const u32 buf_mode = use_auto ? SfBufferAttr_HipcAutoSelect : SfBufferAttr_HipcMapAlias;

    const struct {
        s32 session_id;
        u32 code;
        u32 flags;
    } in = { b->id, code, flags };

    return serviceDispatchIn(b->relay, cmd_id, in,
        .buffer_attrs = {
            buf_mode | SfBufferAttr_In,
            buf_mode | SfBufferAttr_Out,
        },
        .buffers = {
            { parcel_data,  parcel_data_size  },
            { parcel_reply, parcel_reply_size },
        },
    );
}

Result binderConvertErrorCode(s32 code)
{
    if (code >= 0)
        return 0;

    switch (-code) {
        case 1: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_PermissionDenied);
        case 2: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_NameNotFound);
        case 11: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_WouldBlock);
        case 12: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_NoMemory);
        case 17: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_AlreadyExists);
        case 19: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_NoInit);
        case 22: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_BadValue);
        case 32: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_DeadObject);
        case 38: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_InvalidOperation);
        case 61: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_NotEnoughData);
        case 74: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_UnknownTransaction);
        case 75: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_BadIndex);
        case 110: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_TimedOut);
        case -(INT32_MIN+7): return MAKERESULT(Module_LibnxBinder, LibnxBinderError_FdsNotAllowed);
        case -(INT32_MIN+2): return MAKERESULT(Module_LibnxBinder, LibnxBinderError_FailedTransaction);
        case -(INT32_MIN+1): return MAKERESULT(Module_LibnxBinder, LibnxBinderError_BadType);
        default: return MAKERESULT(Module_LibnxBinder, LibnxBinderError_Unknown);
    }
}

Result binderAdjustRefcount(Binder* b, s32 addval, s32 type)
{
    if (!b->created)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        s32 session_id;
        s32 addval;
        s32 type;
    } in = { b->id, addval, type };

    return serviceDispatchIn(b->relay, 1, in);
}

Result binderGetNativeHandle(Binder* b, u32 inval, Event *event_out)
{
    if (!b->created)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        s32 session_id;
        u32 inval;
    } in = { b->id, inval };

    Handle h = INVALID_HANDLE;
    Result rc = serviceDispatchIn(b->relay, 2, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &h,
    );

    if (R_SUCCEEDED(rc) && event_out)
        eventLoadRemote(event_out, h, false);

    return rc;
}
