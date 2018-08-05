/**
 * @file condvar.h
 * @brief Condition variable synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/svc.h"
#include "../kernel/mutex.h"

/// Condition variable.
typedef u32 CondVar;

/**
 * @brief Initializes a condition variable.
 * @param[in] c Condition variable object.
 */
static inline void condvarInit(CondVar* c)
{
    *c = 0;
}

/**
 * @brief Waits on a condition variable with a timeout.
 * @param[in] c Condition variable object.
 * @param[in] m Mutex object to use inside the condition variable.
 * @param[in] timeout Timeout in nanoseconds.
 * @return Result code (0xEA01 on timeout).
 * @remark On function return, the underlying mutex is acquired.
 */
Result condvarWaitTimeout(CondVar* c, Mutex* m, u64 timeout);

/**
 * @brief Waits on a condition variable.
 * @param[in] c Condition variable object.
 * @param[in] m Mutex object to use inside the condition variable.
 * @return Result code.
 * @remark On function return, the underlying mutex is acquired.
 */
static inline Result condvarWait(CondVar* c, Mutex* m)
{
    return condvarWaitTimeout(c, m, U64_MAX);
}

/**
 * @brief Wakes up up to the specified number of threads waiting on a condition variable.
 * @param[in] c Condition variable object.
 * @param[in] num Maximum number of threads to wake up (or -1 to wake them all up).
 * @return Result code.
 */
static inline Result condvarWake(CondVar* c, int num)
{
    return svcSignalProcessWideKey(c, num);
}

/**
 * @brief Wakes up a single thread waiting on a condition variable.
 * @param[in] c Condition variable object.
 * @return Result code.
 */
static inline Result condvarWakeOne(CondVar* c)
{
    return condvarWake(c, 1);
}

/**
 * @brief Wakes up all thread waiting on a condition variable.
 * @param[in] c Condition variable object.
 * @return Result code.
 */
static inline Result condvarWakeAll(CondVar* c)
{
    return condvarWake(c, -1);
}
