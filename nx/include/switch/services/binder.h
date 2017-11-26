#define BINDER_FIRST_CALL_TRANSACTION 0x1

typedef struct {
    Handle sessionHandle;
    s32    id;
    Handle nativeHandle;
    size_t ipcBufferSize;
    bool   hasTransactAuto;
} binderSession;

// binderExitSession will not close the sessionHandle since it's user-specified via binderCreateSession and may be used elsewhere.
void binderCreateSession(binderSession *session, Handle sessionHandle, s32 ID);
Result binderInitSession(binderSession *session, u32 unk0);
void binderExitSession(binderSession *session);

Result binderTransactParcel(
    binderSession *session, u32 code,
    void* parcel_data, size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags);

Result binderAdjustRefcount(binderSession *session, s32 addval, s32 type);
Result binderGetNativeHandle(binderSession *session, u32 unk0, Handle *handle_out);

