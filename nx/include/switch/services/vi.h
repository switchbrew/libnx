/**
 * @file vi.h
 * @brief Display (vi:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/sm.h"

typedef struct {
    u64  display_id;
    char display_name[0x40];
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
    ViScalingMode_Default = 0x2,
} ViScalingMode;

Result viInitialize(ViServiceType service_type);
void viExit(void);

Service* viGetSession_IApplicationDisplayService(void);
Service* viGetSession_IHOSBinderDriverRelay(void);
Service* viGetSession_ISystemDisplayService(void);
Service* viGetSession_IManagerDisplayService(void);
Service* viGetSession_IHOSBinderDriverIndirect(void);

// Display functions

Result viOpenDisplay(const char *display_name, ViDisplay *display);
Result viCloseDisplay(ViDisplay *display);

static inline Result viOpenDefaultDisplay(ViDisplay *display)
{
    return viOpenDisplay("Default", display);
}

Result viGetDisplayResolution(ViDisplay *display, u64 *width, u64 *height);
Result viGetDisplayVsyncEvent(ViDisplay *display, Event *event_out);

// Layer functions

Result viCreateLayer(const ViDisplay *display, ViLayer *layer);
Result viCreateManagedLayer(const ViDisplay *display, ViLayerFlags layer_flags, u64 aruid, u64 *layer_id);
Result viCloseLayer(ViLayer *layer);

Result viSetLayerScalingMode(ViLayer *layer, ViScalingMode scaling_mode);
