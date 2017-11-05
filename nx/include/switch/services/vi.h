typedef struct {
    u64 DisplayId;
    char DisplayName[0x40];
    bool initialized;
} viDisplay;

typedef struct {
    u64 LayerId;
    bool StrayLayer;
    bool initialized;
} viLayer;

typedef enum {
	VISERVTYPE_Default = -1,
	VISERVTYPE_Application = 0,
	VISERVTYPE_System = 1,
	VISERVTYPE_Manager = 2,
} viServiceType;

/// Used by viOpenLayer when CreateStrayLayer is used internally.
typedef enum {
    VILAYERFLAGS_Default = 0x1,
} viLayerFlags;

/// Used with viSetLayerScalingMode.
typedef enum {
    VISCALINGMODE_Default = 0x2,
} viScalingMode;

Result viInitialize(viServiceType servicetype);
void viExit(void);
Handle viGetSessionService(void);
Handle viGetSession_IApplicationDisplayService(void);
Handle viGetSession_IHOSBinderDriverRelay(void);
Handle viGetSession_ISystemDisplayService(void);
Handle viGetSession_IManagerDisplayService(void);
Handle viGetSession_IHOSBinderDriverIndirect(void);

Result viOpenDisplay(const char *DisplayName, viDisplay *display);
Result viCloseDisplay(viDisplay *display);
Result viOpenLayer(u8 NativeWindow[0x100], u64 *NativeWindow_Size, const viDisplay *display, viLayer *layer, u32 LayerFlags, u64 LayerId);
Result viCloseLayer(viLayer *layer);

/// See viScalingMode.
Result viSetLayerScalingMode(viLayer *layer, u32 ScalingMode);

Result viGetDisplayVsyncEvent(viDisplay *display, Handle *handle_out);
