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
#include "../services/pdm.h"
#include "../services/caps.h"
#include "../services/pm.h"
#include "../kernel/tmem.h"
#include "../kernel/event.h"

/// AppletType
typedef enum {
    AppletType_None = -2,
    AppletType_Default = -1,
    AppletType_Application = 0,
    AppletType_SystemApplet = 1,
    AppletType_LibraryApplet = 2,
    AppletType_OverlayApplet = 3,
    AppletType_SystemApplication = 4,
} AppletType;

/// OperationMode
typedef enum {
    AppletOperationMode_Handheld = 0,
    AppletOperationMode_Docked = 1,
} AppletOperationMode;

/// applet hook types.
typedef enum {
    AppletHookType_OnFocusState = 0,                    ///< ::AppletNotificationMessage_FocusStateChanged
    AppletHookType_OnOperationMode,                     ///< ::AppletNotificationMessage_OperationModeChanged
    AppletHookType_OnPerformanceMode,                   ///< ::AppletNotificationMessage_PerformanceModeChanged
    AppletHookType_OnExitRequest,                       ///< ::AppletNotificationMessage_ExitRequested
    AppletHookType_OnRestart,                           ///< ::AppletNotificationMessage_Restart
    AppletHookType_OnCaptureButtonShortPressed,         ///< ::AppletNotificationMessage_CaptureButtonShortPressed
    AppletHookType_OnAlbumImageTaken,                   ///< ::AppletNotificationMessage_AlbumImageTaken
    AppletHookType_RequestToDisplay,                    ///< ::AppletNotificationMessage_RequestToDisplay

    AppletHookType_Max,                                 ///< Number of applet hook types.
} AppletHookType;

/// NotificationMessage, for \ref appletGetMessage. See also \ref AppletHookType.
typedef enum {
    AppletNotificationMessage_ExitRequested             = 0x4,    ///< Exit requested.
    AppletNotificationMessage_FocusStateChanged         = 0xF,    ///< FocusState changed.
    AppletNotificationMessage_Restart                   = 0x10,   ///< Current applet execution was resumed.
    AppletNotificationMessage_OperationModeChanged      = 0x1E,   ///< OperationMode changed.
    AppletNotificationMessage_PerformanceModeChanged    = 0x1F,   ///< PerformanceMode changed.
    AppletNotificationMessage_RequestToDisplay          = 0x33,   ///< Display requested, see \ref appletApproveToDisplay.
    AppletNotificationMessage_CaptureButtonShortPressed = 0x5A,   ///< Capture button was short-pressed.
    AppletNotificationMessage_AlbumImageTaken           = 0x5C,   ///< Screenshot was taken.
} AppletNotificationMessage;

/// FocusState
typedef enum {
    AppletFocusState_Focused = 1,                   ///< Applet is focused.
    AppletFocusState_NotFocusedLibraryApplet = 2,   ///< Out of focus - LibraryApplet open.
    AppletFocusState_NotFocusedHomeSleep = 3        ///< Out of focus - HOME menu open / console is sleeping.
} AppletFocusState;

/// FocusHandlingMode
typedef enum {
  AppletFocusHandlingMode_SuspendHomeSleep = 0,       ///< Suspend only when HOME menu is open / console is sleeping (default).
  AppletFocusHandlingMode_NoSuspend,                  ///< Don't suspend when out of focus.
  AppletFocusHandlingMode_SuspendHomeSleepNotify,     ///< Suspend only when HOME menu is open / console is sleeping but still receive OnFocusState hook.
  AppletFocusHandlingMode_AlwaysSuspend,              ///< Always suspend when out of focus, regardless of the reason.

  AppletFocusHandlingMode_Max,                        ///< Number of focus handling modes.
} AppletFocusHandlingMode;

/// LaunchParameterKind
typedef enum {
    AppletLaunchParameterKind_Application     = 1, ///< Application-specific LaunchParameter
    AppletLaunchParameterKind_PreselectedUser = 2, ///< account PreselectedUser
    AppletLaunchParameterKind_Unknown         = 3, ///< Unknown if used by anything?
} AppletLaunchParameterKind;

/// AppletId
typedef enum {
    AppletId_application = 0x01,    ///<                  Application. Not valid for use with LibraryApplets.
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
    LibAppletMode_AllForeground                = 0,       ///< Foreground.
    LibAppletMode_Background                   = 1,       ///< Background.
    LibAppletMode_NoUi                         = 2,       ///< No UI.
    LibAppletMode_BackgroundIndirect           = 3,       ///< Background with indirect display, see \ref appletHolderGetIndirectLayerConsumerHandle.
    LibAppletMode_AllForegroundInitiallyHidden = 4,       ///< Foreground except initially hidden.
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

/// Mode values for \ref appletSetTvPowerStateMatchingMode.
typedef enum {
    AppletTvPowerStateMatchingMode_Unknown0 = 0,   ///< Unknown.
    AppletTvPowerStateMatchingMode_Unknown1 = 1,   ///< Unknown.
} AppletTvPowerStateMatchingMode;

/// Type values for \ref appletPerformSystemButtonPressingIfInFocus.
typedef enum {
    AppletSystemButtonType_HomeButtonShortPressing    = 1,   ///< Short-pressing with the HOME-button.
    AppletSystemButtonType_HomeButtonLongPressing     = 2,   ///< Long-pressing with the HOME-button.
    AppletSystemButtonType_CaptureButtonShortPressing = 6,   ///< Short-pressing with the Capture-button.
    AppletSystemButtonType_CaptureButtonLongPressing  = 7,   ///< Long-pressing with the Capture-button.
} AppletSystemButtonType;

/// Permission values for \ref appletSetScreenShotPermission.
typedef enum {
    AppletScreenShotPermission_Inherit = 0,        ///< Inherit from parent applet.
    AppletScreenShotPermission_Enable  = 1,        ///< Enable.
    AppletScreenShotPermission_Disable = 2,        ///< Disable.
} AppletScreenShotPermission;

/// Extension values for \ref appletSetIdleTimeDetectionExtension / \ref appletGetIdleTimeDetectionExtension, for extending user inactivity detection.
typedef enum {
    AppletIdleTimeDetectionExtension_None           = 0,       ///< No extension.
    AppletIdleTimeDetectionExtension_Extended       = 1,       ///< Extended
    AppletIdleTimeDetectionExtension_ExtendedUnsafe = 2,       ///< ExtendedUnsafe
} AppletIdleTimeDetectionExtension;

/// Input mode values for \ref appletSetWirelessPriorityMode.
typedef enum {
    AppletWirelessPriorityMode_Unknown1 = 1,       ///< Unknown.
    AppletWirelessPriorityMode_Unknown2 = 2,       ///< Unknown.
} AppletWirelessPriorityMode;

/// CaptureSharedBuffer for the IDisplayController commands.
typedef enum {
    AppletCaptureSharedBuffer_LastApplication = 0,                     ///< LastApplication
    AppletCaptureSharedBuffer_LastForeground  = 1,                     ///< LastForeground
    AppletCaptureSharedBuffer_CallerApplet    = 2,                     ///< CallerApplet
} AppletCaptureSharedBuffer;

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

/// LockAccessor
typedef struct {
    Service s;                         ///< ILockAccessor
    Event event;                       ///< Event from the GetEvent cmd, with autoclear=false.
} AppletLockAccessor;

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

/// LibraryAppletInfo
typedef struct {
    AppletId appletId;                 ///< \ref AppletId
    LibAppletMode mode;                ///< \ref LibAppletMode
} LibAppletInfo;

/// IdentityInfo
typedef struct {
    AppletId appletId;                 ///< \ref AppletId
    u32 pad;                           ///< Padding.
    u64 titleID;                       ///< titleID, only set with appletId == ::AppletId_application.
} AppletIdentityInfo;

/// Attributes for launching applications for Quest.
typedef struct {
    u32 unk_x0;                        ///< See AppletApplicationAttribute::unk_x0.
    u32 unk_x4;                        ///< See AppletApplicationAttribute::unk_x4.
    float volume;                      ///< [7.0.0+] See AppletApplicationAttribute::volume.
} AppletApplicationAttributeForQuest;

/// ApplicationAttribute
typedef struct {
    u32 unk_x0;                        ///< Default is 0 for non-Quest. Only used when non-zero: unknown value in seconds.
    u32 unk_x4;                        ///< Default is 0 for non-Quest. Only used when non-zero: unknown value in seconds.
    float volume;                      ///< Audio volume. Must be in the range of 0.0f-1.0f. The default is 1.0f.
    u8 unused[0x14];                   ///< Unused. Default is 0.
} AppletApplicationAttribute;

/// Initialize applet, called automatically during app startup.
Result appletInitialize(void);

/// Exit applet, called automatically during app exit.
void appletExit(void);

/// Get the cached AppletResourceUserId.
Result appletGetAppletResourceUserId(u64 *out);

/// Get the \ref AppletType.
AppletType appletGetAppletType(void);

/// Sets the state field for \ref AppletThemeColorType.
void appletSetThemeColorType(AppletThemeColorType theme);

/// Gets the state field for \ref AppletThemeColorType. Used internally by \ref libappletArgsCreate.
AppletThemeColorType appletGetThemeColorType(void);

// ICommonStateGetter

/**
 * @brief Gets the CradleStatus.
 * @param[out] status Output Dock status.
 */
Result appletGetCradleStatus(u8 *status);

/**
 * @brief Gets the BootMode which originated from \ref pmbmGetBootMode.
 * @param[out] mode \ref PmBootMode
 */
Result appletGetBootMode(PmBootMode *mode);

/**
 * @brief Request to AcquireSleepLock.
 * @note On success, this then uses cmd GetAcquiredSleepLockEvent and waits on that event.
 */
Result appletRequestToAcquireSleepLock(void);

/**
 * @brief Release the SleepLock.
 */
Result appletReleaseSleepLock(void);

/**
 * @brief Release the SleepLock transiently.
 * @note On success, this then uses cmd GetAcquiredSleepLockEvent and waits on that event.
 */
Result appletReleaseSleepLockTransiently(void);

/**
 * @brief Pushes a storage to the general channel. Used for sending requests to qlaunch.
 * @note  This is not usable under an Application, however it is usable under a LibraryApplet.
 * @note  This uses \ref appletStorageClose automatically.
 * @param s Storage object.
 */
Result appletPushToGeneralChannel(AppletStorage *s);

/**
 * @brief Gets a \ref AppletLockAccessor for HomeButtonReader.
 * @note Similar to using \ref appletGetReaderLockAccessorEx with inval=0.
 * @param a LockAccessor object.
 */
Result appletGetHomeButtonReaderLockAccessor(AppletLockAccessor *a);

/**
 * @brief Gets a Reader \ref AppletLockAccessor.
 * @note Only available with [2.0.0+].
 * @param a LockAccessor object.
 * @param[in] inval Input value, must be 0-3. 0 = HomeButton.
 */
Result appletGetReaderLockAccessorEx(AppletLockAccessor *a, u32 inval);

/**
 * @brief Gets a Writer \ref AppletLockAccessor.
 * @note Only available with [7.0.0+]. On older sysvers, this is only available with AppletType_SystemApplet on [2.0.0+].
 * @param a LockAccessor object.
 * @param[in] inval Input value, must be 0-3. 0 = HomeButton.
 */
Result appletGetWriterLockAccessorEx(AppletLockAccessor *a, u32 inval);

/**
 * @brief Gets the Dock firmware version.
 * @note Only available with [2.0.0+].
 * @param[out] out0 First output value.
 * @param[out] out1 Second output value.
 * @param[out] out2 Third output value.
 * @param[out] out3 Fourth output value.
 */
Result appletGetCradleFwVersion(u32 *out0, u32 *out1, u32 *out2, u32 *out3);

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
 * @brief Sets whether the LCD screen backlight is turned off.
 * @note Only available with [4.0.0+].
 * @param[in] flag Flag
 */
Result appletSetLcdBacklightOffEnabled(bool flag);

/**
 * @brief Gets the ControllerFirmwareUpdateSection flag.
 * @note Only available with [3.0.0+].
 * @param[out] out Output flag.
 */
Result appletIsInControllerFirmwareUpdateSection(bool *out);

/**
 * @brief Gets the DefaultDisplayResolution.
 * @note Only available with [3.0.0+].
 * @param[out] width Output width.
 * @param[out] height Output height.
 */
Result appletGetDefaultDisplayResolution(s32 *width, s32 *height);

/**
 * @brief Gets an Event which is signaled when the output from \ref appletGetDefaultDisplayResolution changes.
 * @note Only available with [3.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result appletGetDefaultDisplayResolutionChangeEvent(Event *out_event);

/**
 * @brief Gets the HdcpAuthenticationState.
 * @note Only available with [4.0.0+].
 * @param[out] state Output state.
 */
Result appletGetHdcpAuthenticationState(s32 *state);

/**
 * @brief Gets an Event which is signaled when the output from \ref appletGetHdcpAuthenticationState changes.
 * @note Only available with [4.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result appletGetHdcpAuthenticationStateChangeEvent(Event *out_event);

/**
 * @brief Sets the \ref AppletTvPowerStateMatchingMode.
 * @note Only available with [5.0.0+].
 * @param[in] mode \ref AppletTvPowerStateMatchingMode
 */
Result appletSetTvPowerStateMatchingMode(AppletTvPowerStateMatchingMode mode);

/**
 * @brief Gets the application titleID for the specified ContentActionName string.
 * @note Only available when the current applet is ::AppletId_starter on [5.1.0+].
 * @param[out] titleID Application titleID.
 * @param[in] name ContentActionName string.
 */
Result appletGetApplicationIdByContentActionName(u64 *titleID, const char *name);

/**
 * @brief Sets the \ref ApmCpuBoostMode.
 * @note Only available with [7.0.0+] (not fully usable system-side with 6.x).
 * @param mode \ref ApmCpuBoostMode.
 */
Result appletSetCpuBoostMode(ApmCpuBoostMode mode);

/**
 * @brief Perform SystemButtonPressing with the specified \ref AppletSystemButtonType.
 * @note Only available with [6.0.0+].
 * @param[in] type \ref AppletSystemButtonType
 */
Result appletPerformSystemButtonPressingIfInFocus(AppletSystemButtonType type);

/**
 * @brief Sets whether PerformanceConfigurationChangedNotification is enabled.
 * @note Only available with [7.0.0+].
 * @param[in] flag Whether to enable the notification.
 */
Result appletSetPerformanceConfigurationChangedNotification(bool flag);

/**
 * @brief Gets the current PerformanceConfiguration.
 * @note Only available with [7.0.0+].
 * @param PerformanceConfiguration Output PerformanceConfiguration.
 */
Result appletGetCurrentPerformanceConfiguration(u32 *PerformanceConfiguration);

/**
 * @brief Gets the OperationModeSystemInfo.
 * @note Only available with [7.0.0+].
 * @param[out] info Output info.
 */
Result appletGetOperationModeSystemInfo(u32 *info);

// ISelfController

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
 * @brief Enter FatalSection.
 */
Result appletEnterFatalSection(void);

/**
 * @brief Leave FatalSection.
 */
Result appletLeaveFatalSection(void);

/**
 * @brief Controls whether screenshot-capture is allowed.
 * @param permission \ref AppletScreenShotPermission
 */
Result appletSetScreenShotPermission(AppletScreenShotPermission permission);

/**
 * @brief Sets whether ::AppletNotificationMessage_Restart is enabled.
 * @param[in] flag Whether to enable the notification.
 */
Result appletSetRestartMessageEnabled(bool flag);

/**
 * @brief Sets the \ref AppletIdentityInfo for screenshots.
 * @param[in] info \ref AppletIdentityInfo
 */
Result appletSetScreenShotAppletIdentityInfo(AppletIdentityInfo *info);

/**
 * @brief Sets ControllerFirmwareUpdateSection.
 * @note Only available with [3.0.0+].
 * @note This throws error 0x40280 when the internal state flag already matches the input value.
 * @param[in] flag Flag
 */
Result appletSetControllerFirmwareUpdateSection(bool flag);

/**
 * @brief Sets whether ::AppletNotificationMessage_CaptureButtonShortPressed is enabled.
 * @note Only available with [3.0.0+].
 * @note When enabled with a non-Overlay applet, Overlay applet will not be notified of capture button short-presses for screenshots.
 * @param[in] flag Whether to enable the notification.
 */
Result appletSetRequiresCaptureButtonShortPressedMessage(bool flag);

/**
 * @brief Sets the Album screenshot ImageOrientation.
 * @note Only available with [3.0.0+].
 * @param[in] orientation \ref AlbumImageOrientation
 */
Result appletSetAlbumImageOrientation(AlbumImageOrientation orientation);

/**
 * @brief Sets the DesirableKeyboardLayout.
 * @note Only available with [4.0.0+].
 * @param[in] layout Input layout.
 */
Result appletSetDesirableKeyboardLayout(u32 layout);

Result appletCreateManagedDisplayLayer(u64 *out);

/**
 * @brief Checks whether SystemBufferSharing is enabled, throwing an error otherwise.
 * @note Only available with [4.0.0+]. Not usable with AppletType_*Application.
 */
Result appletIsSystemBufferSharingEnabled(void);

/**
 * @brief Gets the System SharedBufferHandle and SharedLayerHandle.
 * @note Only available with [4.0.0+]. Not usable with AppletType_*Application.
 * @param[out] SharedBufferHandle Output System SharedBufferHandle.
 * @param[out] SharedLayerHandle Output System SharedLayerHandle.
 */
Result appletGetSystemSharedLayerHandle(u64 *SharedBufferHandle, u64 *SharedLayerHandle);

/**
 * @brief Same as \ref appletGetSystemSharedLayerHandle except this just gets the SharedBufferHandle.
 * @note Only available with [5.0.0+]. Not usable with AppletType_*Application.
 * @param[out] SharedBufferHandle Output System SharedBufferHandle.
 */
Result appletGetSystemSharedBufferHandle(u64 *SharedBufferHandle);

/**
 * @brief Sets whether ::AppletNotificationMessage_RequestToDisplay is enabled.
 * @note Sets an internal state flag. When the input flag is 0, this will in additional run the same code as \ref appletApproveToDisplay.
 * @param[in] flag Flag
 */
Result appletSetHandlesRequestToDisplay(bool flag);

/**
 * @brief Approve the display requested by ::AppletNotificationMessage_RequestToDisplay, see also \ref appletSetHandlesRequestToDisplay.
 */
Result appletApproveToDisplay(void);

/**
 * @brief OverrideAutoSleepTimeAndDimmingTime
 * @param[in] inval0 Unknown input value.
 * @param[in] inval1 Unknown input value.
 * @param[in] inval2 Unknown input value.
 * @param[in] inval3 Unknown input value.
 */
Result appletOverrideAutoSleepTimeAndDimmingTime(s32 inval0, s32 inval1, s32 inval2, s32 inval3);

/**
 * @brief Sets the IdleTimeDetectionExtension.
 * @param[in] ext \ref AppletIdleTimeDetectionExtension Must be 0-2: 0 = disabled, 1 = Extended, and 2 = ExtendedUnsafe.
 */
Result appletSetIdleTimeDetectionExtension(AppletIdleTimeDetectionExtension ext);

/**
 * @brief Gets the value set by \ref appletSetIdleTimeDetectionExtension.
 * @param[out] ext \ref AppletIdleTimeDetectionExtension
 */
Result appletGetIdleTimeDetectionExtension(AppletIdleTimeDetectionExtension *ext);

/**
 * @brief Sets the InputDetectionSourceSet.
 * @param[in] val Input value.
 */
Result appletSetInputDetectionSourceSet(u32 val);

/**
 * @brief Reports that the user is active, for idle detection (screen dimming / auto-sleep). This is equivalent to when the user uses HID input.
 * @note Only available with [2.0.0+].
 */
Result appletReportUserIsActive(void);

/**
 * @brief Gets the current Illuminance from the light sensor.
 * @note Only available with [3.0.0+].
 * @param fLux Output fLux
 */
Result appletGetCurrentIlluminance(float *fLux);

/**
 * @brief Gets whether Illuminance is available.
 * @note Only available with [3.0.0+].
 * @param out Output flag
 */
Result appletIsIlluminanceAvailable(bool *out);

/**
 * @brief Sets AutoSleepDisabled.
 * @note Only available with [5.0.0+].
 * @param[in] flag Flag
 */
Result appletSetAutoSleepDisabled(bool flag);

/**
 * @brief Gets AutoSleepDisabled.
 * @note Only available with [5.0.0+].
 * @param[out] out Output flag
 */
Result appletIsAutoSleepDisabled(bool *out);

/**
 * @brief Gets the current Illuminance from the light sensor. Same as \ref appletGetCurrentIlluminance except for the additional param.
 * @note Only available with [5.0.0+].
 * @param bOverLimit Output bOverLimit
 * @param fLux Output fLux
 */
Result appletGetCurrentIlluminanceEx(bool *bOverLimit, float *fLux);

/**
 * @brief Sets the WirelessPriorityMode.
 * @note Only available with [4.0.0+].
 * @param[in] mode \ref AppletWirelessPriorityMode
 */
Result appletSetWirelessPriorityMode(AppletWirelessPriorityMode mode);

/**
 * @brief Sets whether ::AppletNotificationMessage_AlbumImageTaken is enabled.
 * @note Only available with [7.0.0+].
 * @param[in] flag Whether to enable the notification.
 */
Result appletSetAlbumImageTakenNotificationEnabled(bool flag);

/**
 * @brief Gets the total time in nanoseconds that the current process was actively running (not suspended), relative to when \ref appletInitialize was last used.
 * @note Only available with [6.0.0+].
 * @param[out] activeTime Output nanoseconds value.
 */
Result appletGetProgramTotalActiveTime(u64 *activeTime);

/**
 * @brief Sets the Application AlbumUserData.
 * @note Only available with [8.0.0+].
 * @param[in] buffer Buffer containing arbitrary UserData.
 * @param[in] size Buffer size, must be <=0x400.
 */
Result appletSetApplicationAlbumUserData(const void* buffer, size_t size);

// IWindowController

/**
 * @brief Gets the AppletResourceUserId of the CallerApplet.
 * @note Only available with [6.0.0+].
 * @param[out] out AppletResourceUserId
 */
Result appletGetAppletResourceUserIdOfCallerApplet(u64 *out);

/**
 * @brief Sets the current applet WindowVisibility.
 * @note Only available with [7.0.0+].
 * @param[in] flag Flag
 */
Result appletSetAppletWindowVisibility(bool flag);

/**
 * @brief Sets the AppletGpuTimeSlice.
 * @note Only available with [7.0.0+].
 * @param[in] val Input value, must not be negative.
 */
Result appletSetAppletGpuTimeSlice(s64 val);

// IAudioController

/**
 * @brief Sets the ExpectedMasterVolume for MainApplet and LibraryApplet.
 * @note Used by some official apps before/after launching LibraryApplets. Prior to changing the volume, the official app uses \ref appletGetExpectedMasterVolume, with the output being used to restore the volume after LibraryApplet handling.
 * @param[in] mainAppletVolume MainApplet ExpectedMasterVolume.
 * @param[in] libraryAppletVolume LibraryApplet ExpectedMasterVolume.
 */
Result appletSetExpectedMasterVolume(float mainAppletVolume, float libraryAppletVolume);

/**
 * @brief Gets the ExpectedMasterVolume for MainApplet and LibraryApplet.
 * @note See also \ref appletSetExpectedMasterVolume.
 * @param[out] mainAppletVolume MainApplet ExpectedMasterVolume. Optional, can be NULL. Used with cmd GetMainAppletExpectedMasterVolume when not NULL.
 * @param[out] libraryAppletVolume LibraryApplet ExpectedMasterVolume. Optional, can be NULL. Used with cmd GetLibraryAppletExpectedMasterVolume when not NULL.
 */
Result appletGetExpectedMasterVolume(float *mainAppletVolume, float *libraryAppletVolume);

/**
 * @brief Change the MainApplet MasterVolume.
 * @param[in] volume MainApplet MasterVolume.
 * @param[in] unk Unknown.
 */
Result appletChangeMainAppletMasterVolume(float volume, u64 unk);

/**
 * @brief Sets the TransparentVolumeRate.
 * @param[in] val Input value.
 */
Result appletSetTransparentVolumeRate(float val);

// IDisplayController

/**
 * @brief Update the LastForeground CaptureImage.
 */
Result appletUpdateLastForegroundCaptureImage(void);

/**
 * @brief Update the CallerApplet CaptureImage.
 */
Result appletUpdateCallerAppletCaptureImage(void);

/**
 * @brief Gets the LastForeground CaptureImage.
 * @param[out] buffer Output buffer containing the 1280x720 RGBA8 image.
 * @param[out] size Buffer size, must match 0x384000.
 * @param[out] flag Output flag.
 */
Result appletGetLastForegroundCaptureImageEx(void* buffer, size_t size, bool *flag);

/**
 * @brief Gets the LastApplication CaptureImage.
 * @param[out] buffer Output buffer containing the 1280x720 RGBA8 image.
 * @param[out] size Buffer size, must match 0x384000.
 * @param[out] flag Output flag.
 */
Result appletGetLastApplicationCaptureImageEx(void* buffer, size_t size, bool *flag);

/**
 * @brief Gets the CallerApplet CaptureImage.
 * @param[out] buffer Output buffer containing the 1280x720 RGBA8 image.
 * @param[out] size Buffer size, must match 0x384000.
 * @param[out] flag Output flag.
 */
Result appletGetCallerAppletCaptureImageEx(void* buffer, size_t size, bool *flag);

/**
 * @brief Takes a screenshot of the current applet Layer into the specified CaptureSharedBuffer.
 * @note Only available with [2.0.0+].
 * @param[in] flag Flag.
 * @param[in] captureBuf \ref AppletCaptureSharedBuffer
 */
Result appletTakeScreenShotOfOwnLayer(bool flag, AppletCaptureSharedBuffer captureBuf);

/**
 * @brief Copies image data from a CaptureSharedBuffer to another CaptureSharedBuffer.
 * @note Only available with [5.0.0+].
 * @param[in] dstCaptureBuf Destination \ref AppletCaptureSharedBuffer.
 * @param[in] srcCaptureBuf Source \ref AppletCaptureSharedBuffer.
 */
Result appletCopyBetweenCaptureBuffers(AppletCaptureSharedBuffer dstCaptureBuf, AppletCaptureSharedBuffer srcCaptureBuf);

/**
 * @brief Clear the input CaptureSharedBuffer with the specified color.
 * @note Only available with [3.0.0+].
 * @param[in] flag Flag.
 * @param[in] captureBuf \ref AppletCaptureSharedBuffer
 * @param[in] color RGBA8 color.
 */
Result appletClearCaptureBuffer(bool flag, AppletCaptureSharedBuffer captureBuf, u32 color);

/**
 * @brief Clear the AppletTransitionBuffer with the specified color.
 * @note Only available with [3.0.0+].
 * @param[in] color RGBA8 color.
 */
Result appletClearAppletTransitionBuffer(u32 color);

/**
 * @brief Acquire the LastApplication CaptureSharedBuffer.
 * @note Only available with [4.0.0+].
 * @param[out] flag Output flag.
 * @param[out] id Output ID.
 */
Result appletAcquireLastApplicationCaptureSharedBuffer(bool *flag, s32 *id);

/**
 * @brief Release the LastApplication CaptureSharedBuffer.
 * @note Only available with [4.0.0+].
 */
Result appletReleaseLastApplicationCaptureSharedBuffer(void);

/**
 * @brief Acquire the LastForeground CaptureSharedBuffer.
 * @note Only available with [4.0.0+].
 * @param[out] flag Output flag.
 * @param[out] id Output ID.
 */
Result appletAcquireLastForegroundCaptureSharedBuffer(bool *flag, s32 *id);

/**
 * @brief Release the LastForeground CaptureSharedBuffer.
 * @note Only available with [4.0.0+].
 */
Result appletReleaseLastForegroundCaptureSharedBuffer(void);

/**
 * @brief Acquire the CallerApplet CaptureSharedBuffer.
 * @note Only available with [4.0.0+].
 * @param[out] flag Output flag.
 * @param[out] id Output ID.
 */
Result appletAcquireCallerAppletCaptureSharedBuffer(bool *flag, s32 *id);

/**
 * @brief Release the CallerApplet CaptureSharedBuffer.
 * @note Only available with [4.0.0+].
 */
Result appletReleaseCallerAppletCaptureSharedBuffer(void);

/**
 * @brief Takes a screenshot of the current applet Layer into the specified CaptureSharedBuffer. Same as \ref appletTakeScreenShotOfOwnLayer except for the additional immediately param.
 * @note Only available with [6.0.0+].
 * @param[in] flag0 Flag0.
 * @param[in] immediately Whether the screenshot should be taken immediately.
 * @param[in] captureBuf \ref AppletCaptureSharedBuffer
 */
Result appletTakeScreenShotOfOwnLayerEx(bool flag0, bool immediately, AppletCaptureSharedBuffer captureBuf);

// LockAccessor

/**
 * @brief Closes a LockAccessor.
 * @param a LockAccessor object.
 */
void appletLockAccessorClose(AppletLockAccessor *a);

/**
 * @brief TryLock a LockAccessor.
 * @param a LockAccessor object.
 * @param[out] flag Whether locking was successful, when false this indicates that this func should be called again.
 */
Result appletLockAccessorTryLock(AppletLockAccessor *a, bool *flag);

/**
 * @brief Lock a LockAccessor.
 * @note Similar to \ref appletLockAccessorTryLock, except this uses timeout U64_MAX with the eventWait call, and this uses TryLock repeatedly until the output flag value is true.
 * @param a LockAccessor object.
 */
Result appletLockAccessorLock(AppletLockAccessor *a);

/**
 * @brief Unlock a LockAccessor.
 * @param a LockAccessor object.
 */
Result appletLockAccessorUnlock(AppletLockAccessor *a);

// ILibraryAppletCreator

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
 * @note  Only available when \ref LibAppletMode is ::LibAppletMode_BackgroundIndirect.
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
 * @brief Sets OutOfFocusApplicationSuspendingEnabled.
 * @note Only available with AppletType_*Application.
 * @param h AppletHolder object.
 * @param[in] flag Flag
 */
Result appletHolderSetOutOfFocusApplicationSuspendingEnabled(AppletHolder *h, bool flag);

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
 * @brief Gets the \ref LibAppletInfo for the specified LibraryApplet.
 * @param h AppletHolder object.
 * @param[out] info \ref LibAppletInfo
 */
Result appletHolderGetLibraryAppletInfo(AppletHolder *h, LibAppletInfo *info);

// (ILibraryAppletCreator ->) IStorage

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

// IFunctions for AppletType_*Application (IApplicationFunctions).

/**
 * @brief Pops a LaunchParameter AppletStorage, the storage will be removed from sysmodule state during this.
 * @param s Output storage.
 * @param kind See \ref AppletLaunchParameterKind.
 * @note Only available with AppletType_*Application.
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
 * @note Identical to \ref appletRequestLaunchApplication, except this allows the user to specify the attribute fields instead of the defaults being used.
 * @param[in] titleID Application titleID
 * @param s Optional AppletStorage object, can be NULL. This is automatically closed. When NULL on pre-4.0.0, this will internally create a tmp storage with size 0 for use with the cmd. This is the storage available to the launched application via \ref appletPopLaunchParameter with ::AppletLaunchParameterKind_Application.
 * @param[in] attr Kiosk application attributes.
 */
Result appletRequestLaunchApplicationForQuest(u64 titleID, AppletStorage* s, const AppletApplicationAttributeForQuest *attr);

Result appletGetDesiredLanguage(u64 *LanguageCode);

/// Only available with AppletType_*Application.
Result appletSetTerminateResult(Result res);

/**
 * @brief Gets the DisplayVersion for the current host title control.nacp.
 * @note Only available with AppletType_*Application.
 * @param[out] displayVersion Output DisplayVersion string, must be at least 0x10-bytes. This is always NUL-terminated.
 */
Result appletGetDisplayVersion(char *displayVersion);

/**
 * @brief Blocks the usage of the home button, for short (Home Menu) and long (Overlay) presses.
 * @note Only available with AppletType_*Application.
 * @param val Unknown. Official sw only uses hard-coded value 0 for this.
 */
Result appletBeginBlockingHomeButtonShortAndLongPressed(s64 val);

/**
 * @brief Ends the blocking started by \ref appletBeginBlockingHomeButtonShortAndLongPressed.
 * @note Only available with AppletType_*Application.
 */
Result appletEndBlockingHomeButtonShortAndLongPressed(void);

/**
 * @brief Blocks the usage of the home button, for short presses (Home Menu).
 * @note Only available with AppletType_*Application.
 * @param val Unknown nanoseconds. Value 0 can be used.
 */
Result appletBeginBlockingHomeButton(s64 val);

/**
 * @brief Ends the blocking started by \ref appletBeginBlockingHomeButton.
 * @note Only available with AppletType_*Application.
 */
Result appletEndBlockingHomeButton(void);

/**
 * @brief Notify that the app is now running, for the Application logo screen. This throws a fatal-error on failure.
 * @note This will just return when applet-type isn't AppletType_Application, or when this was already used previously. Used automatically by \ref appletInitialize when __nx_applet_auto_notifyrunning is set to true (the default value).
 */
void appletNotifyRunning(bool *out);

/**
 * @brief Gets the PseudoDeviceId. This is derived from the output of a ns command, and from data in the host title control.nacp.
 * @note Only available with AppletType_*Application on 2.0.0+.
 * @param[out] out Output PseudoDeviceId.
 */
Result appletGetPseudoDeviceId(u128 *out);

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
/// Only usable when running under a title which supports video recording. Using this is only needed when the host title control.nacp has VideoCaptureMode set to Enabled, with Automatic appletInitializeGamePlayRecording is not needed.
Result appletInitializeGamePlayRecording(void);

/**
 * @brief Requests a system shutdown.
 * @note Only available with AppletType_*Application on 3.0.0+.
 */
Result appletRequestToShutdown(void);

/**
 * @brief Requests a system reboot.
 * @note Only available with AppletType_*Application on 3.0.0+.
 */
Result appletRequestToReboot(void);

/**
 * @brief Initializes the ApplicationCopyrightFrameBuffer, with dimensions 1280x720 + the tmem for it. This is used as an overlay for screenshots.
 * @note Only available with AppletType_*Application on 5.0.0+.
 * @note Cleanup for this is handled automatically during app exit in \ref appletExit.
 */
Result appletInitializeApplicationCopyrightFrameBuffer(void);

/**
 * @brief Sets the RGBA8 image for use with \ref appletInitializeApplicationCopyrightFrameBuffer. Overrides the current image, if this was already used previously.
 * @note Only available with AppletType_*Application on 5.0.0+.
 * @note The specified coordinates and width/height must be within the bounds of the framebuffer setup by \ref appletInitializeApplicationCopyrightFrameBuffer.
 * @param[in] buffer Input image buffer.
 * @param[in] size Input image buffer size.
 * @param[in] x X coordinate. Must not be negative.
 * @param[in] y Y coordinate. Must not be negative.
 * @param[in] width Image width. Must be >=1.
 * @param[in] height Image height. Must be >=1.
 * @param[in] mode WindowOriginMode. Should be at least 1.
 */
Result appletSetApplicationCopyrightImage(const void* buffer, size_t size, s32 x, s32 y, s32 width, s32 height, s32 mode);

/**
 * @brief Sets the visibility for the image set by \ref appletSetApplicationCopyrightImage, in screenshots.
 * @note Only available with AppletType_*Application on 5.0.0+.
 * @param[in] visible Whether the image is visible. The default is true.
 */
Result appletSetApplicationCopyrightVisibility(bool visible);

/**
 * @brief Gets ApplicationPlayStatistics.
 * @note Only available with AppletType_*Application on 5.0.0+.
 * @note The input titleIDs must be allowed via control.nacp with the current host title. The minimum allowed titleID is the titleID for the current-process.
 * @param stats Output \ref PdmApplicationPlayStatistics array.
 * @param titleIDs Input titleIDs array.
 * @param count Total entries in the input/output arrays.
 * @param total_out Total output entries.
 */
Result appletQueryApplicationPlayStatistics(PdmApplicationPlayStatistics *stats, const u64 *titleIDs, s32 count, s32 *total_out);

/**
 * @brief Same as \ref appletQueryApplicationPlayStatistics except this gets playstats specific to the input userID.
 * @note Only available with AppletType_*Application on 6.0.0+.
 * @param userID userID
 * @param stats Output \ref PdmApplicationPlayStatistics array.
 * @param titleIDs Input titleIDs array.
 * @param count Total entries in the input/output arrays.
 * @param total_out Total output entries.
 */
Result appletQueryApplicationPlayStatisticsByUid(u128 userID, PdmApplicationPlayStatistics *stats, const u64 *titleIDs, s32 count, s32 *total_out);

/**
 * @brief Gets an Event which is signaled for GpuErrorDetected.
 * @note Only available with AppletType_*Application on [8.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @note Official sw waits on this Event from a seperate thread, triggering an abort when it's signaled.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetGpuErrorDetectedSystemEvent(Event *out_event);

// IHomeMenuFunctions

/**
 * @brief RequestToGetForeground
 * @note Only available with AppletType_SystemApplet.
 */
Result appletRequestToGetForeground(void);

/**
 * @brief LockForeground
 * @note Only available with AppletType_SystemApplet.
 */
Result appletLockForeground(void);

/**
 * @brief UnlockForeground
 * @note Only available with AppletType_SystemApplet.
 */
Result appletUnlockForeground(void);

/**
 * @brief Pops a storage from the general channel.
 * @note Only available with AppletType_SystemApplet.
 * @param[out] s Storage object.
 */
Result appletPopFromGeneralChannel(AppletStorage *s);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletPopFromGeneralChannel.
 * @note Only available with AppletType_SystemApplet.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetPopFromGeneralChannelEvent(Event *out_event);

/**
 * @brief Gets a \ref AppletLockAccessor for HomeButtonWriter.
 * @note Only available with AppletType_SystemApplet.
 * @note Similar to using \ref appletGetWriterLockAccessorEx with inval=0.
 * @param a LockAccessor object.
 */
Result appletGetHomeButtonWriterLockAccessor(AppletLockAccessor *a);

// IGlobalStateController

/**
 * @brief Start the sequence for entering sleep-mode.
 * @note Only available with AppletType_SystemApplet.
 * @param[in] flag Flag, official sw uses hard-coded value = true.
 */
Result appletStartSleepSequence(bool flag);

/**
 * @brief Start the system-shutdown sequence.
 * @note Only available with AppletType_SystemApplet.
 */
Result appletStartShutdownSequence(void);

/**
 * @brief Start the system-reboot sequence.
 * @note Only available with AppletType_SystemApplet.
 */
Result appletStartRebootSequence(void);

/**
 * @brief IsAutoPowerDownRequested. Uses an idle:sys cmd internally.
 * @note Only available with AppletType_SystemApplet on [7.0.0+].
 * @param[out] out Output flag.
 */
Result appletIsAutoPowerDownRequested(bool *out);

/**
 * @brief LoadAndApplyIdlePolicySettings. Uses an idle:sys cmd internally.
 * @note Only available with AppletType_SystemApplet.
 */
Result appletLoadAndApplyIdlePolicySettings(void);

/**
 * @brief NotifyCecSettingsChanged. Uses an omm cmd internally.
 * @note Only available with AppletType_SystemApplet on [2.0.0+].
 */
Result appletNotifyCecSettingsChanged(void);

/**
 * @brief Sets the DefaultHomeButtonLongPressTime.
 * @note Only available with AppletType_SystemApplet on [3.0.0+].
 * @param[in] val Input value.
 */
Result appletSetDefaultHomeButtonLongPressTime(s64 val);

/**
 * @brief UpdateDefaultDisplayResolution. Uses an omm cmd internally.
 * @note Only available with AppletType_SystemApplet on [3.0.0+].
 */
Result appletUpdateDefaultDisplayResolution(void);

/**
 * @brief ShouldSleepOnBoot. Uses an omm cmd internally.
 * @note Only available with AppletType_SystemApplet on [3.0.0+].
 * @param[out] out Output flag.
 */
Result appletShouldSleepOnBoot(bool *out);

/**
 * @brief Gets an Event which is signaled for HdcpAuthenticationFailed.
 * @note Only available with AppletType_SystemApplet on [4.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetHdcpAuthenticationFailedEvent(Event *out_event);

// ILibraryAppletSelfAccessor

/**
 * @brief Gets the \ref LibAppletInfo for the current LibraryApplet.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] info \ref LibAppletInfo
 */
Result appletGetLibraryAppletInfo(LibAppletInfo *info);

/**
 * @brief Gets the \ref AppletIdentityInfo for the MainApplet.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] info \ref AppletIdentityInfo
 */
Result appletGetMainAppletIdentityInfo(AppletIdentityInfo *info);

/**
 * @brief Gets the \ref AppletIdentityInfo for the CallerApplet.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] info \ref AppletIdentityInfo
 */
Result appletGetCallerAppletIdentityInfo(AppletIdentityInfo *info);

/**
 * @brief Gets an array of \ref AppletIdentityInfo for the CallerStack.
 * @note Only available with AppletType_LibraryApplet on [3.0.0+].
 * @param[out] stack Output array of \ref AppletIdentityInfo.
 * @param[in] count Size of the stack array.
 * @param[out] total_out Total output entries.
 */
Result appletGetCallerAppletIdentityInfoStack(AppletIdentityInfo *stack, s32 count, s32 *total_out);

/**
 * @brief Gets the \ref AppletIdentityInfo for the NextReturnDestinationApplet.
 * @note Only available with AppletType_LibraryApplet on [4.0.0+].
 * @param[out] info \ref AppletIdentityInfo
 */
Result appletGetNextReturnDestinationAppletIdentityInfo(AppletIdentityInfo *info);

// IFunctions for AppletType_OverlayApplet (IOverlayFunctions).

/**
 * @brief Stops forwarding the input to the foreground app.
 * @note Only available with AppletType_OverlayApplet.
 * @note You have to call this to receive inputs through the hid service when running as the overlay applet.
 */
Result appletBeginToWatchShortHomeButtonMessage(void);

/**
 * @brief Forwards input to the foreground app.
 * @note Only available with AppletType_OverlayApplet.
 * @note After calling this the overlay applet won't receive any input until \ref appletBeginToWatchShortHomeButtonMessage is called again.
 */
Result appletEndToWatchShortHomeButtonMessage(void);

/**
 * @brief Gets the application titleID for displaying the logo screen during application launch.
 * @note Only available with AppletType_OverlayApplet.
 * @param[out] titleID Output application titleID, 0 when no application is running.
 */
Result appletGetApplicationIdForLogo(u64 *titleID);

/**
 * @brief Sets the GpuTimeSliceBoost.
 * @note Only available with AppletType_OverlayApplet.
 * @param[in] val Input value.
 */
Result appletSetGpuTimeSliceBoost(u64 val);

/**
 * @brief Sets AutoSleepTimeAndDimmingTimeEnabled.
 * @note Only available with AppletType_OverlayApplet on [2.0.0+].
 * @param[in] flag Flag
 */
Result appletSetAutoSleepTimeAndDimmingTimeEnabled(bool flag);

/**
 * @brief TerminateApplicationAndSetReason
 * @note Only available with AppletType_OverlayApplet on [2.0.0+].
 * @param[in] reason Result reason.
 */
Result appletTerminateApplicationAndSetReason(Result reason);

/**
 * @brief Sets ScreenShotPermissionGlobally.
 * @note Only available with AppletType_OverlayApplet on [3.0.0+].
 * @param[in] flag Flag
 */
Result appletSetScreenShotPermissionGlobally(bool flag);

/**
 * @brief Start the system-shutdown sequence.
 * @note Only available with AppletType_SystemApplet on [6.0.0+].
 */
Result appletStartShutdownSequenceForOverlay(void);

/**
 * @brief Start the system-reboot sequence.
 * @note Only available with AppletType_SystemApplet on [6.0.0+].
 */
Result appletStartRebootSequenceForOverlay(void);

/**
 * @brief Sets HandlingHomeButtonShortPressedEnabled.
 * @note Only available with AppletType_OverlayApplet on [8.0.0+].
 * @param[in] flag Flag
 */
Result appletSetHandlingHomeButtonShortPressedEnabled(bool flag);

// IAppletCommonFunctions

/**
 * @brief Reads the ThemeStorage for the current applet.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [7.0.0+].
 * @note offset(+size) must be <=0x400.
 * @param[out] buffer Output buffer data.
 * @param[in] size Size to read.
 * @param[in] offset Offset within the ThemeStorage.
 * @param[out] transfer_size Actual read size.
 */
Result appletReadThemeStorage(void* buffer, size_t size, u64 offset, size_t *transfer_size);

/**
 * @brief Writes the ThemeStorage for the current applet.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [7.0.0+].
 * @note offset(+size) must be <=0x400.
 * @param[in] buffer Input buffer data.
 * @param[in] size Size to write.
 * @param[in] offset Offset within the ThemeStorage.
 */
Result appletWriteThemeStorage(const void* buffer, size_t size, u64 offset);

/**
 * @brief Gets the DisplayLogicalResolution.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [8.0.0+].
 * @param[out] width Output width.
 * @param[out] height Output height.
 */
Result appletGetDisplayLogicalResolution(s32 *width, s32 *height);

/**
 * @brief Sets the DisplayMagnification. This is essentially layer image crop, for everything non-Overlay.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [8.0.0+].
 * @note x and width are multiplied with the same width value returned by \ref appletGetDisplayLogicalResolution, so these should be in the range 0.0f-1.0f. Likewise for y and height, except these are multipled with the height value.
 * @param[in] x X position.
 * @param[in] y Y position.
 * @param[in] width Width.
 * @param[in] height Height.
 */
Result appletSetDisplayMagnification(float x, float y, float width, float height);

/**
 * @brief Sets whether HomeButtonDoubleClick is enabled.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [8.0.0+].
 * @param[in] flag Flag
 */
Result appletSetHomeButtonDoubleClickEnabled(bool flag);

/**
 * @brief Gets whether HomeButtonDoubleClick is enabled.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [8.0.0+].
 * @param[out] out Output flag.
 */
Result appletGetHomeButtonDoubleClickEnabled(bool *out);

// State / other

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
