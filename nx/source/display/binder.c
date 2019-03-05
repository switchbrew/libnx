#include <string.h>
#include "types.h"
#include "result.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/vi.h"
#include "display/binder.h"

static Result _binderIpcDispatch(void)
{
    return serviceIpcDispatch(viGetSession_IHOSBinderDriverRelay());
}

void binderCreate(Binder* b, s32 id)
{
    memset(b, 0, sizeof(Binder));
    b->created = true;
    b->id = id;
}

Result binderInitSession(Binder* b)
{
    Result rc = 0;

    if (!b->created)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (b->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    rc = binderIncreaseWeakRef(b);
    if (R_FAILED(rc))
        return rc;

    rc = binderIncreaseStrongRef(b);
    if (R_FAILED(rc)) {
        binderDecreaseStrongRef(b);
        return rc;
    }

    b->initialized = true;

    rc = ipcQueryPointerBufferSize(viGetSession_IHOSBinderDriverRelay()->handle, &b->ipc_buffer_size);
    if (R_FAILED(rc)) {
        binderClose(b);
        return rc;
    }

    // Use TransactParcelAuto when available.
    if (hosversionAtLeast(3,0,0))
        b->has_transact_auto = true;

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

static Result _binderTransactParcel(
    Binder* b, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    if (!b->created || !b->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 session_id;
        u32 code;
        u32 flags;
    } *raw;

    ipcAddSendBuffer(&c, parcel_data, parcel_data_size, 0);
    ipcAddRecvBuffer(&c, parcel_reply, parcel_reply_size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->session_id = b->id;
    raw->code = code;
    raw->flags = flags;

    Result rc = _binderIpcDispatch();

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _binderTransactParcelAuto(
    Binder* b, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    if (!b->created || !b->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 session_id;
        u32 code;
        u32 flags;
    } PACKED *raw;

    ipcAddSendSmart(&c, b->ipc_buffer_size, parcel_data, parcel_data_size, 0);
    ipcAddRecvSmart(&c, b->ipc_buffer_size, parcel_reply, parcel_reply_size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->session_id = b->id;
    raw->code = code;
    raw->flags = flags;

    Result rc = _binderIpcDispatch();

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result binderTransactParcel(
    Binder* b, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    Result rc = 0;

    if (b->has_transact_auto)
        rc = _binderTransactParcelAuto(b, code, parcel_data, parcel_data_size, parcel_reply, parcel_reply_size, flags);
    else
        rc = _binderTransactParcel(b, code, parcel_data, parcel_data_size, parcel_reply, parcel_reply_size, flags);

    return rc;
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

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 session_id;
        s32 addval;
        s32 type;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->session_id = b->id;
    raw->addval = addval;
    raw->type = type;

    Result rc = _binderIpcDispatch();

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result binderGetNativeHandle(Binder* b, u32 inval, Event *event_out)
{
    if (!b->created)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 session_id;
        u32 inval;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->session_id = b->id;
    raw->inval = inval;

    Result rc = _binderIpcDispatch();

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event_out, r.Handles[0], false);
        }
    }

    return rc;
}
