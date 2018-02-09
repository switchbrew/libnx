/**
 * @file condvar.h
 * @brief Condition variable synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#include "../types.h"
#include "../kernel/mutex.h"

/// Condition variable structure.
typedef struct {
    u32    tag;
    Mutex* mutex;
} CondVar;

/**
 * @brief Initializes a condition variable.
 * @param[in] c Condition variable object.
 * @param[in] m Mutex object to use inside the condition variable.
 */
void condvarInit(CondVar* c, Mutex* m);

/**
 * @brief Waits on a condition variable with a timeout.
 * @param[in] c Condition variable object.
 * @param[in] timeout Timeout in nanoseconds.
 * @return Result code (0xEA01 on timeout).
 * @remark On function return, the underlying mutex is acquired.
 */
Result condvarWaitTimeout(CondVar* c, u64 timeout);

/**
 * @brief Waits on a condition variable.
 * @param[in] c Condition variable object.
 * @return Result code.
 * @remark On function return, the underlying mutex is acquired.
 */
static inline Result condvarWait(CondVar* c)
{
    return condvarWaitTimeout(c, -1ull);
}

/**
 * @brief Wakes up up to the specified number of threads waiting on a condition variable.
 * @param[in] c Condition variable object.
 * @param[in] num Maximum number of threads to wake up (or -1 to wake them all up).
 * @return Result code.
 */
Result condvarWake(CondVar* c, int num);

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
