#pragma once
#include "../types.h"
#include "../kernel/event.h"

#define BINDER_FIRST_CALL_TRANSACTION 0x1

typedef struct {
    bool   created           : 1;
    bool   initialized       : 1;
    bool   has_transact_auto : 1;
    s32    id;
    size_t ipc_buffer_size;
} Binder;

// Note: binderClose will not close the session_handle provided to binderCreate.
void binderCreate(Binder* b, s32 id);
void binderClose(Binder* b);

Result binderInitSession(Binder* b);

Result binderTransactParcel(
    Binder* b, u32 code,
    void* parcel_data, size_t parcel_data_size,
    void* parcel_reply, size_t parcel_reply_size,
    u32 flags);

Result binderConvertErrorCode(s32 code);

Result binderAdjustRefcount(Binder* b, s32 addval, s32 type);
Result binderGetNativeHandle(Binder* b, u32 unk0, Event *event_out);

static inline Result binderIncreaseWeakRef(Binder* b)
{
    return binderAdjustRefcount(b, 1, 0);
}

static inline Result binderDecreaseWeakRef(Binder* b)
{
    return binderAdjustRefcount(b, -1, 0);
}

static inline Result binderIncreaseStrongRef(Binder* b)
{
    return binderAdjustRefcount(b, 1, 1);
}

static inline Result binderDecreaseStrongRef(Binder* b)
{
    return binderAdjustRefcount(b, -1, 1);
}
