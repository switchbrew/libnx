/**
 * @file applet.h
 * @brief Applet (applet) service IPC wrapper.
 * @note For wrappers which launch LibraryApplets etc, see switch/applets/.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/apm.h"
#include "../kernel/tmem.h"
#include "../kernel/event.h"

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
    AppletFocusState_Focused = 1,                   ///< Applet is focused.
    AppletFocusState_NotFocusedLibraryApplet = 2,   ///< Out of focus - LibraryApplet open.
    AppletFocusState_NotFocusedHomeSleep = 3        ///< Out of focus - HOME menu open / console is sleeping.
} AppletFocusState;

typedef enum {
  AppletFocusHandlingMode_SuspendHomeSleep = 0,       ///< Suspend only when HOME menu is open / console is sleeping (default).
  AppletFocusHandlingMode_NoSuspend,                  ///< Don't suspend when out of focus.
  AppletFocusHandlingMode_SuspendHomeSleepNotify,     ///< Suspend only when HOME menu is open / console is sleeping but still receive OnFocusState hook.
  AppletFocusHandlingMode_AlwaysSuspend,              ///< Always suspend when out of focus, regardless of the reason.

  AppletFocusHandlingMode_Max,                        ///< Number of focus handling modes.
} AppletFocusHandlingMode;

typedef enum {
    AppletLaunchParameterKind_Application     = 1, ///< Application-specific LaunchParameter
    AppletLaunchParameterKind_PreselectedUser = 2, ///< account PreselectedUser
    AppletLaunchParameterKind_Unknown         = 3, ///< Unknown if used by anything?
} AppletLaunchParameterKind;

typedef enum {
    AppletId_overlayDisp = 0x02,    ///< 010000000000100C "overlayDisp"
    AppletId_qlaunch = 0x03,        ///< 0100000000001000 "qlaunch" (SystemAppletMenu)
    AppletId_starter = 0x04,        ///< 0100000000001012 "starter"
    AppletId_auth = 0x0A,           ///< 0100000000001001 "auth"
    AppletId_cabinet = 0x0B,        ///< 0100000000001002 "cabinet"
    AppletId_controller = 0x0C,     ///< 0100000000001003 "controller"
    AppletId_dataErase = 0x0D,      ///< 0100000000001004 "dataErase"
    AppletId_error = 0x0E,          ///< 0100000000001005 "error"
    AppletId_netConnect = 0x0F,     ///< 0100000000001006 "netConnect"
    AppletId_playerSelect = 0x10,   ///< 0100000000001007 "playerSelect"
    AppletId_swkbd = 0x11,          ///< 0100000000001008 "swkbd"
    AppletId_miiEdit = 0x12,        ///< 0100000000001009 "miiEdit"
    AppletId_web = 0x13,            ///< 010000000000100A "LibAppletWeb" WebApplet applet
    AppletId_shop = 0x14,           ///< 010000000000100B "LibAppletShop" ShopN applet
    AppletId_photoViewer = 0x15,    ///< 010000000000100D "photoViewer"
    AppletId_set = 0x16,            ///< 010000000000100E "set" (This title is currently not present on retail devices.)
    AppletId_offlineWeb = 0x17,     ///< 010000000000100F "LibAppletOff" Offline web-applet
    AppletId_loginShare = 0x18,     ///< 0100000000001010 "LibAppletLns" Whitelisted web-applet
    AppletId_wifiWebAuth = 0x19,    ///< 0100000000001011 "LibAppletAuth" WifiWebAuth applet
    AppletId_myPage = 0x1A,         ///< 0100000000001013 "myPage"
} AppletId;

/// LibraryAppletMode
typedef enum {
    LibAppletMode_AllForeground = 0,  ///< Foreground
    LibAppletMode_Background = 1,     ///< Background
    LibAppletMode_Unknown2 = 2,       ///< Unknown
    LibAppletMode_Unknown3 = 3,       ///< Unknown
} LibAppletMode;

/// LibraryAppletExitReason
typedef enum {
    LibAppletExitReason_Normal = 0,
    LibAppletExitReason_Canceled = 1,
    LibAppletExitReason_Abnormal = 2,
    LibAppletExitReason_Unexpected = 10,
} LibAppletExitReason;

/// ThemeColorType
typedef enum {
    AppletThemeColorType_Default = 0,
    AppletThemeColorType_Unknown1 = 1,
    AppletThemeColorType_Unknown2 = 2,
    AppletThemeColorType_Unknown3 = 3,
} AppletThemeColorType;

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

/// LibraryApplet state.
typedef struct {
    Service s;                         ///< ILibraryAppletAccessor
    Event StateChangedEvent;           ///< Output from GetAppletStateChangedEvent, autoclear=false.
    Event PopInteractiveOutDataEvent;  ///< Output from GetPopInteractiveOutDataEvent, autoclear=false.
    LibAppletMode mode;                ///< See ref \ref LibAppletMode.
    u64 layer_handle;                  ///< Output from GetIndirectLayerConsumerHandle on 2.0.0+.
    bool creating_self;                ///< When set, indicates that the LibraryApplet title is creating itself.
    LibAppletExitReason exitreason;    ///< Set by \ref appletHolderJoin using the output from cmd GetResult, see \ref LibAppletExitReason.
} AppletHolder;

/// 'pdm' ApplicationPlayStatistics
typedef struct {
    u8 unk_x0[0x8];
} AppletApplicationPlayStatistics;

/// Attributes for launching applications for Quest.
typedef struct {
    u32 unk_x0;
    u32 unk_x4;
} AppletApplicationAttributeForQuest;

/// Initialize applet, called automatically during app startup.
Result appletInitialize(void);

/// Exit applet, called automatically during app startup.
void appletExit(void);

Result appletGetAppletResourceUserId(u64 *out);
AppletType appletGetAppletType(void);

void appletNotifyRunning(u8 *out);
Result appletCreateManagedDisplayLayer(u64 *out);

/// Sets the state field for \ref AppletThemeColorType.
void appletSetThemeColorType(AppletThemeColorType theme);

/// Gets the state field for \ref AppletThemeColorType. Used internally by \ref libappletArgsCreate.
AppletThemeColorType appletGetThemeColorType(void);

/**
 * @brief Pops a LaunchParameter AppletStorage, the storage will be removed from sysmodule state during this.
 * @param s Output storage.
 * @param kind See \ref AppletLaunchParameterKind.
 * @note Can only be used in Applications.
 * @note See also acc.h \ref accountGetPreselectedUser (wrapper for appletPopLaunchParameter etc).
 */
Result appletPopLaunchParameter(AppletStorage *s, AppletLaunchParameterKind kind);

/**
 * @brief Requests to launch the specified application.
 * @note Only available with AppletType_*Application, or AppletType_LibraryApplet on 5.0.0+.
 * @param[in] titleID Application titleID. Value 0 can be used to relaunch the current application.
 * @param s Optional AppletStorage object, can be NULL. This is automatically closed. When NULL on pre-4.0.0 (or with AppletType_LibraryApplet), this will internally create a tmp storage with size 0 for use with the cmd. This is the storage available to the launched application via \ref appletPopLaunchParameter with ::AppletLaunchParameterKind_Application.
 */
Result appletRequestLaunchApplication(u64 titleID, AppletStorage* s);

/**
 * @brief Requests to launch the specified application, for kiosk systems.
 * @note Only available with AppletType_*Application on 3.0.0+.
 * @param[in] titleID Application titleID
 * @param s Optional AppletStorage object, can be NULL. This is automatically closed. When NULL on pre-4.0.0, this will internally create a tmp storage with size 0 for use with the cmd. This is the storage available to the launched application via \ref appletPopLaunchParameter with ::AppletLaunchParameterKind_Application.
 * @param[in] attr Kiosk application attributes.
 */
Result appletRequestLaunchApplicationForQuest(u64 titleID, AppletStorage* s, const AppletApplicationAttributeForQuest *attr);

Result appletGetDesiredLanguage(u64 *LanguageCode);

/// Only available with AppletType_*Application.
Result appletSetTerminateResult(Result res);

/// Set media playback state.
/// If state is set to true, screen dimming and auto sleep is disabled.
/// For *Application, this uses cmd SetMediaPlaybackStateForApplication, otherwise cmd SetMediaPlaybackState is used.
Result appletSetMediaPlaybackState(bool state);

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
 * @brief Gets ApplicationPlayStatistics.
 * @note Only available with AppletType_*Application on 5.0.0+.
 * @note This may return no output in some cases.
 * @param stats Output \ref AppletApplicationPlayStatistics array.
 * @param titleIDs Input titleIDs array.
 * @param count Total entries in the input/output arrays.
 * @param out Output s32.
 */
Result appletQueryApplicationPlayStatistics(AppletApplicationPlayStatistics *stats, const u64 *titleIDs, s32 count, s32 *out);

/**
 * @brief Delay exiting until \ref appletUnlockExit is called, with a 15 second timeout once exit is requested.
 * @note When exit is requested \ref appletMainLoop will return false, hence any main-loop using appletMainLoop will exit. This allows the app to handle cleanup post-main-loop instead of being force-terminated.
 * @note If the above timeout occurs after exit was requested where \ref appletUnlockExit was not called, the process will be forced-terminated.
 * @note \ref appletUnlockExit must be used before main() returns.
 */
Result appletLockExit(void);

/// Unlocks exiting, see \ref appletLockExit.
Result appletUnlockExit(void);

/**
 * @brief Controls whether screenshot-capture is allowed.
 * @param val 0 = disable, 1 = enable.
 */
Result appletSetScreenShotPermission(s32 val);

Result appletSetScreenShotImageOrientation(s32 val);

/**
 * @brief Gets the current Illuminance from the light sensor.
 * @note Only available with [3.0.0+].
 * @param fLux Output fLux
 */
Result appletGetCurrentIlluminance(float *fLux);

/**
 * @brief Gets the current Illuminance from the light sensor. Same as \ref appletGetCurrentIlluminance except for the additional param.
 * @note Only available with [5.0.0+].
 * @param bOverLimit Output bOverLimit
 * @param fLux Output fLux
 */
Result appletGetCurrentIlluminanceEx(bool *bOverLimit, float *fLux);

/**
 * @brief Gets whether Illuminance is available.
 * @note Only available with [3.0.0+].
 * @param out Output flag
 */
Result appletIsIlluminanceAvailable(bool *out);

/**
 * @brief Stops forwarding the input to the foreground app, works only in the Overlay applet context.
 * @note You have to call this to receive inputs through the hid service when running as the overlay applet.
 */
Result appletBeginToWatchShortHomeButtonMessage(void);

/**
 * @brief Forwards input to the foreground app, works only in the Overlay applet context.
 * @note After calling this the overlay applet won't receive any input until \ref appletBeginToWatchShortHomeButtonMessage is called again.
 */
Result appletEndToWatchShortHomeButtonMessage(void);

/**
 * @brief Get an event that fires when the home button is pressed, doesn't interfere with home menu. This event does not auto clear.
 * @note Doesn't fire for long press.
 */
Result appletHomeButtonReaderLockAccessorGetEvent(Event *out_event);

/**
 * @brief Pushes a storage to the general channel. Used for sending requests to qlaunch.
 * @note  This is not usable under an Application, however it is usable under a LibraryApplet.
 * @note  This uses \ref appletStorageClose automatically.
 * @param s Storage object.
 */
Result appletPushToGeneralChannel(AppletStorage *s);

/**
 * @brief Gets whether VrMode is enabled.
 * @note Only available with [3.0.0+].
 * @param out Output flag
 */
Result appletIsVrModeEnabled(bool *out);

/**
 * @brief Sets whether VrMode is enabled.
 * @note This is only fully usable system-side with [6.0.0+].
 * @note For checking Parental Controls, see \ref pctlIsStereoVisionPermitted.
 * @note On pre-7.0.0 this uses cmd SetVrModeEnabled internally, while on [7.0.0+] this uses cmds BeginVrModeEx/EndVrModeEx.
 * @param flag Flag
 */
Result appletSetVrModeEnabled(bool flag);

/**
 * @brief Sets the \ref ApmCpuBoostMode.
 * @note Only available with [7.0.0+] (not fully usable system-side with 6.x).
 * @param mode \ref ApmCpuBoostMode.
 */
Result appletSetCpuBoostMode(ApmCpuBoostMode mode);

/**
 * @brief Gets the current PerformanceConfiguration.
 * @note Only available with [7.0.0+].
 * @param PerformanceConfiguration Output PerformanceConfiguration.
 */
Result appletGetCurrentPerformanceConfiguration(u32 *PerformanceConfiguration);

/**
 * @brief Creates a LibraryApplet.
 * @param h AppletHolder object.
 * @param id See \ref AppletId.
 * @param mode See \ref LibAppletMode.
 */
Result appletCreateLibraryApplet(AppletHolder *h, AppletId id, LibAppletMode mode);

/**
 * @brief Creates a LibraryApplet. This is for when a LibraryApplet title creates itself.
 * @note  Identical to \ref appletCreateLibraryApplet except this sets the creating_self flag to true.
 * @param h AppletHolder object.
 * @param id See \ref AppletId.
 * @param mode See \ref LibAppletMode.
 */
Result appletCreateLibraryAppletSelf(AppletHolder *h, AppletId id, LibAppletMode mode);

/// Closes an AppletHolder object.
void appletHolderClose(AppletHolder *h);

/// Returns whether the AppletHolder object was initialized.
bool appletHolderActive(AppletHolder *h);

/**
 * @brief Gets the IndirectLayerConsumerHandle loaded during \ref appletCreateLibraryApplet, on 2.0.0+.
 * @note  Only available when \ref LibAppletMode is ::LibAppletMode_Unknown3.
 * @param h AppletHolder object.
 * @param out Output IndirectLayerConsumerHandle.
 */
Result appletHolderGetIndirectLayerConsumerHandle(AppletHolder *h, u64 *out);

/**
 * @brief Starts the LibraryApplet.
 * @param h AppletHolder object.
 */
Result appletHolderStart(AppletHolder *h);

/**
 * @brief Requests the LibraryApplet to exit. The command is only used if \ref appletHolderCheckFinished returns false.
 * @param h AppletHolder object.
 */
Result appletHolderRequestExit(AppletHolder *h);

/**
 * @brief Waits for the LibraryApplet to exit.
 * @param h AppletHolder object.
 */
void appletHolderJoin(AppletHolder *h);

/**
 * @brief Waits on the LibraryApplet StateChangedEvent with timeout=0, and returns whether it was successful.
 * @param h AppletHolder object.
 */
bool appletHolderCheckFinished(AppletHolder *h);

/**
 * @brief Gets the \ref LibAppletExitReason set by \ref appletHolderJoin.
 * @param h AppletHolder object.
 */
LibAppletExitReason appletHolderGetExitReason(AppletHolder *h);

/**
 * @brief Waits for the PopInteractiveOutDataEvent and StateChangedEvent.
 * @return false for error / when StateChangedEvent was signaled, and true when PopInteractiveOutDataEvent was signaled.
 * @param h AppletHolder object.
 */
bool appletHolderWaitInteractiveOut(AppletHolder *h);

/**
 * @brief Pushes a storage for LibraryApplet input.
 * @note  This uses \ref appletStorageClose automatically.
 * @param h AppletHolder object.
 * @param s Storage object.
 */
Result appletHolderPushInData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pops a storage from LibraryApplet output.
 * @param h AppletHolder object.
 * @param s Storage object.
 */
Result appletHolderPopOutData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pushes a storage for LibraryApplet Extra storage input.
 * @note  This uses \ref appletStorageClose automatically.
 * @param h AppletHolder object.
 * @param s Storage object.
 */
Result appletHolderPushExtraStorage(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pushes a storage for LibraryApplet Interactive input.
 * @note  This uses \ref appletStorageClose automatically.
 * @param h AppletHolder object.
 * @param s Storage object.
 */
Result appletHolderPushInteractiveInData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pops a storage from LibraryApplet Interactive output.
 * @param h AppletHolder object.
 * @param s Storage object.
 */
Result appletHolderPopInteractiveOutData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Creates a storage.
 * @param s Storage object.
 * @param size Size of storage.
 */
Result appletCreateStorage(AppletStorage *s, s64 size);

/**
 * @brief Creates a TransferMemory storage.
 * @param s Storage object.
 * @param buffer TransferMemory buffer, will be automatically allocated if NULL.
 * @param size Size of storage.
 * @param writable Controls whether writing to the storage is allowed with \ref appletStorageWrite.
 */
Result appletCreateTransferMemoryStorage(AppletStorage *s, void* buffer, s64 size, bool writable);

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
 * @param buffer TransferMemory buffer, will be automatically allocated if NULL.
 * @param size Size of storage.
 */
Result appletCreateHandleStorageTmem(AppletStorage *s, void* buffer, s64 size);

/// Closes the storage object. TransferMemory closing is seperate, see \ref appletStorageCloseTmem.
/// Other applet functions which push an input storage will automatically call this.
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
 * @param s Storage object.
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
 * @brief Gets a notification message.
 */
Result appletGetMessage(u32 *msg);

/**
 * @brief Processes the current applet status using the specified msg.
 * @param msg Notification message, normally from \ref appletGetMessage.
 * @return Whether the application should continue running.
 */
bool appletProcessMessage(u32 msg);

/**
 * @brief Processes the current applet status. Generally used within a main loop.
 * @note Uses \ref appletGetMessage and \ref appletProcessMessage internally.
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
