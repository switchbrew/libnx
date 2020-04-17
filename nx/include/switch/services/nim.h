/**
 * @file nim.h
 * @brief Network Install Manager (nim) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// SystemUpdateTaskId
typedef struct {
    alignas(8) Uuid uuid;  ///< UUID
} NimSystemUpdateTaskId;

/// Initialize nim.
Result nimInitialize(void);

/// Exit nim.
void nimExit(void);

/// Gets the Service object for the actual nim service session.
Service* nimGetServiceSession(void);

Result nimListSystemUpdateTask(s32 *out_count, NimSystemUpdateTaskId *out_task_ids, size_t max_task_ids);
Result nimDestroySystemUpdateTask(const NimSystemUpdateTaskId *task_id);
