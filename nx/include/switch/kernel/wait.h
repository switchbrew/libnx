/**
 * @file wait.h
 * @brief User mode synchronization primitive waiting operations.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "mutex.h"

// Implementation details.

typedef struct Waitable Waitable;
typedef struct WaitableMethods WaitableMethods;
typedef struct WaitableNode WaitableNode;

struct WaitableNode {
    WaitableNode* prev;
    WaitableNode* next;
};

struct Waitable {
    const WaitableMethods* vt;
    WaitableNode list;
    Mutex mutex;
};

typedef enum {
    WaiterType_Handle,
    WaiterType_HandleWithClear,
    WaiterType_Waitable,
} WaiterType;

// User-facing API starts here.

/// Waiter structure.
typedef struct {
    WaiterType type;

    union {
        Handle handle;
        Waitable* waitable;
    };
} Waiter;

/// Creates a \ref Waiter for a kernel-mode \ref Handle.
static inline Waiter waiterForHandle(Handle h)
{
    Waiter wait_obj;
    wait_obj.type = WaiterType_Handle;
    wait_obj.handle = h;
    return wait_obj;
}

Result waitN(s32* idx_out, Waiter* objects, size_t num_objects, u64 timeout);
Result waitNHandle(s32* idx_out, Handle* handles, size_t num_handles, u64 timeout);

/**
 * @brief Waits for an arbitrary number of waiters. This is a macro that uses var-args.
 * @param[out] idx_out The index of the signalled waiter.
 * @param[in] timeout Timeout (in nanoseconds).
 * @note The number of waiters must be less than 64. This is a Horizon kernel limitation.
 */
#define waitMulti(idx_out, timeout, ...) \
    waitN((idx_out), (Waiter[]) { __VA_ARGS__ }, sizeof((Waiter[]) { __VA_ARGS__ }) / sizeof(Waiter), (timeout))

/**
 * @brief Waits for an arbitrary number of handles. This is a macro that uses var-args.
 * @param[out] idx_out The index of the signalled handle.
 * @param[in] timeout Timeout (in nanoseconds).
 * @note The number of handles must be less than 64. This is a Horizon kernel limitation.
 */
#define waitMultiHandle(idx_out, timeout, ...) \
    waitNHandle((idx_out), (Handle[]) { __VA_ARGS__ }, sizeof((Handle[]) { __VA_ARGS__ }) / sizeof(Handle), (timeout))

/**
 * @brief Waits for a single waiter.
 * @param[in] w The waiter to wait for.
 * @param[in] timeout Timeout (in nanoseconds).
 */
static inline Result waitSingle(Waiter w, u64 timeout)
{
    s32 idx;
    return waitMulti(&idx, timeout, w);
}

/**
 * @brief Waits for a single handle.
 * @param[in] h The handle to wait for.
 * @param[in] timeout Timeout (in nanoseconds).
 */
static inline Result waitSingleHandle(Handle h, u64 timeout)
{
    s32 idx;
    return waitMultiHandle(&idx, timeout, h);
}
