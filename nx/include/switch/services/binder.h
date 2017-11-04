typedef struct {
    bool initialized;
    Handle sessionhandle;
    s32 ID;

    Handle nativehandle;
} binderSession;

//binderExitSession will not close the sessionhandle since it's user-specified via binderCreateSession and may be used elsewhere.
void binderCreateSession(binderSession *session, Handle sessionhandle, s32 ID);
Result binderInitSession(binderSession *session, u32 nativehandle_inval);/// nativehandle_inval is the inval for binderGetNativeHandle.
Result binderExitSession(binderSession *session);

Result binderTransactParcel(binderSession *session, u32 code, void* parcel_data, size_t parcel_data_size, void* parcel_reply, size_t parcel_reply_size, u32 flags);
Result binderAdjustRefcount(binderSession *session, s32 addval, s32 type);
Result binderGetNativeHandle(binderSession *session, u32 inval, Handle *handle_out);

