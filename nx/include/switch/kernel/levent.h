/**
 * @file levent.h
 * @brief Light event synchronization primitive [4.0.0+]
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../result.h"
#include "svc.h"

/// User-mode light event structure.
typedef struct LEvent {
    u32  counter;
    bool autoclear;
} LEvent;

/**
 * @brief Initializes a user-mode light event.
 * @param[out] le Pointer to \ref LEvent structure.
 * @param[in] signaled Whether the event starts off in signaled state.
 * @param[in] autoclear Autoclear flag.
 */
NX_CONSTEXPR void leventInit(LEvent* le, bool signaled, bool autoclear) {
    le->counter = signaled ? 2 : 0;
    le->autoclear = autoclear;
}

/**
 * @brief Waits on a user-mode light event.
 * @param[in] le Pointer to \ref LEvent structure.
 * @param[in] timeout_ns Timeout in nanoseconds (pass UINT64_MAX to wait indefinitely).
 * @return true if wait succeeded, false if wait timed out.
 */
bool leventWait(LEvent* le, u64 timeout_ns);

/**
 * @brief Polls a user-mode light event.
 * @param[in] le Pointer to \ref LEvent structure.
 * @return true if event is signaled, false otherwise.
 */
bool leventTryWait(LEvent* le);

/**
 * @brief Signals a user-mode light event.
 * @param[in] le Pointer to \ref LEvent structure.
 */
void leventSignal(LEvent* le);

/**
 * @brief Clears a user-mode light event.
 * @param[in] le Pointer to \ref LEvent structure.
 */
void leventClear(LEvent* le);
