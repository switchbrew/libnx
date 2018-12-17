/**
 * @file utimer.h
 * @brief User-mode timer synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "wait.h"

typedef struct UTimer UTimer;

/// Valid types for a user-mode timer.
typedef enum {
    TimerType_OneShot,   ///< Timers of this kind fire once and then stop automatically.
    TimerType_Repeating, ///< Timers of this kind fire periodically.
} TimerType;

/// User-mode timer object.
struct UTimer {
    Waitable waitable;
    TimerType type : 8;
    bool started : 1;
    u64 next_tick;
    u64 interval;
};

/// Creates a waiter for a user-mode timer.
static inline Waiter waiterForUTimer(UTimer* t)
{
    Waiter wait_obj;
    wait_obj.type = WaiterType_Waitable;
    wait_obj.waitable = &t->waitable;
    return wait_obj;
}

/**
 * @brief Creates a user-mode timer.
 * @param[out] t UTimer object.
 * @param[in] interval Interval (in nanoseconds).
 * @param[in] type Type of timer to create (see \ref TimerType).
 * @note The timer is stopped when it is created. Use \ref utimerStart to start it.
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
