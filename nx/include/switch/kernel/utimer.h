/**
 * @file uevent.h
 * @brief User-mode timer synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "wait.h"

typedef struct UTimer UTimer;

typedef enum {
    TimerType_OneShot,
    TimerType_Repeating,
} TimerType;

struct UTimer {
    Waitable waitable;
    TimerType type;
    u64 next_tick;
    u64 interval;
};

/**
 * @brief Creates a user-mode timer.
 * @param[out] t UTimer object.
 * @param[in] interval Interval (in nanoseconds).
 * @param[in] type Timer type (repeating or one-shot)
 * @note It is safe to wait on this timer with several threads simultaneously.
 * @note If more than one thread is listening on it, at least one thread will get the signal. No other guarantees.
 * @note For a repeating timer: If the timer triggers twice before you wait on it, you will only get one signal.
 */
void utimerCreate(UTimer* t, u64 interval, TimerType type);

/**
 * @brief Starts the timer.
 * @param[in] t UTimer object.
 */
void utimerStart(UTimer* t);

/**
 * @brief Stops the timer.
 * @param[in] t UTimer object.
 */
void utimerStop(UTimer* t);
