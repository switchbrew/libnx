/**
 * @file ectx.h
 * @brief [11.0.0+] Error Context services IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

/// Initialize ectx:r.
Result ectxrInitialize(void);

/// Exit ectx:r.
void ectxrExit(void);

/// Gets the Service object for the actual ectx:r service session.
Service* ectxrGetServiceSession(void);

/**
 * @brief Retrieves the error context associated with an error descriptor and result.
 * @param[out] out0 Output value.
 * @param[out] out_total_size Total error context size.
 * @param[out] out_size Error context size.
 * @param[out] dst Buffer for output error context.
 * @param[in] dst_size Buffer size for output error context.
 * @param[in] descriptor Error descriptor.
 * @param[in] result Error result.
 * @return Result code.
 */
Result ectxrPullContext(s32 *out0, u32 *out_total_size, u32 *out_size, void *dst, size_t dst_size, u32 descriptor, Result result);
