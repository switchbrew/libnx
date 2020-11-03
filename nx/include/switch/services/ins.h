/**
 * @file ins.h
 * @brief INS services IPC wrapper.
 * @author averne
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

/// Initialize ins:r.
Result insrInitialize(void);

/// Exit ins:r.
void insrExit(void);

/// Gets the Service object for the actual ins:r service session.
Service* insrGetServiceSession(void);

/**
 * @brief Retrieves the last system tick the event corresponding to the ID was signaled at.
 * @param[in] id Ins request ID (should be 0..4).
 * @param[out] tick.
 * @return Result code.
 * @note The tick is only updated once per second at minimum.
 */
Result insrGetLastTick(u32 id, u64 *tick);

/**
 * @brief Retrieves the event corresponding to the ID.
 * @param[in] id Ins request ID (should be 0..4).
 * @param[out] out.
 * @return Result code.
 * @note The event is only signaled once per second at minimum.
 */
Result insrGetReadableEvent(u32 id, Event *out);

/// Initialize ins:s.
Result inssInitialize(void);

/// Exit ins:s.
void inssExit(void);

/// Gets the Service object for the actual ins:s service session.
Service* inssGetServiceSession(void);

/**
 * @brief Retrieves the event corresponding to the ID.
 * @param[in] id Ins send ID (should be 0..11).
 * @param[out] out.
 * @return Result code.
 * @note The returned event cannot be waited on, only signaled. Clearing is handled by the service.
 */
Result inssGetWritableEvent(u32 id, Event *out);
