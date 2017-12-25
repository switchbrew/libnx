typedef enum
{
    APPLET_TYPE_None = -2,
    APPLET_TYPE_Default = -1,
    APPLET_TYPE_Application = 0,
    APPLET_TYPE_SystemApplet = 1,
    APPLET_TYPE_LibraryApplet = 2,
    APPLET_TYPE_OverlayApplet = 3,
    APPLET_TYPE_SystemApplication = 4,
} appletType;

typedef enum
{
    APPLET_OperationMode_Handheld = 0,
    APPLET_OperationMode_Docked = 1,
} appletOperationMode;

/// applet hook types.
typedef enum {
	APPLETHOOK_ONFOCUSSTATE = 0,       ///< FocusState changed.
	APPLETHOOK_ONOPERATIONMODE,   ///< OperationMode changed.
	APPLETHOOK_ONPERFORMANCEMODE, ///< PerformanceMode changed.

	APPLETHOOK_COUNT,             ///< Number of applet hook types.
} applet_HookType;

/// applet hook function.
typedef void (*appletHookFn)(applet_HookType hook, void* param);

/// applet hook cookie.
typedef struct tag_appletHookCookie
{
	struct tag_appletHookCookie* next; ///< Next cookie.
	appletHookFn callback;             ///< Hook callback.
	void* param;                       ///< Callback parameter.
} appletHookCookie;

Result appletInitialize(void);
void appletExit(void);
Result appletGetAppletResourceUserId(u64 *out);

void appletNotifyRunning(u8 *out);
Result appletCreateManagedDisplayLayer(u64 *out);

/**
 * @brief Processes the current applet status. Generally used within a main loop.
 * @return Whether the application should continue running.
 */
bool appletMainLoop(void);


/**
 * @brief Sets up an applet status hook.
 * @param cookie Hook cookie to use.
 * @param callback Function to call when applet's status changes.
 * @param param User-defined parameter to pass to the callback.
 */
void appletHook(appletHookCookie* cookie, appletHookFn callback, void* param);

/**
 * @brief Removes an applet status hook.
 * @param cookie Hook cookie to remove.
 */
void appletUnhook(appletHookCookie* cookie);

/// These return state which is updated by appletMainLoop() when notifications are received.
u8 appletGetOperationMode(void);
u32 appletGetPerformanceMode(void);
u8 appletGetFocusState(void);
