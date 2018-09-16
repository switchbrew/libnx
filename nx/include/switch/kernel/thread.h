/**
 * @file thread.h
 * @brief Multi-threading support
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../arm/thread_context.h"

/// Thread information structure.
typedef struct {
    Handle handle;       ///< Thread handle.
    void*  stack_mem;    ///< Pointer to stack memory.
    void*  stack_mirror; ///< Pointer to stack memory mirror.
    size_t stack_sz;     ///< Stack size.
} Thread;

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
