/**
 * @file uevent.h
 * @brief User-mode event synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "wait.h"

typedef struct UEvent UEvent;

struct UEvent {
    Waitable waitable;
    bool signal;
    bool auto_clear;
};

/**
 * @brief Creates a user-mode event.
 * @param[out] e UEvent object.
 * @param[in] bool auto_clear Whether to automatically clear the event.
 * @note It is safe to wait on this event with several threads simultaneously.
 * @note If more than one thread is listening on it, at least one thread will get the signal. No other guarantees.
 */
void ueventCreate(UEvent* e, bool auto_clear);

/**
 * @brief Clears the event signal.
 * @param[in] e UEvent object.
 */
void ueventClear(UEvent* e);

/**
 * @brief Signals the event.
 * @param[in] e UEvent object.
 */
void ueventSignal(UEvent* e);
