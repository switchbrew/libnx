// Copyright 2018 plutoo
#pragma once
#include "../kernel/wait.h"

typedef struct Utimer Utimer;

typedef enum {
    TimerType_OneShot,
    TimerType_Repeating
} TimerType;

struct Utimer
{
    Waitable waitable;
    TimerType type;
    u64 next_tick;
    u64 interval;
};

/**
 * @brief Creates a usermode timer.
 * @param[out] t Utimer object.
 * @param[in] interval Interval (in nanoseconds).
 * @param[in] type Timer type (repeating or one-shot)
 * @note It is safe to wait on this timer with several threads simultaneously.
 * @note If more than one thread is listening on it, at least one thread will get the signal. No other guarantees.
 * @note For a repeating timer: If the timer triggers twice before you wait on it, you will only get one signal.
 */
void utimerCreate(Utimer* t, u64 interval, TimerType type);
/**
 * @brief Starts the timer.
 * @param[in] t Utimer object.
 */
void utimerStart(Utimer* t);
/**
 * @brief Stops the timer.
 * @param[in] t Utimer object.
 */
void utimerStop(Utimer* t);
