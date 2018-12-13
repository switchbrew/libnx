// Copyright 2018 plutoo
#pragma once
#include "../kernel/wait.h"

typedef struct UsermodeTimer UsermodeTimer;

typedef enum {
    TimerType_OneShot,
    TimerType_Repeating
} TimerType;

struct UsermodeTimer
{
    Waitable waitable;
    TimerType type;
    u64 next_tick;
    u64 interval;
};

/**
 * @brief Creates a usermode timer.
 * @param[out] t UsermodeTimer object.
 * @param[in] interval Interval (in nanoseconds).
 * @param[in] type Timer type (repeating or one-shot)
 * @note It is safe to wait on this timer with several threads simultaneously.
 * @note If more than one thread is listening on it, at least one thread will get the signal. No other guarantees.
 * @note For a repeating timer: If the timer triggers twice before you wait on it, you will only get one signal.
 */
void utimerCreate(UsermodeTimer* t, u64 interval, TimerType type);
/**
 * @brief Starts the timer.
 * @param[in] t UsermodeTimer object.
 */
void utimerStart(UsermodeTimer* t);
/**
 * @brief Stops the timer.
 * @param[in] t UsermodeTimer object.
 */
void utimerStop(UsermodeTimer* t);
