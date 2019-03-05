/**
 * @file thread.h
 * @brief Multi-threading support
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../arm/thread_context.h"
#include "wait.h"

/// Thread information structure.
typedef struct Thread {
    Handle handle;       ///< Thread handle.
    void*  stack_mem;    ///< Pointer to stack memory.
    void*  stack_mirror; ///< Pointer to stack memory mirror.
    size_t stack_sz;     ///< Stack size.
    void** tls_array;
    struct Thread* next;
    struct Thread** prev_next;
} Thread;

/// Creates a \ref Waiter for a \ref Thread.
static inline Waiter waiterForThread(Thread* t)
{
    return waiterForHandle(t->handle);
}

/**
 * @brief Creates a thread.
 * @param t Thread information structure which will be filled in.
 * @param entry Entrypoint of the thread.
 * @param arg Argument to pass to the entrypoint.
 * @param stack_sz Stack size (rounded up to page alignment).
 * @param prio Thread priority (0x00~0x3F); 0x2C is the usual priority of the main thread, 0x3B is a special priority on cores 0..2 that enables preemptive multithreading (0x3F on core 3).
 * @param cpuid ID of the core on which to create the thread (0~3); or -2 to use the default core for the current process.
 * @return Result code.
 */
Result threadCreate(
    Thread* t, ThreadFunc entry, void* arg, size_t stack_sz, int prio,
    int cpuid);

/**
 * @brief Starts the execution of a thread.
 * @param t Thread information structure.
 * @return Result code.
 */
Result threadStart(Thread* t);

/**
 * @brief Exits the current thread immediately.
 */
void NORETURN threadExit(void);

/**
 * @brief Waits for a thread to finish executing.
 * @param t Thread information structure.
 * @return Result code.
 */
Result threadWaitForExit(Thread* t);

/**
 * @brief Frees up resources associated with a thread.
 * @param t Thread information structure.
 * @return Result code.
 */
Result threadClose(Thread* t);

/**
 * @brief Pauses the execution of a thread.
 * @param t Thread information structure.
 * @return Result code.
 */
Result threadPause(Thread* t);

/**
 * @brief Resumes the execution of a thread, after having been paused.
 * @param t Thread information structure.
 * @return Result code.
 */
Result threadResume(Thread* t);

/**
 * @brief Dumps the registers of a thread paused by @ref threadPause (register groups: all).
 * @param[out] ctx Output thread context (register dump).
 * @param t Thread information structure.
 * @return Result code.
 * @warning Official kernel will not dump x0..x18 if the thread is currently executing a system call, and prior to 6.0.0 doesn't dump TPIDR_EL0.
 */
Result threadDumpContext(ThreadContext* ctx, Thread* t);

/**
 * @brief Gets the raw handle to the current thread.
 * @return The current thread's handle.
 */
Handle threadGetCurHandle(void);

/**
 * @brief Allocates a TLS slot.
 * @param destructor Function to run automatically when a thread exits.
 * @return TLS slot ID on success, or a negative value on failure.
 */
s32 threadTlsAlloc(void (* destructor)(void*));

/**
 * @brief Retrieves the value stored in a TLS slot.
 * @param slot_id TLS slot ID.
 * @return Value.
 */
void* threadTlsGet(s32 slot_id);

/**
 * @brief Stores the specified value into a TLS slot.
 * @param slot_id TLS slot ID.
 * @param value Value.
 */
void threadTlsSet(s32 slot_id, void* value);

/**
 * @brief Frees a TLS slot.
 * @param slot_id TLS slot ID.
 */
void threadTlsFree(s32 slot_id);
