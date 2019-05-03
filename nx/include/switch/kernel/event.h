/**
 * @file event.h
 * @brief Kernel-mode event synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../result.h"
#include "wait.h"

/// Kernel-mode event structure.
typedef struct {
    Handle revent;  ///< Read-only event handle
    Handle wevent;  ///< Write-only event handle
    bool autoclear; ///< Autoclear flag
} Event;

/// Creates a \ref Waiter for a kernel-mode event.
static inline Waiter waiterForEvent(Event* t)
{
    Waiter wait_obj;
    wait_obj.type = t->autoclear ? WaiterType_HandleWithClear : WaiterType_Handle;
    wait_obj.handle = t->revent;
    return wait_obj;
}

/**
 * @brief Creates a kernel-mode event.
 * @param[out] t Pointer to \ref Event structure.
 * @param[in] autoclear Autoclear flag.
 * @return Result code.
 * @warning This is a privileged operation; in normal circumstances applications shouldn't use this function.
 */
Result eventCreate(Event* t, bool autoclear);

/**
 * @brief Loads a kernel-mode event obtained from IPC.
 * @param[out] t Pointer to \ref Event structure.
 * @param[in] handle Read-only event handle.
 * @param[in] autoclear Autoclear flag.
 */
void   eventLoadRemote(Event* t, Handle handle, bool autoclear);

/**
 * @brief Closes a kernel-mode event.
 * @param[in] t Pointer to \ref Event structure.
 */
void   eventClose(Event* t);

/**
 * @brief Returns whether an \ref Event is initialized.
 * @param[in] t Pointer to \ref Event structure.
 * @return Initialization status.
 */
static inline bool eventActive(Event* t)
{
    return t->revent != INVALID_HANDLE;
}

/**
 * @brief Waits on a kernel-mode event.
 * @param[in] t Pointer to \ref Event structure.
 * @param[in] timeout Timeout in nanoseconds (pass UINT64_MAX to wait indefinitely).
 * @return Result code.
 */
Result eventWait(Event* t, u64 timeout);

/**
 * @brief Signals a kernel-mode event.
 * @param[in] t Pointer to \ref Event structure.
 * @return Result code.
 * @note This function only works for events initialized with \ref eventCreate, it doesn't work with events initialized with \ref eventLoadRemote.
 * @warning This is a privileged operation; in normal circumstances applications shouldn't use this function.
 */
Result eventFire(Event* t);

/**
 * @brief Clears a kernel-mode event.
 * @param[in] t Pointer to \ref Event structure.
 * @return Result code.
 * @note This function shouldn't be used on autoclear events.
 */
Result eventClear(Event* t);
