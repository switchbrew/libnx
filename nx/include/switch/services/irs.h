/**
 * @file irs.h
 * @brief HID IR sensor (irs) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../services/sm.h"
#include "../services/hid.h"

typedef struct {
    u64 unk_x0;
    u8  unk_x8;
    u8  unk_x9;
    u8  unk_xa;
    u8  pad[5];
    u16 unk_x10;
    u32 unk_x12;
    u16 unk_x16;
    u32 unk_constant;//offset 0x18
    u8  unk_x1c;
    u8  unk_x1d;
    u8  pad2[2];
} PACKED IrsPackedMomentProcessorConfig;

typedef struct {
    u64 exposure;     ///< IR Sensor exposure time in nanoseconds.
    u32 ir_leds;      ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u32 digital_gain; ///< IR sensor signal's digital gain.
    u8  color_invert; ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8  pad[7];
    u32 sensor_res;   ///< IR Sensor resolution. 0: 240x320, 1: 120x160, 2: 60x80.
} IrsImageTransferProcessorConfig;

typedef struct {
    u64 exposure;     ///< IR Sensor exposure time in nanoseconds.
    u8  ir_leds;      ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u8  digital_gain; ///< IR sensor signal's digital gain.
    u8  color_invert; ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8  pad[5];
    u32 unk_constant;//offset 0x10
    u8  sensor_res;   ///< IR Sensor resolution. 0: 240x320, 1: 120x160, 2: 60x80.
    u8  pad2[3];
} IrsPackedImageTransferProcessorConfig;

typedef struct {
    u8 unk_x0[0x10];
} PACKED IrsImageTransferProcessorState;

/// Initialize irs.
Result irsInitialize(void);

/// Exit irs.
void irsExit(void);

Service* irsGetServiceSession(void);
void* irsGetSharedmemAddr(void);

/// (De)activate the IR sensor, this is automatically used by \ref irsExit. Must be called after irsInitialize() to activate the IR sensor.
Result irsActivateIrsensor(bool activate);

Result irsGetIrCameraHandle(u32 *IrCameraHandle, HidControllerID id);

/**
 * @brief Start ImageTransferProcessor.
 * @param[in] IrCameraHandle Camera handle.
 * @param[in] config Input config.
 * @param[in] size Work-buffer size, must be 0x1000-byte aligned.
 * @note Do not use if already started.
 */
Result irsRunImageTransferProcessor(u32 IrCameraHandle, IrsImageTransferProcessorConfig *config, size_t size);

Result irsGetImageTransferProcessorState(u32 IrCameraHandle, void* buffer, size_t size, IrsImageTransferProcessorState *state);

/// Stop ImageTransferProcessor. Do not use if already stopped.
/// \ref irsExit calls this with all IrCameraHandles which were not already used with \ref irsStopImageProcessor.
Result irsStopImageProcessor(u32 IrCameraHandle);

/// "Suspend" ImageTransferProcessor.
/// TODO: What does this really do?
Result irsSuspendImageProcessor(u32 IrCameraHandle);

/** 
 * Gets the default configuration for Image Transfer mode.
 * Defaults are exposure 300us, IR LEDs all ON, 8x digital gain, normal image and resolution 240 x 320.
 */
void irsGetDefaultImageTransferProcessorConfig(IrsImageTransferProcessorConfig *config);
