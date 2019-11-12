/**
 * @file capssc.h
 * @brief Screenshot control (caps:sc) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize caps:sc. Only available on [2.0.0+].
Result capsscInitialize(void);

/// Exit caps:sc.
void capsscExit(void);

/// Gets the Service for caps:sc.
Service* capsscGetServiceSession(void);

/**
 * @brief This takes a screenshot, with the screenshot being written into the output buffer.
 * @note Not available with [5.0.0+] (stubbed).
 * @note buffer_index and buffer_count correspond to buffers with size 0x384000(1280*720*4). These must not be negative.
 * @param buf Output buffer containing the RGBA8 image.
 * @param size Size of buf, should be 0x384000(1280*720*4) * buffer_count.
 * @param layer_stack Value 0 can be used for this.
 * @param width Image width, must be 1280.
 * @param height Image height, must be 720.
 * @param buffer_count Total number of output image buffers.
 * @param buffer_index Starting image buffer index. Must be < buffer_count.
 * @param timeout Timeout in nanoseconds. A default value of 100000000 can be used.
 */
Result capsscCaptureRawImageWithTimeout(void* buf, size_t size, u32 layer_stack, u64 width, u64 height, s64 buffer_count, s64 buffer_index, u64 timeout);

