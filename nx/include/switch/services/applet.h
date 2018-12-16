/**
 * @file applet.h
 * @brief Applet (applet) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../kernel/tmem.h"

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

typedef enum {
    AppletFocusState_Focused = 1,               ///< Applet is focused.
    AppletFocusState_NotFocusedUnknown = 2,     ///< Out of focus - unknown reason (LibraryApplet open?).
    AppletFocusState_NotFocusedHomeSleep = 3    ///< Out of focus - HOME menu is open / console is sleeping.
} AppletFocusState;

typedef enum {
  AppletFocusHandlingMode_Suspend1 = 0, ///< Suspend when out of focus.
  AppletFocusHandlingMode_NoSuspend,    ///< Don't suspend when out of focus.
  AppletFocusHandlingMode_Suspend2,     ///< Suspend when out of focus (unknown variant 1).
  AppletFocusHandlingMode_Suspend3,     ///< Suspend when out of focus (unknown variant 2).

  AppletFocusHandlingMode_Max,          //< Number of focus handling modes
} AppletFocusHandlingMode;

typedef enum {
    AppletLaunchParameterKind_Application     = 1, ///< Application-specific LaunchParameter
    AppletLaunchParameterKind_PreselectedUser = 2, ///< account PreselectedUser
    AppletLaunchParameterKind_Unknown         = 3, ///< Unknown if used by anything?
} AppletLaunchParameterKind;

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

/// applet IStorage
typedef struct {
    Service s;
    TransferMemory tmem;
} AppletStorage;

Result appletInitialize(void);
void appletExit(void);
Result appletGetAppletResourceUserId(u64 *out);
AppletType appletGetAppletType(void);

void appletNotifyRunning(u8 *out);
Result appletCreateManagedDisplayLayer(u64 *out);

/**
 * @brief Pops a LaunchParameter AppletStorage, the storage will be removed from sysmodule state during this.
 * @param s Output storage.
 * @param kind See \ref AppletLaunchParameterKind.
 * @note Can only be used in Applications.
 * @note See also acc.h \ref accountGetPreselectedUser (wrapper for appletPopLaunchParameter etc).
 */
Result appletPopLaunchParameter(AppletStorage *s, AppletLaunchParameterKind kind);

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
 * @brief Creates a storage.
 * @param s Storage object.
 * @param size Size of storage.
 */
Result appletCreateStorage(AppletStorage *s, s64 size);

/**
 * @brief Creates a TransferMemory storage.
 * @param s Storage object.
 * @param size Size of storage.
 * @param writable Controls whether writing to the storage is allowed with \ref appletStorageWrite.
 */
Result appletCreateTransferMemoryStorage(AppletStorage *s, s64 size, bool writable);

/**
 * @brief Creates a HandleStorage. Only available on 2.0.0+.
 * @param s Storage object.
 * @param inval Arbitrary input value.
 * @param handle Arbitrary input handle.
 */
Result appletCreateHandleStorage(AppletStorage *s, s64 inval, Handle handle);

/**
 * @brief Creates a HandleStorage using TransferMemory. Wrapper for \ref appletCreateHandleStorage.
 * @param s Storage object.
 * @param size Size of storage.
 */
Result appletCreateHandleStorageTmem(AppletStorage *s, s64 size);

/// Closes the storage object. TransferMemory closing is seperate, see \ref appletStorageCloseTmem.
void appletStorageClose(AppletStorage *s);

/// Closes the TransferMemory in the storage object. For TransferMemory storage created by the current process, this must be called after the LibraryApplet finishes using it (if sent to one).
void appletStorageCloseTmem(AppletStorage *s);

/// Gets the size of the storage. This is not usable with HandleStorage, use \ref appletStorageGetHandle or \ref appletStorageMap instead for that.
Result appletStorageGetSize(AppletStorage *s, s64 *size);

/**
 * @brief Writes to a storage. offset(+size) must be within the actual storage size.
 * @note  This is not usable with HandleStorage.
 * @param s Storage object.
 * @param offset Offset in storage.
 * @param buffer Input data.
 * @param size Data size.
 */
Result appletStorageWrite(AppletStorage *s, s64 offset, const void* buffer, size_t size);

/**
 * @brief Reads from a storage. offset(+size) must be within the actual storage size.
 * @note  This is not usable with HandleStorage.
 * @param s Storage object.
 * @param offset Offset in storage.
 * @param buffer Input data.
 * @param size Data size.
 */
Result appletStorageRead(AppletStorage *s, s64 offset, void* buffer, size_t size);

/**
 * @brief Gets data for a HandleStorage originally from \ref appletCreateHandleStorage input.
 * @note  Only available on 2.0.0+.
 * @param out Output value.
 * @param handle Output handle.
 */
Result appletStorageGetHandle(AppletStorage *s, s64 *out, Handle *handle);

/**
 * @brief Maps TransferMemory for a HandleStorage. Wrapper for \ref appletCreateHandleStorage.
 * @note  The TransferMemory can be unmapped with \ref appletStorageCloseTmem.
 * @note  Do not use this if the AppletStorage already contains initialized TransferMemory state.
 * @param s Storage object.
 * @param addr Output mapped address (optional).
 * @param size Output size (optional).
 */
Result appletStorageMap(AppletStorage *s, void** addr, size_t *size);

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
AppletFocusState appletGetFocusState(void);

Result appletSetFocusHandlingMode(AppletFocusHandlingMode mode);
