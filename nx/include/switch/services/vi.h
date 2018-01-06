typedef struct {
    u64  display_id;
    char display_name[0x40];
    bool initialized;
} ViDisplay;

typedef struct {
    u64  layer_id;
    bool stray_layer;
    bool initialized;
} ViLayer;

typedef enum {
    ViServiceType_Default = -1,
    ViServiceType_Application = 0,
    ViServiceType_System = 1,
    ViServiceType_Manager = 2,
} ViServiceType;

/// Used by viOpenLayer when CreateStrayLayer is used internally.
typedef enum {
    ViLayerFlags_Default = 0x1,
} ViLayerFlags;

/// Used with viSetLayerScalingMode.
typedef enum {
    ViScalingMode_Default = 0x2,
} ViScalingMode;

Result viInitialize(ViServiceType servicetype);
void viExit(void);
Handle viGetSessionService(void);
Handle viGetSession_IApplicationDisplayService(void);
Handle viGetSession_IHOSBinderDriverRelay(void);
Handle viGetSession_ISystemDisplayService(void);
Handle viGetSession_IManagerDisplayService(void);
Handle viGetSession_IHOSBinderDriverIndirect(void);

Result viOpenDisplay(const char *DisplayName, ViDisplay *display);
Result viCloseDisplay(ViDisplay *display);
Result viOpenLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const ViDisplay *display, ViLayer *layer, u32 LayerFlags, u64 LayerId);
Result viCloseLayer(ViLayer *layer);

/// See ViScalingMode.
Result viSetLayerScalingMode(ViLayer *layer, u32 ScalingMode);

Result viGetDisplayResolution(ViDisplay *display, u64 *width, u64 *height);
Result viGetDisplayVsyncEvent(ViDisplay *display, Handle *handle_out);
