/**
 * @file native_window.h
 * @brief Native window (NWindow) wrapper object, used for presenting images to the display (or other sinks).
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../kernel/mutex.h"
#include "../kernel/event.h"
#include "../services/vi.h"
#include "../nvidia/graphic_buffer.h"
#include "types.h"
#include "binder.h"
#include "buffer_producer.h"

/// Native window structure.
typedef struct NWindow {
    u32 magic;
    Binder bq;
    Event event;
    Mutex mutex;
    u64 slots_configured;
    u64 slots_requested;
    s32 cur_slot;
    u32 width;
    u32 height;
    u32 format;
    u32 usage;
    BqRect crop;
    u32 scaling_mode;
    u32 transform;
    u32 sticky_transform;
    u32 default_width;
    u32 default_height;
    u32 swap_interval;
    bool is_connected;
    bool producer_controlled_by_app;
    bool consumer_running_behind;
} NWindow;

///@name Basic functions
///@{

/// Checks whether a pointer refers to a valid \ref NWindow object.
bool nwindowIsValid(NWindow* nw);

/**
 * @brief Retrieves the default \ref NWindow object.
 * @return Pointer to the default \ref NWindow object.
 * @note When this function is used/referenced, libnx will initialize VI services
 *       and create a \ref NWindow object from a \ref ViLayer created on the default \ref ViDisplay;
 *       all of this happening automatically during application startup (i.e. before main is called).
 *       If creating the default \ref NWindow fails, libnx will throw a LibnxError_BadGfxInit fatal error.
 *       Likewise, after main returns (or exit is called) libnx will clean up all resources used by it.
 */
NWindow* nwindowGetDefault(void);

/**
 * @brief Creates a \ref NWindow.
 * @param[out] nw Output \ref NWindow structure.
 * @param[in] binder_id Android IGraphicBufferProducer binder session ID.
 * @param[in] producer_controlled_by_app Specifies whether the producer is controlled by the application.
 */
Result nwindowCreate(NWindow* nw, s32 binder_id, bool producer_controlled_by_app);

/**
 * @brief Creates a \ref NWindow operating on a \ref ViLayer.
 * @param[out] nw Output \ref NWindow structure.
 * @param[in] layer Pointer to \ref ViLayer structure (such as the one returned by \ref viCreateLayer).
 */
Result nwindowCreateFromLayer(NWindow* nw, const ViLayer* layer);

/// Closes a \ref NWindow, freeing all resources associated with it.
void nwindowClose(NWindow* nw);

///@}

///@name Window configuration
///@{

/**
 * @brief Retrieves the dimensions of a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[out] out_width Output variable containing the width of the \ref NWindow.
 * @param[out] out_height Output variable containing the height of the \ref NWindow.
 * @note After creation, a \ref NWindow reports a default size (usually 1280x720).
 *       This size can be overriden by calling \ref nwindowSetDimensions.
 */
Result nwindowGetDimensions(NWindow* nw, u32* out_width, u32* out_height);

/**
 * @brief Sets the dimensions of a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] width Desired width of the \ref NWindow.
 * @param[in] height Desired width of the \ref NWindow.
 * @note This function cannot be called when there are buffers registered with the \ref NWindow.
 */
Result nwindowSetDimensions(NWindow* nw, u32 width, u32 height);

/**
 * @brief Configures the crop applied to images presented through a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] left X coordinate of the left margin of the crop bounding box.
 * @param[in] top Y coordinate of the top margin of the crop bounding box.
 * @param[in] right X coordinate of the right margin of the crop bounding box.
 * @param[in] bottom Y coordinate of the bottom margin of the crop bounding box.
 * @note Passing zero to all parameters disables the crop functionality. This is also the default.
 * @note The bounding box defined by the parameters will be adjusted to fit within the dimensions of the \ref NWindow.
 * @note \p left must be less or equal than \p right.
 * @note \p top must be less or equal than \p bottom.
 */
Result nwindowSetCrop(NWindow* nw, s32 left, s32 top, s32 right, s32 bottom);

/**
 * @brief Configures the transformation applied to images presented through a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] transform Android transformation mode (see NATIVE_WINDOW_TRANSFORM_* enum)
 * @note The default transformation is 0 (i.e. no transformation applied)
 */
Result nwindowSetTransform(NWindow* nw, u32 transform);

/**
 * @brief Configures the swap interval of a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] swap_interval Value specifying the number of display refreshes (VBlanks) that must occur between presenting images.
 * @note The default swap interval is 1.
 * @note If the \ref NWindow has three or more buffers configured (with \ref nwindowConfigureBuffer), it is possible to pass 0
 *       to disable the swap interval feature and present images as fast as allowed by the compositor. Otherwise, the system
 *       enforces a minimum of 1 as the swap interval.
 */
Result nwindowSetSwapInterval(NWindow* nw, u32 swap_interval);

/// Checks whether the consumer of a \ref NWindow is running behind.
static inline bool nwindowIsConsumerRunningBehind(NWindow* nw)
{
    return nw->consumer_running_behind;
}

///@}

///@name Buffer configuration and presentation
///@{

/**
 * @brief Registers a \ref NvGraphicBuffer with a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] slot ID of the slot to configure (starting from 0).
 * @param[in] buf Pointer to \ref NvGraphicBuffer structure.
 * @note When a buffer is registered, it is added to the internal queue of buffers used for presenting.
 * @note All buffers registered with a \ref NWindow must have the same dimensions, format and usage.
 *       If \ref nwindowSetDimensions has not been previously called, the \ref NWindow will automatically
 *       adopt the dimensions of the first buffer registered with it. Otherwise, said buffer will need
 *       to match the dimensions that were previously configured.
 */
Result nwindowConfigureBuffer(NWindow* nw, s32 slot, NvGraphicBuffer* buf);

/**
 * @brief Dequeues a buffer from a \ref NWindow.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[out] out_slot Output variable containing the ID of the slot that has been dequeued.
 * @param[out] out_fence Output variable containing a \ref NvMultiFence that will be signalled by
 *             the compositor when the buffer is ready to be written to. Pass NULL to wait instead
 *             on this fence before this function returns.
 * @note For \p out_fence=NULL to work, \ref nvFenceInit must have been previously called.
 */
Result nwindowDequeueBuffer(NWindow* nw, s32* out_slot, NvMultiFence* out_fence);

/**
 * @brief Cancels a buffer previously dequeued with \ref nwindowDequeueBuffer.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] slot ID of the slot to cancel. This must match the output of the previous \ref nwindowDequeueBuffer call.
 * @param[in] fence Pointer to the \ref NvMultiFence that will be waited on by the compositor before cancelling the buffer.
 *                  Pass NULL if there is no such fence.
 */
Result nwindowCancelBuffer(NWindow* nw, s32 slot, const NvMultiFence* fence);

/**
 * @brief Queues a buffer previously dequeued with \ref nwindowDequeueBuffer, making it ready for presentation.
 * @param[in] nw Pointer to \ref NWindow structure.
 * @param[in] slot ID of the slot to queue. This must match the output of the previous \ref nwindowDequeueBuffer call.
 * @param[in] fence Pointer to the \ref NvMultiFence that will be waited on by the compositor before queuing/presenting the buffer.
 *                  Pass NULL if there is no such fence.
 */
Result nwindowQueueBuffer(NWindow* nw, s32 slot, const NvMultiFence* fence);

/// Releases all buffers registered with a \ref NWindow.
Result nwindowReleaseBuffers(NWindow* nw);

///@}
