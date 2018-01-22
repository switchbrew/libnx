#pragma once
#include "../types.h"

#define BINDER_FIRST_CALL_TRANSACTION 0x1

typedef struct {
    bool   created;
    bool   initialized;
    Handle sessionHandle;
    s32    id;
    Handle nativeHandle;
    size_t ipcBufferSize;
    bool   hasTransactAuto;
} Binder;

// binderExitSession will not close the sessionHandle since it's user-specified via binderCreateSession and may be used elsewhere.
void binderCreateSession(Binder *session, Handle sessionHandle, s32 ID);
Result binderInitSession(Binder *session, u32 unk0);
void binderExitSession(Binder *session);

Result binderTransactParcel(
    Binder *session, u32 code,
    void* parcel_data, size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags);

Result binderAdjustRefcount(Binder *session, s32 addval, s32 type);
Result binderGetNativeHandle(Binder *session, u32 unk0, Handle *handle_out);

