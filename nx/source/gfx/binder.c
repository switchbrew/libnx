#include <string.h>
#include <switch/types.h>
#include <switch/result.h>
#include <switch/ipc.h>
#include <switch/kernel/detect.h>
#include <switch/gfx/binder.h>

void binderCreateSession(Binder *session, Handle sessionHandle, s32 id)
{
    memset(session, 0, sizeof(Binder));
    session->created = 1;
    session->sessionHandle = sessionHandle;
    session->id = id;
    session->nativeHandle = INVALID_HANDLE;
    session->hasTransactAuto = 0;
}

Result binderInitSession(Binder *session, u32 unk0)
{
    Result rc = 0;

    if (!session->created) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (session->initialized) return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    rc = binderAdjustRefcount(session, 1, 0);

    if (R_FAILED(rc))
        return rc;

    rc = binderAdjustRefcount(session, 1, 1);

    if (R_FAILED(rc)) {
        rc = binderAdjustRefcount(session, -1, 0);
        return rc;
    }

    rc = binderGetNativeHandle(session, unk0, &session->nativeHandle);

    if (R_FAILED(rc)) {
        rc = binderAdjustRefcount(session, -1, 1);
        rc = binderAdjustRefcount(session, -1, 0);
        return rc;
    }

    // When the output nativeHandle is 0 the Binder ID is probably invalid.
    if(session->nativeHandle == 0) {
        rc = binderAdjustRefcount(session, -1, 1);
        rc = binderAdjustRefcount(session, -1, 0);
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    session->initialized = 1;

    rc = ipcQueryPointerBufferSize(session->sessionHandle, &session->ipcBufferSize);

    if (R_FAILED(rc)) {
        binderExitSession(session);
        return rc;
    }

    // Use TransactParcelAuto when available.
    if (kernelAbove300())
        session->hasTransactAuto = 1;

    return rc;
}

void binderExitSession(Binder *session)
{
    if (!session->created) return;

    if (session->initialized) {
        binderAdjustRefcount(session, -1, 1);
        binderAdjustRefcount(session, -1, 0);

        if (session->nativeHandle != INVALID_HANDLE) {
            svcCloseHandle(session->nativeHandle);
            session->nativeHandle = INVALID_HANDLE;
        }
    }

    session->sessionHandle = INVALID_HANDLE;
    session->id = 0;

    session->created = 0;
    session->initialized = 0;
}

static Result _binderTransactParcel(
    Binder *session, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    if (!session->created || !session->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

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
    raw->session_id = session->id;
    raw->code = code;
    raw->flags = flags;

    Result rc = ipcDispatch(session->sessionHandle);

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
    Binder *session, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    if (!session->created || !session->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 session_id;
        u32 code;
        u32 flags;
    } *raw;

    void* buf_static[2] = {parcel_data, parcel_reply};
    void* buf_transfer[2] = {parcel_data, parcel_reply};
    size_t buf_static_size[2] = {parcel_data_size, parcel_reply_size};
    size_t buf_transfer_size[2] = {parcel_data_size, parcel_reply_size};

    if(session->ipcBufferSize!=0 && buf_static_size[0] <= session->ipcBufferSize) {
        buf_transfer[0] = NULL;
        buf_transfer[1] = NULL;
        buf_transfer_size[0] = 0;
        buf_transfer_size[1] = 0;
    }
    else {
        buf_static[0] = NULL;
        buf_static[1] = NULL;
        buf_static_size[0] = 0;
        buf_static_size[1] = 0;
    }

    ipcAddSendBuffer(&c, buf_transfer[0], buf_transfer_size[0], 0);
    ipcAddRecvBuffer(&c, buf_transfer[1], buf_transfer_size[1], 0);

    ipcAddSendStatic(&c, buf_static[0], buf_static_size[0], 0);
    ipcAddRecvStatic(&c, buf_static[1], buf_static_size[1], 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->session_id = session->id;
    raw->code = code;
    raw->flags = flags;

    Result rc = ipcDispatch(session->sessionHandle);

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
    Binder *session, u32 code,
    void* parcel_data,  size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags)
{
    Result rc = 0;

    if (session->hasTransactAuto)
        rc = _binderTransactParcelAuto(session, code, parcel_data, parcel_data_size, parcel_reply, parcel_reply_size, flags);
    else
        rc = _binderTransactParcel(session, code, parcel_data, parcel_data_size, parcel_reply, parcel_reply_size, flags);

    return rc;
}

Result binderAdjustRefcount(Binder *session, s32 addval, s32 type)
{
    if (!session->created) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

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
    raw->session_id = session->id;
    raw->addval = addval;
    raw->type = type;

    Result rc = ipcDispatch(session->sessionHandle);

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

Result binderGetNativeHandle(Binder *session, u32 inval, Handle *handle_out)
{
    if (!session->created) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

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
    raw->session_id = session->id;
    raw->inval = inval;

    Result rc = ipcDispatch(session->sessionHandle);

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
