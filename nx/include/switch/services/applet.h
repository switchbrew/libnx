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
