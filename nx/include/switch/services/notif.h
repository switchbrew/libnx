/**
 * @file notif.h
 * @brief Alarm notification (notif:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/applet.h"
#include "../sf/service.h"

/**
 * @brief Gets an Event which is signaled when data is available with \ref notifTryPopNotifiedApplicationParameter.
 * @note This is a wrapper for \ref appletGetNotificationStorageChannelEvent, see that for the usage requirements.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
NX_INLINE Result notifGetNotificationSystemEvent(Event *out_event) {
    return appletGetNotificationStorageChannelEvent(out_event);
}

/**
 * @brief Uses \ref appletTryPopFromNotificationStorageChannel then reads the data from there into the output params.
 * @note This is a wrapper for \ref appletTryPopFromNotificationStorageChannel, see that for the usage requirements.
 * @param[out] buffer Output buffer.
 * @param[out] size Output buffer size.
 * @param[out] out_size Size of the data which was written into the output buffer. Optional, can be NULL.
 */
Result notifTryPopNotifiedApplicationParameter(void* buffer, u64 size, u64 *out_size);

