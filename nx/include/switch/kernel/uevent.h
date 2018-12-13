// Copyright 2018 plutoo
#pragma once
#include "../kernel/wait.h"

typedef struct Uevent Uevent;

struct Uevent
{
    Waitable waitable;
    bool signal;
    bool auto_clear;
};

/**
 * @brief Creates a usermode event.
 * @param[out] e Uevent object.
 * @param[in] bool auto_clear Whether to automatically clear the event.
 * @note It is safe to wait on this event with several threads simultaneously.
 * @note If more than one thread is listening on it, at least one thread will get the signal. No other guarantees.
 */
void ueventCreate(Uevent* e, bool auto_clear);
/**
 * @brief Clears the event signal.
 * @param[in] e Uevent object.
 */
void ueventClear(Uevent* e);
/**
 * @brief Signals the event.
 * @param[in] e Uevent object.
 */
void ueventSignal(Uevent* e);
