#include <string.h>
#include <switch.h>

void binderCreateSession(binderSession *session, Handle sessionhandle, s32 ID) {
    memset(session, 0, sizeof(binderSession));
    session->sessionhandle = sessionhandle;
    session->ID = ID;
    session->initialized = 1;
}

Result binderInitSession(binderSession *session, u32 nativehandle_inval) {
    Result rc = 0;

    rc = binderAdjustRefcount(session, 1, 0);
    if (R_FAILED(rc)) return rc;

    rc = binderAdjustRefcount(session, 1, 1);
    if (R_FAILED(rc)) return rc;

    rc = binderGetNativeHandle(session, nativehandle_inval, &session->nativehandle);
    if (R_FAILED(rc)) return rc;

    //When the output nativehandle is 0 the binderSession ID is probably invalid.
    if(session->nativehandle == 0) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);

    return 0;
}

Result binderExitSession(binderSession *session) {
    Result rc = 0;

    if(!session->initialized)return 0;

    rc = binderAdjustRefcount(session, -1, 1);

    if (R_SUCCEEDED(rc)) rc = binderAdjustRefcount(session, -1, 0);

    if(session->nativehandle) {
        svcCloseHandle(session->nativehandle);
        session->nativehandle = 0;
    }

    session->initialized = 0;

    return rc;
}

static Result _binderTransactParcel(binderSession *session, u32 code, void* parcel_data, size_t parcel_data_size, void* parcel_reply, size_t parcel_reply_size, u32 flags) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 ID;
        u32 code;
        u32 flags;
    } *raw;

    ipcAddSendBuffer(&c, parcel_data, parcel_data_size, 0);
    ipcAddRecvBuffer(&c, parcel_reply, parcel_reply_size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->ID = session->ID;
    raw->code = code;
    raw->flags = flags;

    Result rc = ipcDispatch(session->sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

//TODO: Use TransactParcelAuto when it's available.

Result binderTransactParcel(binderSession *session, u32 code, void* parcel_data, size_t parcel_data_size, void* parcel_reply, size_t parcel_reply_size, u32 flags) {
    return _binderTransactParcel(session, code, parcel_data, parcel_data_size, parcel_reply, parcel_reply_size, flags);
}

Result binderAdjustRefcount(binderSession *session, s32 addval, s32 type) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 ID;
        s32 addval;
        s32 type;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->ID = session->ID;
    raw->addval = addval;
    raw->type = type;

    Result rc = ipcDispatch(session->sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result binderGetNativeHandle(binderSession *session, u32 inval, Handle *handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 ID;
        u32 inval;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->ID = session->ID;
    raw->inval = inval;

    Result rc = ipcDispatch(session->sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

