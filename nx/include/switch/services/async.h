/**
 * @file async.h
 * @brief NS/NIM IAsync* IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../applets/error.h"
#include "../kernel/event.h"

/// AsyncValue
typedef struct {
    Service s;      ///< IAsyncValue
    Event event;    ///< Event with autoclear=false.
} AsyncValue;

/// AsyncResult
typedef struct {
    Service s;      ///< IAsyncResult
    Event event;    ///< Event with autoclear=false.
} AsyncResult;

///@name IAsyncValue
///@{

/**
 * @brief Close a \ref AsyncValue.
 * @note When the object is initialized, this uses \ref asyncValueCancel then \ref asyncValueWait with timeout=U64_MAX.
 * @param a \ref AsyncValue
 */
void asyncValueClose(AsyncValue *a);

/**
 * @brief Waits for the async operation to finish using the specified timeout.
 * @param a \ref AsyncValue
 * @param[in] timeout Timeout in nanoseconds. U64_MAX for no timeout.
 */
Result asyncValueWait(AsyncValue *a, u64 timeout);

/**
 * @brief Gets the value size.
 * @param a \ref AsyncValue
 * @param[out] size Output size.
 */
Result asyncValueGetSize(AsyncValue *a, u64 *size);

/**
 * @brief Gets the value.
 * @note Prior to using the cmd, this uses \ref asyncResultWait with timeout=U64_MAX.
 * @param a \ref AsyncValue
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size.
 */
Result asyncValueGet(AsyncValue *a, void* buffer, size_t size);

/**
 * @brief Cancels the async operation.
 * @note Used automatically by \ref asyncValueClose.
 * @param a \ref AsyncValue
 */
Result asyncValueCancel(AsyncValue *a);

/**
 * @brief Gets the \ref ErrorContext.
 * @param a \ref AsyncValue
 * @param[out] context \ref ErrorContext
 */
Result asyncValueGetErrorContext(AsyncValue *a, ErrorContext *context);

///@}

///@name IAsyncResult
///@{

/**
 * @brief Close a \ref AsyncResult.
 * @note When the object is initialized, this uses \ref asyncResultCancel then \ref asyncResultWait with timeout=U64_MAX.
 * @param a \ref AsyncResult
 */
void asyncResultClose(AsyncResult *a);

/**
 * @brief Waits for the async operation to finish using the specified timeout.
 * @param a \ref AsyncResult
 * @param[in] timeout Timeout in nanoseconds. U64_MAX for no timeout.
 */
Result asyncResultWait(AsyncResult *a, u64 timeout);

/**
 * @brief Gets the Result.
 * @note Prior to using the cmd, this uses \ref asyncResultWait with timeout=U64_MAX.
 * @param a \ref AsyncResult
 */
Result asyncResultGet(AsyncResult *a);

/**
 * @brief Cancels the async operation.
 * @note Used automatically by \ref asyncResultClose.
 * @param a \ref AsyncResult
 */
Result asyncResultCancel(AsyncResult *a);

/**
 * @brief Gets the \ref ErrorContext.
 * @param a \ref AsyncResult
 * @param[out] context \ref ErrorContext
 */
Result asyncResultGetErrorContext(AsyncResult *a, ErrorContext *context);

///@}

