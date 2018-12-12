// Copyright 2018 plutoo
#pragma once
#include "../kernel/wait.h"

typedef struct UsermodeEvent UsermodeEvent;

struct UsermodeEvent
{
    Waitable waitable;
    bool signal;
    bool auto_clear;
};

/**
 * @brief Creates a usermode event.
 * @param[out] e UsermodeEvent object.
 * @param[in] bool auto_clear Whether to automatically clear the event.
 * @note It is safe to wait on this event with several threads simultaneously.
 * @note If more than one thread is listening on it, at least one thread will get the signal. No other guarantees.
 */
void ueventCreate(UsermodeEvent* e, bool auto_clear);
/**
 * @brief Clears the event signal.
 * @param[in] e UsermodeEvent object.
 */
void ueventClear(UsermodeEvent* e);
/**
 * @brief Signals the event.
 * @param[in] e UsermodeEvent object.
 */
void ueventSignal(UsermodeEvent* e);
