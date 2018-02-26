#pragma once
#include "../types.h"

#define BINDER_FIRST_CALL_TRANSACTION 0x1

typedef struct {
    bool   created;
    bool   initialized;
    Handle session_handle;
    s32    id;
    Handle native_handle;
    size_t ipc_buffer_size;
    bool   has_transact_auto;
} Binder;

// Note: binderClose will not close the session_handle provided to binderCreate.
void binderCreate(Binder* b, Handle session_handle, s32 id);
void binderClose(Binder* b);

Result binderInitSession(Binder* b, u32 unk0);

Result binderTransactParcel(
    Binder* b, u32 code,
    void* parcel_data, size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags);

Result binderAdjustRefcount(Binder* b, s32 addval, s32 type);
Result binderGetNativeHandle(Binder* b, u32 unk0, Handle *handle_out);

