/**
 * @file applet.h
 * @brief Applet (applet) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum {
    AppletType_None = -2,
    AppletType_Default = -1,
    AppletType_Application = 0,
    AppletType_SystemApplet = 1,
    AppletType_LibraryApplet = 2,
    AppletType_OverlayApplet = 3,
    AppletType_SystemApplication = 4,
} AppletType;

typedef enum {
    AppletOperationMode_Handheld = 0,
    AppletOperationMode_Docked = 1,
} AppletOperationMode;

/// applet hook types.
typedef enum {
    AppletHookType_OnFocusState = 0,  ///< FocusState changed.
    AppletHookType_OnOperationMode,   ///< OperationMode changed.
    AppletHookType_OnPerformanceMode, ///< PerformanceMode changed.
    AppletHookType_OnExitRequest,     ///< Exit requested.

    AppletHookType_Max,               ///< Number of applet hook types.
} AppletHookType;

/// applet hook function.
typedef void (*AppletHookFn)(AppletHookType hook, void* param);

/// applet hook cookie.
typedef struct AppletHookCookie AppletHookCookie;

struct AppletHookCookie
{
    AppletHookCookie* next;  ///< Next cookie.
    AppletHookFn callback;   ///< Hook callback.
    void* param;             ///< Callback parameter.
};

Result appletInitialize(void);
void appletExit(void);
Result appletGetAppletResourceUserId(u64 *out);
AppletType appletGetAppletType(void);

void appletNotifyRunning(u8 *out);
Result appletCreateManagedDisplayLayer(u64 *out);

Result appletGetDesiredLanguage(u64 *LanguageCode);

/// Gets whether video recording is supported.
/// See also \ref appletInitializeGamePlayRecording.
Result appletIsGamePlayRecordingSupported(bool *flag);

/// Disable/enable video recording. Only available after \ref appletInitializeGamePlayRecording was used.
/// See also \ref appletInitializeGamePlayRecording.
Result appletSetGamePlayRecordingState(bool state);

/// Initializes video recording. This allocates a 0x6000000-byte buffer for the TransferMemory, cleanup is handled automatically during app exit in \ref appletExit.
/// Only available with AppletType_Application on 3.0.0+, hence errors from this can be ignored.
/// Video recording is only fully available system-side with 4.0.0+.
/// Only usable when running under a title which supports video recording.
Result appletInitializeGamePlayRecording(void);

/**
 * @brief Blocks the usage of the home button.
 * @param val Unknown nanoseconds. Value 0 can be used.
 * @note Can only be used in regularapps.
 */
Result appletBeginBlockingHomeButton(s64 val);

Result appletEndBlockingHomeButton(void);

/**
 * @brief Delay exiting until \ref appletUnlockExit is called, with a 15 second timeout once exit is requested.
 * @note When exit is requested \ref appletMainLoop will return false, hence any main-loop using appletMainLoop will exit. This allows the app to handle cleanup post-main-loop instead of being force-terminated.
 * @note If the above timeout occurs after exit was requested where \ref appletUnlockExit was not called, the process will be forced-terminated.
 * @note \ref appletUnlockExit must be used before main() returns.
 */
Result appletLockExit(void);
Result appletUnlockExit(void);

/**
 * @brief Controls whether screenshot-capture is allowed.
 * @param val 0 = disable, 1 = enable.
 */
Result appletSetScreenShotPermission(s32 val);

Result appletSetScreenShotImageOrientation(s32 val);

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
void appletHook(AppletHookCookie* cookie, AppletHookFn callback, void* param);

/**
 * @brief Removes an applet status hook.
 * @param cookie Hook cookie to remove.
 */
void appletUnhook(AppletHookCookie* cookie);

/// These return state which is updated by appletMainLoop() when notifications are received.
u8  appletGetOperationMode(void);
u32 appletGetPerformanceMode(void);
u8  appletGetFocusState(void);
