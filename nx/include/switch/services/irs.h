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
} PACKED irsPackedMomentProcessorConfig;

typedef struct {
    //exposure controls the sensor's exposure time in microseconds.
    u64 exposure;
    //irLedsMode controls which group of IR leds is enabled. Can be all, group 1, group 2 or none.
    u32 irLeds;
    //digitalGain controls the signal's digital gain.
    u32 digitalGain;
    //negative inverts the captured image.
    u8  negative;
    u8  pad[7];
    //sensorResolution controls the sensor's resolution.
    u32 sensorResolution;
} irsImageTransferProcessorConfig;

typedef struct {
    //exposure controls the sensor's exposure time in microseconds.
    u64 exposure;
    //irLedsMode controls which group of IR leds is enabled. Can be all, group 1, group 2 or none.
    u8  irLeds;
    //digitalGain controls the signal's digital gain.
    u8  digitalGain;
     //negative inverts the captured image.
    u8  negative;
    u8  pad[5];
    u32 unk_constant;//offset 0x10
    //sensorResolution controls the sensor's resolution.
    u8  sensorResolution;
    u8  pad2[3];
} irsPackedImageTransferProcessorConfig;

typedef struct {
    u8 unk_x0[0x10];
} PACKED irsImageTransferProcessorState;

Result irsInitialize(void);
void irsExit(void);

Service* irsGetSessionService(void);
void* irsGetSharedmemAddr(void);

/// (De)activate the IR sensor, this is automatically used by irsExit(). Must be called after irsInitialize() to activate the IR sensor.
Result irsActivateIrsensor(bool activate);

Result irsGetIrCameraHandle(u32 *IrCameraHandle, HidControllerID id);

/**
 * @brief Start ImageTransferProcessor.
 * @param[in] IrCameraHandle Camera handle.
 * @param[in] config Input config.
 * @param[in] size Work-buffer size, must be 0x1000-byte aligned.
 * @note Do not use if already started.
 */
Result irsRunImageTransferProcessor(u32 IrCameraHandle, irsImageTransferProcessorConfig *config, size_t size);

Result irsGetImageTransferProcessorState(u32 IrCameraHandle, void* buffer, size_t size, irsImageTransferProcessorState *state);

/// Stop ImageTransferProcessor. Do not use if already stopped.
/// \ref irsExit calls this with all IrCameraHandles which were not already used with \ref irsStopImageProcessor.
Result irsStopImageProcessor(u32 IrCameraHandle);

/// "Suspend" ImageTransferProcessor.
/// TODO: What does this really do?
Result irsSuspendImageProcessor(u32 IrCameraHandle);

void irsGetDefaultImageTransferProcessorConfig(irsImageTransferProcessorConfig *config);
