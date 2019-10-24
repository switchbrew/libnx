/**
 * @file vi.h
 * @brief Display (vi:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

typedef struct {
    char data[0x40];
} ViDisplayName;

typedef struct {
    u64  display_id;
    ViDisplayName display_name;
    bool initialized;
} ViDisplay;

typedef struct {
    u64  layer_id;
    u32  igbp_binder_obj_id;
    bool initialized : 1;
    bool stray_layer : 1;
} ViLayer;

typedef enum {
    ViServiceType_Default = -1,
    ViServiceType_Application = 0,
    ViServiceType_System = 1,
    ViServiceType_Manager = 2,
} ViServiceType;

/// Used by viCreateLayer when CreateStrayLayer is used internally.
typedef enum {
    ViLayerFlags_Default = 0x1,
} ViLayerFlags;

/// Used with viSetLayerScalingMode.
typedef enum {
    ViScalingMode_None = 0x0,
    ViScalingMode_FitToLayer = 0x2,
    ViScalingMode_PreserveAspectRatio = 0x4,

    ViScalingMode_Default = ViScalingMode_FitToLayer,
} ViScalingMode;

/// Used with viSetDisplayPowerState.
typedef enum {
    ViPowerState_Off           = 0, ///< Screen is off.
    ViPowerState_NotScanning   = 1, ///< [3.0.0+] Screen is on, but not scanning content.
    ViPowerState_On            = 2, ///< [3.0.0+] Screen is on.

    ViPowerState_On_Deprecated = 1, ///< [1.0.0 - 2.3.0] Screen is on.
} ViPowerState;

Result viInitialize(ViServiceType service_type);
void viExit(void);

Service* viGetSession_IApplicationDisplayService(void);
Service* viGetSession_IHOSBinderDriverRelay(void);
Service* viGetSession_ISystemDisplayService(void);
Service* viGetSession_IManagerDisplayService(void);
Service* viGetSession_IHOSBinderDriverIndirect(void);

// Misc functions
Result viSetContentVisibility(bool v);

// Display functions

Result viOpenDisplay(const char *display_name, ViDisplay *display);
Result viCloseDisplay(ViDisplay *display);

static inline Result viOpenDefaultDisplay(ViDisplay *display)
{
    return viOpenDisplay("Default", display);
}

Result viGetDisplayResolution(ViDisplay *display, s32 *width, s32 *height);
Result viGetDisplayLogicalResolution(ViDisplay *display, s32 *width, s32 *height);
/// Only available on [3.0.0+].
Result viSetDisplayMagnification(ViDisplay *display, s32 x, s32 y, s32 width, s32 height);
Result viGetDisplayVsyncEvent(ViDisplay *display, Event *event_out);
Result viSetDisplayPowerState(ViDisplay *display, ViPowerState state);
Result viSetDisplayAlpha(ViDisplay *display, float alpha);
Result viGetZOrderCountMin(ViDisplay *display, s32 *z);
Result viGetZOrderCountMax(ViDisplay *display, s32 *z);

// Layer functions

Result viCreateLayer(const ViDisplay *display, ViLayer *layer);
Result viCreateManagedLayer(const ViDisplay *display, ViLayerFlags layer_flags, u64 aruid, u64 *layer_id);
Result viSetLayerSize(ViLayer *layer, s32 width, s32 height);
Result viSetLayerZ(ViLayer *layer, s32 z);
Result viSetLayerPosition(ViLayer *layer, float x, float y);
Result viCloseLayer(ViLayer *layer);
Result viDestroyManagedLayer(ViLayer *layer);

Result viSetLayerScalingMode(ViLayer *layer, ViScalingMode scaling_mode);

// IndirectLayer functions

Result viGetIndirectLayerImageMap(void* buffer, size_t size, s32 width, s32 height, u64 IndirectLayerConsumerHandle, u64 *out_size, u64 *out_stride);
Result viGetIndirectLayerImageRequiredMemoryInfo(s32 width, s32 height, u64 *out_size, u64 *out_alignment);
