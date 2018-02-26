#include <string.h>
#include "types.h"
#include "result.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "display/binder.h"

void binderCreate(Binder* b, Handle session_handle, s32 id)
{
    memset(b, 0, sizeof(Binder));
    b->created = true;
    b->session_handle = session_handle;
    b->id = id;
    b->native_handle = INVALID_HANDLE;
    b->has_transact_auto = false;
}

Result binderInitSession(Binder* b, u32 unk0)
{
    Result rc = 0;

    if (!b->created)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (b->initialized)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    rc = binderAdjustRefcount(b, 1, 0);

    if (R_FAILED(rc))
        return rc;

    rc = binderAdjustRefcount(b, 1, 1);

    if (R_FAILED(rc)) {
        binderAdjustRefcount(b, -1, 0);
        return rc;
    }

    rc = binderGetNativeHandle(b, unk0, &b->native_handle);

    if (R_FAILED(rc)) {
        binderAdjustRefcount(b, -1, 1);
        binderAdjustRefcount(b, -1, 0);
        return rc;
    }

    // When the output native_handle is 0 the Binder ID is probably invalid.
    if (b->native_handle == 0) {
        binderAdjustRefcount(b, -1, 1);
        binderAdjustRefcount(b, -1, 0);
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    b->initialized = true;

    rc = ipcQueryPointerBufferSize(b->session_handle, &b->ipc_buffer_size);

    if (R_FAILED(rc)) {
        binderClose(b);
        return rc;
    }

    // Use TransactParcelAuto when available.
    if (kernelAbove300())
        b->has_transact_auto = true;

    return rc;
}

void binderClose(Binder* b)
{
    if (!b->created)
        return;

    if (b->initialized) {
        binderAdjustRefcount(b, -1, 1);
        binderAdjustRefcount(b, -1, 0);

        if (b->native_handle != INVALID_HANDLE) {
            svcCloseHandle(b->native_handle);
            b->native_handle = INVALID_HANDLE;
        }
    }

    b->session_handle = INVALID_HANDLE;
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

    Result rc = ipcDispatch(b->session_handle);

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
    } *raw;

    ipcAddSendSmart(&c, b->ipc_buffer_size, parcel_data, parcel_data_size, 0);
    ipcAddRecvSmart(&c, b->ipc_buffer_size, parcel_reply, parcel_reply_size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->session_id = b->id;
    raw->code = code;
    raw->flags = flags;

    Result rc = ipcDispatch(b->session_handle);

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

    Result rc = ipcDispatch(b->session_handle);

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

Result binderGetNativeHandle(Binder* b, u32 inval, Handle *handle_out)
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

    Result rc = ipcDispatch(b->session_handle);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}
