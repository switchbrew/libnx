/**
 * @file applet.h
 * @brief Applet (applet) service IPC wrapper.
 * @note For wrappers which launch LibraryApplets etc, see switch/applets/.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/apm.h"
#include "../services/pdm.h"
#include "../services/caps.h"
#include "../services/pm.h"
#include "../services/ncm_types.h"
#include "../services/acc.h"
#include "../services/set.h"
#include "../kernel/tmem.h"
#include "../kernel/event.h"
#include "../nacp.h"

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
    AppletOperationMode_Handheld = 0,                  ///< Handheld
    AppletOperationMode_Console  = 1,                  ///< Console (Docked / TV-mode)
} AppletOperationMode;

/// applet hook types.
typedef enum {
    AppletHookType_OnFocusState = 0,                    ///< ::AppletMessage_FocusStateChanged
    AppletHookType_OnOperationMode,                     ///< ::AppletMessage_OperationModeChanged
    AppletHookType_OnPerformanceMode,                   ///< ::AppletMessage_PerformanceModeChanged
    AppletHookType_OnExitRequest,                       ///< ::AppletMessage_ExitRequested
    AppletHookType_OnResume,                            ///< ::AppletMessage_Resume
    AppletHookType_OnCaptureButtonShortPressed,         ///< ::AppletMessage_CaptureButtonShortPressed
    AppletHookType_OnAlbumScreenShotTaken,              ///< ::AppletMessage_AlbumScreenShotTaken
    AppletHookType_RequestToDisplay,                    ///< ::AppletMessage_RequestToDisplay

    AppletHookType_Max,                                 ///< Number of applet hook types.
} AppletHookType;

/// AppletMessage, for \ref appletGetMessage. See also \ref AppletHookType.
typedef enum {
    AppletMessage_ExitRequest               = 4,    ///< Exit request.
    AppletMessage_FocusStateChanged         = 15,   ///< FocusState changed.
    AppletMessage_Resume                    = 16,   ///< Current applet execution was resumed.
    AppletMessage_OperationModeChanged      = 30,   ///< OperationMode changed.
    AppletMessage_PerformanceModeChanged    = 31,   ///< PerformanceMode changed.
    AppletMessage_RequestToDisplay          = 51,   ///< Display requested, see \ref appletApproveToDisplay.
    AppletMessage_CaptureButtonShortPressed = 90,   ///< Capture button was short-pressed.
    AppletMessage_AlbumScreenShotTaken      = 92,   ///< Screenshot was taken.
    AppletMessage_AlbumRecordingSaved       = 93,   ///< AlbumRecordingSaved
} AppletMessage;

/// FocusState
typedef enum {
    AppletFocusState_InFocus    = 1,                  ///< Applet is focused.
    AppletFocusState_OutOfFocus = 2,                  ///< Out of focus - LibraryApplet open.
    AppletFocusState_Background = 3                   ///< Out of focus - HOME menu open / console is sleeping.
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
    AppletLaunchParameterKind_UserChannel     = 1, ///< UserChannel. Application-specific LaunchParameter.
    AppletLaunchParameterKind_PreselectedUser = 2, ///< account PreselectedUser
    AppletLaunchParameterKind_Unknown         = 3, ///< Unknown if used by anything?
} AppletLaunchParameterKind;

/// AppletId
typedef enum {
    AppletId_None                      = 0x00,        ///<                  None
    AppletId_application               = 0x01,        ///<                  Application. Not valid for use with LibraryApplets.
    AppletId_OverlayApplet             = 0x02,        ///< 010000000000100C "overlayDisp"
    AppletId_SystemAppletMenu          = 0x03,        ///< 0100000000001000 "qlaunch" (SystemAppletMenu)
    AppletId_SystemApplication         = 0x04,        ///< 0100000000001012 "starter" SystemApplication.
    AppletId_LibraryAppletAuth         = 0x0A,        ///< 0100000000001001 "auth"
    AppletId_LibraryAppletCabinet      = 0x0B,        ///< 0100000000001002 "cabinet"
    AppletId_LibraryAppletController   = 0x0C,        ///< 0100000000001003 "controller"
    AppletId_LibraryAppletDataErase    = 0x0D,        ///< 0100000000001004 "dataErase"
    AppletId_LibraryAppletError        = 0x0E,        ///< 0100000000001005 "error"
    AppletId_LibraryAppletNetConnect   = 0x0F,        ///< 0100000000001006 "netConnect"
    AppletId_LibraryAppletPlayerSelect = 0x10,        ///< 0100000000001007 "playerSelect"
    AppletId_LibraryAppletSwkbd        = 0x11,        ///< 0100000000001008 "swkbd"
    AppletId_LibraryAppletMiiEdit      = 0x12,        ///< 0100000000001009 "miiEdit"
    AppletId_LibraryAppletWeb          = 0x13,        ///< 010000000000100A "LibAppletWeb" WebApplet applet
    AppletId_LibraryAppletShop         = 0x14,        ///< 010000000000100B "LibAppletShop" ShopN applet
    AppletId_LibraryAppletPhotoViewer  = 0x15,        ///< 010000000000100D "photoViewer"
    AppletId_LibraryAppletSet          = 0x16,        ///< 010000000000100E "set" (This applet is currently not present on retail devices.)
    AppletId_LibraryAppletOfflineWeb   = 0x17,        ///< 010000000000100F "LibAppletOff" offlineWeb applet
    AppletId_LibraryAppletLoginShare   = 0x18,        ///< 0100000000001010 "LibAppletLns" loginShare web-applet
    AppletId_LibraryAppletWifiWebAuth  = 0x19,        ///< 0100000000001011 "LibAppletAuth" wifiWebAuth applet
    AppletId_LibraryAppletMyPage       = 0x1A,        ///< 0100000000001013 "myPage"
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

/// AppletApplicationExitReason
typedef enum {
    AppletApplicationExitReason_Normal     = 0,
    AppletApplicationExitReason_Unknown1   = 1,
    AppletApplicationExitReason_Unknown2   = 2,
    AppletApplicationExitReason_Unknown3   = 3,
    AppletApplicationExitReason_Unknown4   = 4,
    AppletApplicationExitReason_Unknown5   = 5,
    AppletApplicationExitReason_Unexpected = 100,
} AppletApplicationExitReason;

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
    AppletSystemButtonType_PowerButtonShortPressing   = 3,   ///< Short-pressing with the Power-button. Only available with \ref appletPerformSystemButtonPressing.
    AppletSystemButtonType_PowerButtonLongPressing    = 4,   ///< Long-pressing with the Power-button. Only available with \ref appletPerformSystemButtonPressing.
    AppletSystemButtonType_Shutdown                   = 5,   ///< Shutdown the system, as if the Power-button was held for longer than ::AppletSystemButtonType_PowerButtonLongPressing. Only available with \ref appletPerformSystemButtonPressing.
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

/// Input policy values for \ref appletSetInputDetectionPolicy.
typedef enum {
    AppletInputDetectionPolicy_Unknown0 = 0,       ///< Unknown.
    AppletInputDetectionPolicy_Unknown1 = 1,       ///< Unknown.
} AppletInputDetectionPolicy;

/// Input mode values for \ref appletSetWirelessPriorityMode.
typedef enum {
    AppletWirelessPriorityMode_Default          = 1,       ///< Default
    AppletWirelessPriorityMode_OptimizedForWlan = 2,       ///< OptimizedForWlan
} AppletWirelessPriorityMode;

/// CaptureSharedBuffer for the IDisplayController commands.
typedef enum {
    AppletCaptureSharedBuffer_LastApplication = 0,                     ///< LastApplication
    AppletCaptureSharedBuffer_LastForeground  = 1,                     ///< LastForeground
    AppletCaptureSharedBuffer_CallerApplet    = 2,                     ///< CallerApplet
} AppletCaptureSharedBuffer;

/// WindowOriginMode
typedef enum {
    AppletWindowOriginMode_LowerLeft          = 0,                     ///< LowerLeft
    AppletWindowOriginMode_UpperLeft          = 1,                     ///< UpperLeft
} AppletWindowOriginMode;

/// ProgramSpecifyKind for the ExecuteProgram cmd. Controls the type of the u64 passed to the ExecuteProgram cmd.
typedef enum {
    AppletProgramSpecifyKind_ExecuteProgram                            = 0,    ///< u8 ProgramIndex.
    AppletProgramSpecifyKind_JumpToSubApplicationProgramForDevelopment = 1,    ///< u64 application_id. Only available when DebugMode is enabled.
    AppletProgramSpecifyKind_RestartProgram                            = 2,    ///< u64 = value 0.
} AppletProgramSpecifyKind;

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
    Service s;                         ///< IStorage
    TransferMemory tmem;               ///< TransferMemory
} AppletStorage;

/// LibraryApplet state.
typedef struct {
    Service s;                         ///< ILibraryAppletAccessor
    Event StateChangedEvent;           ///< Output from GetAppletStateChangedEvent, autoclear=false.
    Event PopInteractiveOutDataEvent;  ///< Output from GetPopInteractiveOutDataEvent, autoclear=false.
    LibAppletMode mode;                ///< See ref \ref LibAppletMode.
    u64 layer_handle;                  ///< Output from GetIndirectLayerConsumerHandle on [2.0.0+].
    bool creating_self;                ///< When set, indicates that the LibraryApplet is creating itself.
    LibAppletExitReason exitreason;    ///< Set by \ref appletHolderJoin using the output from cmd GetResult, see \ref LibAppletExitReason.
} AppletHolder;

/// IApplicationAccessor container.
typedef struct {
    Service s;                                 ///< IApplicationAccessor
    Event StateChangedEvent;                   ///< Output from GetAppletStateChangedEvent, autoclear=false.
    AppletApplicationExitReason exitreason;    ///< Set by \ref appletApplicationJoin using the output from cmd GetResult, see \ref AppletApplicationExitReason.
} AppletApplication;

/// GpuErrorHandler
typedef struct {
    Service s;                                 ///< IGpuErrorHandler
} AppletGpuErrorHandler;

/// Used by \ref appletInitialize with __nx_applet_AppletAttribute for cmd OpenLibraryAppletProxy (AppletType_LibraryApplet), on [3.0.0+]. The default for this struct is all-zero.
typedef struct {
    u8 flag;                           ///< Flag. When non-zero, two state fields are set to 1.
    u8 reserved[0x7F];                 ///< Unused.
} AppletAttribute;

/// LibraryAppletInfo
typedef struct {
    AppletId appletId;                 ///< \ref AppletId
    LibAppletMode mode;                ///< \ref LibAppletMode
} LibAppletInfo;

/// AppletProcessLaunchReason, from GetLaunchReason.
typedef struct {
    u8 flag;                           ///< When non-zero, indicates that OpenCallingLibraryApplet should be used.
    u8 unk_x1[3];                      ///< Always zero.
} AppletProcessLaunchReason;

/// Cached info for the current LibraryApplet, from \ref appletGetAppletInfo.
typedef struct {
    LibAppletInfo info;                ///< Output from \ref appletGetLibraryAppletInfo.
    bool caller_flag;                  ///< Loaded from AppletProcessLaunchReason::flag, indicates that the below AppletHolder is initialized.
    AppletHolder caller;               ///< \ref AppletHolder for the CallingLibraryApplet, automatically closed by \ref appletExit when needed.
} AppletInfo;

/// IdentityInfo
typedef struct {
    AppletId appletId;                 ///< \ref AppletId
    u32 pad;                           ///< Padding.
    u64 application_id;                ///< ApplicationId, only set with appletId == ::AppletId_application.
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

/// ApplicationLaunchProperty
typedef struct {
    u64 application_id;                ///< ApplicationId.
    u32 version;                       ///< Application version.
    u8 app_storageId;                  ///< \ref NcmStorageId for the Application.
    u8 update_storageId;               ///< \ref NcmStorageId for the Application update.
    u8 unk_xa;                         ///< Unknown.
    u8 pad;                            ///< Padding.
} AppletApplicationLaunchProperty;

/// ApplicationLaunchRequestInfo
typedef struct {
    u32 unk_x0;                        ///< Unknown. The default is 0x0 with \ref appletCreateSystemApplication, 0x3 with \ref appletCreateApplication.
    u32 unk_x4;                        ///< Unknown. The default is 0x0 with \ref appletCreateSystemApplication, 0x3 with \ref appletCreateApplication.
    u8 unk_x8[0x8];                    ///< Unknown. The default is 0x0.
} AppletApplicationLaunchRequestInfo;

/// AppletResourceUsageInfo, from \ref appletGetAppletResourceUsageInfo.
typedef struct {
    u32 counter0;                      ///< Unknown counter.
    u32 counter1;                      ///< Unknown counter.
    u32 counter2;                      ///< Output from ns cmd GetRightsEnvironmentCountForDebug.
    u8 unused[0x14];                   ///< Always zero.
} AppletResourceUsageInfo;

/// Initialize applet, called automatically during app startup.
Result appletInitialize(void);

/// Exit applet, called automatically during app exit.
void appletExit(void);

/// Gets the Service object for the actual "appletOE"/"appletAE" service session.
Service* appletGetServiceSession_Proxy(void);

/// Gets the Service object for IAppletCommonFunctions. Only initialized with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [7.0.0+].
Service* appletGetServiceSession_AppletCommonFunctions(void);

/// Gets the Service object for I*Functions, specific to each AppletType (IApplicationFunctions for AppletType_*Application). Not initialized with AppletType_LibraryApplet pre-15.0.0. On [15.0.0+] with AppletType_LibraryApplet this returns the object for IHomeMenuFunctions.
Service* appletGetServiceSession_Functions(void);

/// Gets the Service object for IGlobalStateController. Only initialized with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
Service* appletGetServiceSession_GlobalStateController(void);

/// Gets the Service object for IApplicationCreator. Only initialized with AppletType_SystemApplet.
Service* appletGetServiceSession_ApplicationCreator(void);

/// Gets the Service object for ILibraryAppletSelfAccessor. Only initialized with AppletType_LibraryApplet.
Service* appletGetServiceSession_LibraryAppletSelfAccessor(void);

/// Gets the Service object for IProcessWindingController. Only initialized with AppletType_LibraryApplet.
Service* appletGetServiceSession_ProcessWindingController(void);

/// Gets the Service object for ILibraryAppletCreator.
Service* appletGetServiceSession_LibraryAppletCreator(void);

/// Gets the Service object for ICommonStateGetter.
Service* appletGetServiceSession_CommonStateGetter(void);

/// Gets the Service object for ISelfController.
Service* appletGetServiceSession_SelfController(void);

/// Gets the Service object for IWindowController.
Service* appletGetServiceSession_WindowController(void);

/// Gets the Service object for IAudioController.
Service* appletGetServiceSession_AudioController(void);

/// Gets the Service object for IDisplayController.
Service* appletGetServiceSession_DisplayController(void);

/// Gets the Service object for IDebugFunctions.
Service* appletGetServiceSession_DebugFunctions(void);

/// Get the cached AppletResourceUserId.
u64 appletGetAppletResourceUserId(void);

/// Get the \ref AppletType.
AppletType appletGetAppletType(void);

/// Sets the state field for \ref AppletThemeColorType.
void appletSetThemeColorType(AppletThemeColorType theme);

/// Gets the state field for \ref AppletThemeColorType. Used internally by \ref libappletArgsCreate.
AppletThemeColorType appletGetThemeColorType(void);

///@name ICommonStateGetter
///@{

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
 * @brief GetWakeupCount
 * @note Only available with [11.0.0+].
 * @param[out] out Output value.
 */
Result appletGetWakeupCount(u64 *out);

/**
 * @brief Pushes a storage to the general channel. Used for sending requests to SystemApplet.
 * @note  This is not usable under an Application, however it is usable under a LibraryApplet.
 * @note  This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
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
 * @brief SetVrPositionForDebug
 * @note The cached value loaded from \ref setsysGetDebugModeFlag must be 1, otherwise an error is returned.
 * @note Only available with [11.0.0+].
 * @param[in] x X, must not be negative. x+width must be <=1280.
 * @param[in] y Y, must not be negative. y+height must be <=720.
 * @param[in] width Width, must be 1-1280.
 * @param[in] height Height, must be 1-720.
 */
Result appletSetVrPositionForDebug(s32 x, s32 y, s32 width, s32 height);

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
 * @brief Gets the ApplicationId for the specified ContentActionName string.
 * @note Only available when the current applet is an AppletType_SystemApplication on [5.1.0+].
 * @param[out] application_id ApplicationId.
 * @param[in] name ContentActionName string.
 */
Result appletGetApplicationIdByContentActionName(u64 *application_id, const char *name);

/**
 * @brief Sets the \ref ApmCpuBoostMode.
 * @note Only available with [7.0.0+] (not fully usable system-side with 6.x).
 * @param mode \ref ApmCpuBoostMode.
 */
Result appletSetCpuBoostMode(ApmCpuBoostMode mode);

/**
 * @brief CancelCpuBoostMode
 * @note Only available with [10.0.0+].
 */
Result appletCancelCpuBoostMode(void);

/**
 * @brief GetBuiltInDisplayType
 * @note Only available with [11.0.0+].
 * @param[out] out Output value.
 */
Result appletGetBuiltInDisplayType(s32 *out);

/**
 * @brief Perform SystemButtonPressing with the specified \ref AppletSystemButtonType. Internally this cmd checks a state field, verifies that the type is allowed, then runs the same func as \ref appletPerformSystemButtonPressing internally.
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
 * @brief Opens an \ref AppletGpuErrorHandler.
 * @note The cached value loaded from \ref setsysGetDebugModeFlag must be 1, otherwise an error is returned.
 * @note Only available with [11.0.0+].
 * @param[out] g \ref AppletGpuErrorHandler
 */
Result appletOpenMyGpuErrorHandler(AppletGpuErrorHandler *g);

/**
 * @brief Gets the OperationModeSystemInfo.
 * @note Only available with [7.0.0+].
 * @param[out] info Output info.
 */
Result appletGetOperationModeSystemInfo(u32 *info);

/**
 * @brief This uses \ref setsysGetPlatformRegion internally.
 * @note Only available with [9.0.0+].
 * @param[out] out \ref SetSysPlatformRegion
 */
Result appletGetSettingsPlatformRegion(SetSysPlatformRegion *out);

/**
 * @brief ActivateMigrationService
 * @note Only available with [10.0.0+].
 */
Result appletActivateMigrationService(void);

/**
 * @brief DeactivateMigrationService
 * @note Only available with [10.0.0+].
 */
Result appletDeactivateMigrationService(void);

/**
 * @brief DisableSleepTillShutdown
 * @note Only available with [11.0.0+].
 */
Result appletDisableSleepTillShutdown(void);

/**
 * @brief SuppressDisablingSleepTemporarily
 * @param[in] val Nanoseconds value.
 * @note Only available with [11.0.0+].
 */
Result appletSuppressDisablingSleepTemporarily(u64 val);

/**
 * @brief SetRequestExitToLibraryAppletAtExecuteNextProgramEnabled
 * @note Only available with [11.0.0+].
 */
Result appletSetRequestExitToLibraryAppletAtExecuteNextProgramEnabled(void);

///@}

///@name IGpuErrorHandler
///@{

/**
 * @brief Close an \ref AppletGpuErrorHandler.
 * @param g \ref AppletGpuErrorHandler
 */
void appletGpuErrorHandlerClose(AppletGpuErrorHandler *g);

/**
 * @brief Gets the size of the info available with \ref appletGpuErrorHandlerGetManualGpuErrorInfo.
 * @param g \ref AppletGpuErrorHandler
 * @param[out] out Output size.
 */
Result appletGpuErrorHandlerGetManualGpuErrorInfoSize(AppletGpuErrorHandler *g, u64 *out);

/**
 * @brief GetManualGpuErrorInfo
 * @param g \ref AppletGpuErrorHandler
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size, must be >= the output size from \ref appletGpuErrorHandlerGetManualGpuErrorInfoSize.
 * @param[out] out Output value.
 */
Result appletGpuErrorHandlerGetManualGpuErrorInfo(AppletGpuErrorHandler *g, void* buffer, size_t size, u64 *out);

/**
 * @brief GetManualGpuErrorDetectionSystemEvent
 * @param g \ref AppletGpuErrorHandler
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGpuErrorHandlerGetManualGpuErrorDetectionSystemEvent(AppletGpuErrorHandler *g, Event *out_event);

/**
 * @brief FinishManualGpuErrorHandling
 * @param g \ref AppletGpuErrorHandler
 */
Result appletGpuErrorHandlerFinishManualGpuErrorHandling(AppletGpuErrorHandler *g);

///@}

///@name ISelfController
///@{

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
 * @brief Sets whether ::AppletMessage_Resume is enabled.
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
 * @brief Sets whether ::AppletMessage_CaptureButtonShortPressed is enabled.
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
 * @param[in] layout Input \ref SetKeyboardLayout.
 */
Result appletSetDesirableKeyboardLayout(SetKeyboardLayout layout);

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
 * @brief CreateManagedDisplaySeparableLayer
 * @note Only available with [10.0.0+].
 * @param[out] display_layer Output display_layer.
 * @param[out] recording_layer Output recording_layer.
 */
Result appletCreateManagedDisplaySeparableLayer(u64 *display_layer, u64 *recording_layer);

/**
 * @brief SetManagedDisplayLayerSeparationMode
 * @note Only available with [10.0.0+].
 * @param[in] mode Mode. Must be 0-1.
 */
Result appletSetManagedDisplayLayerSeparationMode(u32 mode);

/**
 * @brief Sets whether ::AppletMessage_RequestToDisplay is enabled.
 * @note Sets an internal state flag. When the input flag is 0, this will in additional run the same code as \ref appletApproveToDisplay.
 * @param[in] flag Flag
 */
Result appletSetHandlesRequestToDisplay(bool flag);

/**
 * @brief Approve the display requested by ::AppletMessage_RequestToDisplay, see also \ref appletSetHandlesRequestToDisplay.
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
 * @brief Sets the \ref AppletInputDetectionPolicy.
 * @note Only available with [9.0.0+].
 * @param[in] policy \ref AppletInputDetectionPolicy
 */
Result appletSetInputDetectionPolicy(AppletInputDetectionPolicy policy);

/**
 * @brief Sets the WirelessPriorityMode.
 * @note Only available with [4.0.0+].
 * @param[in] mode \ref AppletWirelessPriorityMode
 */
Result appletSetWirelessPriorityMode(AppletWirelessPriorityMode mode);

/**
 * @brief Gets the total time in nanoseconds that the current process was actively running (not suspended), relative to when \ref appletInitialize was last used.
 * @note Only available with [6.0.0+].
 * @param[out] activeTime Output nanoseconds value.
 */
Result appletGetProgramTotalActiveTime(u64 *activeTime);

/**
 * @brief Sets whether ::AppletMessage_AlbumScreenShotTaken is enabled.
 * @note Only available with [7.0.0+].
 * @param[in] flag Whether to enable the notification.
 */
Result appletSetAlbumImageTakenNotificationEnabled(bool flag);

/**
 * @brief Sets the Application AlbumUserData.
 * @note Only available with [8.0.0+].
 * @param[in] buffer Buffer containing arbitrary UserData.
 * @param[in] size Buffer size, must be <=0x400.
 */
Result appletSetApplicationAlbumUserData(const void* buffer, size_t size);

/**
 * @brief SaveCurrentScreenshot
 * @note Only available with [11.0.0+].
 * @param[in] option \ref AlbumReportOption
 */
Result appletSaveCurrentScreenshot(AlbumReportOption option);

///@}

///@name IWindowController
///@{

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

///@}

///@name IAudioController
///@{

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

///@}

///@name IDisplayController
///@{

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

///@}

///@name IProcessWindingController
///@{

/**
 * @brief Pushes a storage to the ContextStack. Normally this should only be used when AppletInfo::caller_flag is true.
 * @note Only available with AppletType_LibraryApplet.
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 */
Result appletPushContext(AppletStorage *s);

/**
 * @brief Pops a storage from the ContextStack. Normally this should only be used when AppletInfo::caller_flag is true.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] s Storage object.
 */
Result appletPopContext(AppletStorage *s);

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
 * @note Similar to \ref appletLockAccessorTryLock, except this uses timeout UINT64_MAX with the eventWait call, and this uses TryLock repeatedly until the output flag value is true.
 * @param a LockAccessor object.
 */
Result appletLockAccessorLock(AppletLockAccessor *a);

/**
 * @brief Unlock a LockAccessor.
 * @param a LockAccessor object.
 */
Result appletLockAccessorUnlock(AppletLockAccessor *a);

///@}

///@name ILibraryAppletCreator
///@{

/**
 * @brief Creates a LibraryApplet.
 * @param h AppletHolder object.
 * @param id See \ref AppletId.
 * @param mode See \ref LibAppletMode.
 */
Result appletCreateLibraryApplet(AppletHolder *h, AppletId id, LibAppletMode mode);

/**
 * @brief Creates a LibraryApplet. This is for when a LibraryApplet creates itself.
 * @note  Identical to \ref appletCreateLibraryApplet except this sets the creating_self flag to true.
 * @param h AppletHolder object.
 * @param id See \ref AppletId.
 * @param mode See \ref LibAppletMode.
 */
Result appletCreateLibraryAppletSelf(AppletHolder *h, AppletId id, LibAppletMode mode);

/**
 * @brief TerminateAllLibraryApplets which were created by the current applet.
 * @note Normally LibraryApplet cleanup should be handled via \ref AppletHolder.
 */
Result appletTerminateAllLibraryApplets(void);

/**
 * @brief AreAnyLibraryAppletsLeft which were created by the current applet.
 * @param[out] out Output flag.
 */
Result appletAreAnyLibraryAppletsLeft(bool *out);

///@}

///@name ILibraryAppletAccessor
///@{

/// Closes an AppletHolder object.
void appletHolderClose(AppletHolder *h);

/// Returns whether the AppletHolder object was initialized.
bool appletHolderActive(AppletHolder *h);

/**
 * @brief Gets the IndirectLayerConsumerHandle loaded during \ref appletCreateLibraryApplet, on [2.0.0+].
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
 * @brief Jumps to the LibraryApplet, with the current-LibraryApplet being terminated. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_LibraryApplet.
 * @param h AppletHolder object.
 */
Result appletHolderJump(AppletHolder *h);

/**
 * @brief Requests the LibraryApplet to exit. The command is only used if \ref appletHolderCheckFinished returns false.
 * @param h AppletHolder object.
 */
Result appletHolderRequestExit(AppletHolder *h);

/**
 * @brief Terminate the LibraryApplet.
 * @param h AppletHolder object.
 */
Result appletHolderTerminate(AppletHolder *h);

/**
 * @brief Uses cmds GetAppletStateChangedEvent and RequestExit, then waits for the LibraryApplet to exit with the specified timeout. If a timeout occurs, the Terminate cmd is used.
 * @param h AppletHolder object.
 * @param[in] timeout Timeout in nanoseconds. UINT64_MAX for no timeout.
 */
Result appletHolderRequestExitOrTerminate(AppletHolder *h, u64 timeout);

/**
 * @brief Waits for the LibraryApplet to exit.
 * @param h AppletHolder object.
 */
void appletHolderJoin(AppletHolder *h);

/**
 * @brief Gets the LibraryApplet StateChangedEvent.
 * @param h AppletHolder object.
 */
NX_CONSTEXPR Event *appletHolderGetExitEvent(AppletHolder *h) {
    return &h->StateChangedEvent;
}

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
 * @brief PresetLibraryAppletGpuTimeSliceZero
 * @note Only available with [10.0.0+].
 * @param h AppletHolder object.
 */
Result appletHolderPresetLibraryAppletGpuTimeSliceZero(AppletHolder *h);

/**
 * @brief Gets the PopInteractiveOutDataEvent.
 * @param h AppletHolder object.
 * @param[out] out_event Output Event.
 */
Result appletHolderGetPopInteractiveOutDataEvent(AppletHolder *h, Event **out_event);

/**
 * @brief Waits for the PopInteractiveOutDataEvent and StateChangedEvent.
 * @return false for error / when StateChangedEvent was signaled, and true when PopInteractiveOutDataEvent was signaled. The latter is signaled when a new storage is available with \ref appletHolderPopInteractiveOutData where previously no storage was available (this willl not clear the event), this event is automatically cleared by the system once the last storage is popped.
 * @param h AppletHolder object.
 */
bool appletHolderWaitInteractiveOut(AppletHolder *h);

/**
 * @brief Pushes a storage for LibraryApplet input.
 * @note  This uses \ref appletStorageClose automatically.
 * @param h AppletHolder object.
 * @param[in] s Storage object.
 */
Result appletHolderPushInData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pops a storage from LibraryApplet output.
 * @param h AppletHolder object.
 * @param[out] s Storage object.
 */
Result appletHolderPopOutData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pushes a storage for LibraryApplet Extra storage input.
 * @note  This uses \ref appletStorageClose automatically.
 * @param h AppletHolder object.
 * @param[in] s Storage object.
 */
Result appletHolderPushExtraStorage(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pushes a storage for LibraryApplet Interactive input.
 * @note  This uses \ref appletStorageClose automatically.
 * @param h AppletHolder object.
 * @param[in] s Storage object.
 */
Result appletHolderPushInteractiveInData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Pops a storage from LibraryApplet Interactive output.
 * @param h AppletHolder object.
 * @param[out] s Storage object.
 */
Result appletHolderPopInteractiveOutData(AppletHolder *h, AppletStorage *s);

/**
 * @brief Gets the \ref LibAppletInfo for the specified LibraryApplet.
 * @param h AppletHolder object.
 * @param[out] info \ref LibAppletInfo
 */
Result appletHolderGetLibraryAppletInfo(AppletHolder *h, LibAppletInfo *info);

///@}

///@name (ILibraryAppletCreator ->) IStorage
///@{

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
 * @brief Creates a HandleStorage.
 * @note Only available on [2.0.0+].
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
 * @note  Only available on [2.0.0+].
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

///@}

///@name IApplicationFunctions: IFunctions for AppletType_*Application.
///@{

/**
 * @brief Pops a LaunchParameter AppletStorage, the storage will be removed from sysmodule state during this.
 * @param[out] s Output storage.
 * @param kind See \ref AppletLaunchParameterKind.
 * @note Only available with AppletType_*Application.
 * @note See also acc.h \ref accountGetPreselectedUser (wrapper for appletPopLaunchParameter etc).
 */
Result appletPopLaunchParameter(AppletStorage *s, AppletLaunchParameterKind kind);

/**
 * @brief Requests to launch the specified application.
 * @note Only available with AppletType_*Application, or AppletType_LibraryApplet on [5.0.0+].
 * @param[in] application_id ApplicationId. Value 0 can be used to relaunch the current application.
 * @param[in] s Optional AppletStorage object, can be NULL. This is automatically closed. When NULL on pre-4.0.0 (or with AppletType_LibraryApplet), this will internally create a tmp storage with size 0 for use with the cmd. This is the storage available to the launched application via \ref appletPopLaunchParameter with ::AppletLaunchParameterKind_UserChannel.
 */
Result appletRequestLaunchApplication(u64 application_id, AppletStorage* s);

/**
 * @brief Requests to launch the specified application, for kiosk systems.
 * @note Only available with AppletType_*Application on [3.0.0+].
 * @note Identical to \ref appletRequestLaunchApplication, except this allows the user to specify the attribute fields instead of the defaults being used.
 * @param[in] application_id ApplicationId
 * @param[in] s Optional AppletStorage object, can be NULL. This is automatically closed. When NULL on pre-4.0.0, this will internally create a tmp storage with size 0 for use with the cmd. This is the storage available to the launched application via \ref appletPopLaunchParameter with ::AppletLaunchParameterKind_UserChannel.
 * @param[in] attr Kiosk application attributes.
 */
Result appletRequestLaunchApplicationForQuest(u64 application_id, AppletStorage* s, const AppletApplicationAttributeForQuest *attr);

/**
 * @brief Gets the DesiredLanguage for the current host application control.nacp.
 * @note Only available with AppletType_*Application.
 * @param[out] LanguageCode Output LanguageCode, see set.h.
 */
Result appletGetDesiredLanguage(u64 *LanguageCode);

/**
 * @brief Gets the DisplayVersion for the current host application control.nacp.
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
 * @brief Gets the PseudoDeviceId. This is derived from the output of a ns command, and from data in the host application control.nacp.
 * @note Only available with AppletType_*Application on [2.0.0+].
 * @param[out] out Output PseudoDeviceId.
 */
Result appletGetPseudoDeviceId(Uuid *out);

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
/// Only available with AppletType_Application on [3.0.0+], hence errors from this can be ignored.
/// Video recording is only fully available system-side with [4.0.0+].
/// Only usable when running under an application which supports video recording. Using this is only needed when the host application control.nacp has VideoCaptureMode set to Enabled, with Automatic appletInitializeGamePlayRecording is not needed.
Result appletInitializeGamePlayRecording(void);

/**
 * @brief Requests to save the video recording, as if the Capture-button was held.
 * @note Only available with AppletType_*Application on [4.0.0+].
 */
Result appletRequestFlushGamePlayingMovieForDebug(void);

/**
 * @brief Requests a system shutdown. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_*Application on [3.0.0+].
 */
Result appletRequestToShutdown(void);

/**
 * @brief Requests a system reboot. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_*Application on [3.0.0+].
 */
Result appletRequestToReboot(void);

/**
 * @brief RequestToSleep
 * @note Only available with AppletType_*Application on [10.0.0+].
 */
Result appletRequestToSleep(void);

/**
 * @brief Exit the application and return to the kiosk demo menu. This terminates the current process. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_*Application on [4.0.0+], on kiosk systems (QuestFlag set).
 */
Result appletExitAndRequestToShowThanksMessage(void);

/**
 * @brief Initializes the ApplicationCopyrightFrameBuffer, with dimensions 1280x720 + the tmem for it. This is used as an overlay for screenshots.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @note Cleanup for this is handled automatically during app exit in \ref appletExit.
 */
Result appletInitializeApplicationCopyrightFrameBuffer(void);

/**
 * @brief Sets the RGBA8 image for use with \ref appletInitializeApplicationCopyrightFrameBuffer. Overrides the current image, if this was already used previously.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @note The specified coordinates and width/height must be within the bounds of the framebuffer setup by \ref appletInitializeApplicationCopyrightFrameBuffer.
 * @param[in] buffer Input image buffer.
 * @param[in] size Input image buffer size.
 * @param[in] x X coordinate. Must not be negative.
 * @param[in] y Y coordinate. Must not be negative.
 * @param[in] width Image width. Must be >=1.
 * @param[in] height Image height. Must be >=1.
 * @param[in] mode \ref AppletWindowOriginMode
 */
Result appletSetApplicationCopyrightImage(const void* buffer, size_t size, s32 x, s32 y, s32 width, s32 height, AppletWindowOriginMode mode);

/**
 * @brief Sets the visibility for the image set by \ref appletSetApplicationCopyrightImage, in screenshots.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @param[in] visible Whether the image is visible. The default is true.
 */
Result appletSetApplicationCopyrightVisibility(bool visible);

/**
 * @brief Gets ApplicationPlayStatistics.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @note The input ApplicationIds must be allowed via control.nacp with the current host application. The minimum allowed ApplicationId is the ApplicationId for the current application.
 * @param stats Output \ref PdmApplicationPlayStatistics array.
 * @param application_ids Input ApplicationIds array.
 * @param count Total entries in the input/output arrays.
 * @param total_out Total output entries.
 */
Result appletQueryApplicationPlayStatistics(PdmApplicationPlayStatistics *stats, const u64 *application_ids, s32 count, s32 *total_out);

/**
 * @brief Same as \ref appletQueryApplicationPlayStatistics except this gets playstats specific to the input userId.
 * @note Only available with AppletType_*Application on [6.0.0+].
 * @param[in] uid \ref AccountUid
 * @param[out] stats Output \ref PdmApplicationPlayStatistics array.
 * @param[in] application_ids Input ApplicationIds array.
 * @param[in] count Total entries in the input/output arrays.
 * @param[out] total_out Total output entries.
 */
Result appletQueryApplicationPlayStatisticsByUid(AccountUid uid, PdmApplicationPlayStatistics *stats, const u64 *application_ids, s32 count, s32 *total_out);

/**
 * @brief Launches Application {current_ApplicationId}+programIndex. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @note Creates the storage if needed. Uses cmd ClearUserChannel. Uses cmd UnpopToUserChannel when the storage was created. Lastly cmd ExecuteProgramCmd is used.
 * @param[in] programIndex ProgramIndex, must be 0x0-0xFF. 0 is the same as the current application. ProgramIndex values where the application is not installed should not be used.
 * @param[in] buffer Optional buffer containing the storage data which will be used for ::AppletLaunchParameterKind_UserChannel with the launched Application, can be NULL.
 * @param[in] size Size of the above buffer, 0 to not use the storage. Must be <=0x1000.
 */
Result appletExecuteProgram(s32 programIndex, const void* buffer, size_t size);

/**
 * @brief Launches the specified ApplicationId.
 * @note Only available with AppletType_*Application on [5.0.0+], with DebugMode enabled.
 * @note Creates the storage if needed. Uses cmd ClearUserChannel. Uses cmd UnpopToUserChannel when the storage was created. Lastly cmd ExecuteProgramCmd is used.
 * @param[in] application_id ApplicationId.
 * @param[in] buffer Optional buffer containing the storage data which will be used for ::AppletLaunchParameterKind_UserChannel with the launched Application, can be NULL.
 * @param[in] size Size of the above buffer, 0 to not use the storage. Must be <=0x1000.
 */
Result appletJumpToSubApplicationProgramForDevelopment(u64 application_id, const void* buffer, size_t size);

/**
 * @brief Relaunches the current Application.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @note Creates the storage if needed. Uses cmd ClearUserChannel. Uses cmd UnpopToUserChannel when the storage was created. Lastly cmd ExecuteProgramCmd is used.
 * @param[in] buffer Optional buffer containing the storage data which will be used for ::AppletLaunchParameterKind_UserChannel with the launched Application, can be NULL.
 * @param[in] size Size of the above buffer, 0 to not use the storage. Must be <=0x1000.
 */
Result appletRestartProgram(const void* buffer, size_t size);

/**
 * @brief Gets the ProgramIndex of the program which launched this program.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @param[out] programIndex ProgramIndex, -1 when there was no previous program.
 */
Result appletGetPreviousProgramIndex(s32 *programIndex);

/**
 * @brief SetDelayTimeToAbortOnGpuError
 * @note Only available with AppletType_*Application on [11.0.0+].
 * @param[in] val Input nanoseconds value.
 */
Result appletSetDelayTimeToAbortOnGpuError(u64 val);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletTryPopFromFriendInvitationStorageChannel where previously no storage was available, this event is automatically cleared by the system once the last storage is popped.
 * @note This is used by \ref friendsGetFriendInvitationNotificationEvent.
 * @note Only available with AppletType_*Application on [9.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetFriendInvitationStorageChannelEvent(Event *out_event);

/**
 * @brief Pops a storage from the FriendInvitation StorageChannel.
 * @note This is used by \ref friendsTryPopFriendInvitationNotificationInfo.
 * @note Only available with AppletType_*Application on [9.0.0+].
 * @param[out] s Storage object.
 */
Result appletTryPopFromFriendInvitationStorageChannel(AppletStorage *s);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletTryPopFromNotificationStorageChannel where previously no storage was available, this event is automatically cleared by the system once the last storage is popped.
 * @note This is used by \ref notifGetNotificationSystemEvent.
 * @note Only available with AppletType_*Application on [9.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetNotificationStorageChannelEvent(Event *out_event);

/**
 * @brief Pops a storage from the Notification StorageChannel.
 * @note This is used by \ref notifTryPopNotifiedApplicationParameter.
 * @note Only available with AppletType_*Application on [9.0.0+].
 * @param[out] s Storage object.
 */
Result appletTryPopFromNotificationStorageChannel(AppletStorage *s);

/**
 * @brief GetHealthWarningDisappearedSystemEvent
 * @note Only available with AppletType_*Application on [9.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetHealthWarningDisappearedSystemEvent(Event *out_event);

/**
 * @brief SetHdcpAuthenticationActivated
 * @note Only available with AppletType_*Application on [9.0.0+].
 * @param[in] flag Whether HdcpAuthentication is activated.
 */
Result appletSetHdcpAuthenticationActivated(bool flag);

/**
 * @brief GetLastApplicationExitReason
 * @note Only available with AppletType_*Application on [11.0.0+].
 * @param[out] out Output value.
 */
Result appletGetLastApplicationExitReason(s32 *out);

/**
 * @brief CreateMovieMaker. Do not use this directly, use \ref grcCreateMovieMaker instead.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @param[out] srv_out Output Service for applet IMovieMaker.
 * @param[in] tmem TransferMemory
 */
Result appletCreateMovieMaker(Service* srv_out, TransferMemory *tmem);

/**
 * @brief Launches the jit-sysmodule when it was not previously launched by this cmd. Returns 0 when it was previously launched.
 * @note Only available with AppletType_*Application on [5.0.0+].
 * @note Requires the jit-sysmodule to actually be installed.
 */
Result appletPrepareForJit(void);

///@}

///@name IHomeMenuFunctions: IFunctions for AppletType_SystemApplet and on [15.0.0+] for AppletType_LibraryApplet.
///@{

/**
 * @brief RequestToGetForeground
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet.
 */
Result appletRequestToGetForeground(void);

/**
 * @brief LockForeground
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet.
 */
Result appletLockForeground(void);

/**
 * @brief UnlockForeground
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet.
 */
Result appletUnlockForeground(void);

/**
 * @brief Pops a storage from the general channel.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet.
 * @param[out] s Storage object.
 */
Result appletPopFromGeneralChannel(AppletStorage *s);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletPopFromGeneralChannel where previously no storage was available, this event is automatically cleared by the system once the last storage is popped.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetPopFromGeneralChannelEvent(Event *out_event);

/**
 * @brief Gets a \ref AppletLockAccessor for HomeButtonWriter.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet.
 * @note Similar to using \ref appletGetWriterLockAccessorEx with inval=0.
 * @param a LockAccessor object.
 */
Result appletGetHomeButtonWriterLockAccessor(AppletLockAccessor *a);

/**
 * @brief IsSleepEnabled
 * @note Only available with AppletType_SystemApplet on [11.0.0+], or on [15.0.0+] with AppletType_LibraryApplet.
 * @param[out] out Output flag.
 */
Result appletIsSleepEnabled(bool *out);

/**
 * @brief PopRequestLaunchApplicationForDebug
 * @note Only available with AppletType_SystemApplet on [6.0.0+], or on [15.0.0+] with AppletType_LibraryApplet.
 * @param[out] uids Output array of \ref AccountUid.
 * @param[in] count Size of the uids array in entries, must be at least the size stored in state.
 * @param[out] application_id Output ApplicationId.
 * @param[out] total_out Total output userID entries.
 */
Result appletPopRequestLaunchApplicationForDebug(AccountUid *uids, s32 count, u64 *application_id, s32 *total_out);

/**
 * @brief IsForceTerminateApplicationDisabledForDebug
 * @note Only available with AppletType_SystemApplet on [9.0.0+], or on [15.0.0+] with AppletType_LibraryApplet.
 * @param[out] out Output flag. 0 when DebugMode is not enabled, otherwise this is loaded from a system-setting.
 */
Result appletIsForceTerminateApplicationDisabledForDebug(bool *out);

/**
 * @brief Launches DevMenu and the dev Overlay-applet. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_SystemApplet on [8.0.0+], or on [15.0.0+] with AppletType_LibraryApplet.
 * @note This verifies that DebugMode is enabled, then uses a ns cmd. That cmd then loads the system-settings for these two ProgramIds (which normally only exist on devunits), and verifies that these programs are installed + launches them.
 */
Result appletLaunchDevMenu(void);

/**
 * @brief SetLastApplicationExitReason
 * @note Only available with AppletType_SystemApplet on [11.0.0+], or on [15.0.0+] with AppletType_LibraryApplet.
 * @param[in] reason Reason
 */
Result appletSetLastApplicationExitReason(s32 reason);

///@}

///@name IGlobalStateController
///@{

/**
 * @brief Start the sequence for entering sleep-mode.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 * @param[in] flag Flag, official sw uses hard-coded value = true.
 */
Result appletStartSleepSequence(bool flag);

/**
 * @brief Start the system-shutdown sequence.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 */
Result appletStartShutdownSequence(void);

/**
 * @brief Start the system-reboot sequence.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 */
Result appletStartRebootSequence(void);

/**
 * @brief IsAutoPowerDownRequested. Uses an idle:sys cmd internally.
 * @note Only available with AppletType_SystemApplet on [7.0.0+], or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 * @param[out] out Output flag.
 */
Result appletIsAutoPowerDownRequested(bool *out);

/**
 * @brief LoadAndApplyIdlePolicySettings. Uses an idle:sys cmd internally.
 * @note Only available with AppletType_SystemApplet, or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 */
Result appletLoadAndApplyIdlePolicySettings(void);

/**
 * @brief NotifyCecSettingsChanged. Uses an omm cmd internally.
 * @note Only available with AppletType_SystemApplet on [2.0.0+], or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 */
Result appletNotifyCecSettingsChanged(void);

/**
 * @brief Sets the DefaultHomeButtonLongPressTime.
 * @note Only available with AppletType_SystemApplet on [3.0.0+], or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 * @param[in] val Input value.
 */
Result appletSetDefaultHomeButtonLongPressTime(s64 val);

/**
 * @brief UpdateDefaultDisplayResolution. Uses an omm cmd internally.
 * @note Only available with AppletType_SystemApplet on [3.0.0+], or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 */
Result appletUpdateDefaultDisplayResolution(void);

/**
 * @brief ShouldSleepOnBoot. Uses an omm cmd internally.
 * @note Only available with AppletType_SystemApplet on [3.0.0+], or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 * @param[out] out Output flag.
 */
Result appletShouldSleepOnBoot(bool *out);

/**
 * @brief Gets an Event which is signaled for HdcpAuthenticationFailed.
 * @note Only available with AppletType_SystemApplet on [4.0.0+], or on [15.0.0+] with AppletType_LibraryApplet/AppletType_OverlayApplet.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetHdcpAuthenticationFailedEvent(Event *out_event);

///@}

///@name IApplicationCreator
///@{

/**
 * @brief Creates an Application.
 * @note Only available with AppletType_SystemApplet.
 * @param[out] a \ref AppletApplication
 * @param[in] application_id ApplicationId.
 */
Result appletCreateApplication(AppletApplication *a, u64 application_id);

/**
 * @brief Pops a \ref AppletApplication for a requested Application launch.
 * @note Only available with AppletType_SystemApplet.
 * @param[out] a \ref AppletApplication
 */
Result appletPopLaunchRequestedApplication(AppletApplication *a);

/**
 * @brief Creates a SystemApplication.
 * @note Only available with AppletType_SystemApplet.
 * @param[out] a \ref AppletApplication
 * @param[in] system_application_id SystemApplicationId.
 */
Result appletCreateSystemApplication(AppletApplication *a, u64 system_application_id);

/**
 * @brief PopFloatingApplicationForDevelopment.
 * @note Only available with AppletType_SystemApplet. Should not be used if no FloatingApplication is available.
 * @param[out] a \ref AppletApplication
 */
Result appletPopFloatingApplicationForDevelopment(AppletApplication *a);

///@}

///@name IApplicationAccessor
///@{

/**
 * @brief Close an \ref AppletApplication.
 * @param a \ref AppletApplication
 */
void appletApplicationClose(AppletApplication *a);

/**
 * @brief Returns whether the AppletApplication object was initialized.
 * @param a \ref AppletApplication
 */
bool appletApplicationActive(AppletApplication *a);

/**
 * @brief Starts the Application.
 * @param a \ref AppletApplication
 */
Result appletApplicationStart(AppletApplication *a);

/**
 * @brief Requests the Application to exit.
 * @param a \ref AppletApplication
 */
Result appletApplicationRequestExit(AppletApplication *a);

/**
 * @brief Terminate the Application.
 * @param a \ref AppletApplication
 */
Result appletApplicationTerminate(AppletApplication *a);

/**
 * @brief Waits for the Application to exit.
 * @param a \ref AppletApplication
 */
void appletApplicationJoin(AppletApplication *a);

/**
 * @brief Waits on the Application StateChangedEvent with timeout=0, and returns whether it was successful.
 * @param a \ref AppletApplication
 */
bool appletApplicationCheckFinished(AppletApplication *a);

/**
 * @brief Gets the \ref AppletApplicationExitReason set by \ref appletApplicationJoin.
 * @param a \ref AppletApplication
 */
AppletApplicationExitReason appletApplicationGetExitReason(AppletApplication *a);

/**
 * @brief RequestForApplicationToGetForeground.
 * @param a \ref AppletApplication
 */
Result appletApplicationRequestForApplicationToGetForeground(AppletApplication *a);

/**
 * @brief TerminateAllLibraryApplets which were created by the Application.
 */
Result appletApplicationTerminateAllLibraryApplets(AppletApplication *a);

/**
 * @brief AreAnyLibraryAppletsLeft which were created by the Application.
 * @param a \ref AppletApplication
 * @param[out] out Output flag.
 */
Result appletApplicationAreAnyLibraryAppletsLeft(AppletApplication *a, bool *out);

/**
 * @brief Calls the same func as \ref appletHolderRequestExitOrTerminate with the output IAppletAccessor from the GetCurrentLibraryApplet cmd.
 * @param a \ref AppletApplication
 * @param[in] timeout Timeout in nanoseconds. UINT64_MAX for no timeout.
 */
Result appletApplicationRequestExitLibraryAppletOrTerminate(AppletApplication *a, u64 timeout);

/**
 * @brief Gets the ApplicationId for the Application.
 * @param a \ref AppletApplication
 * @param[out] application_id Output ApplicationId.
 */
Result appletApplicationGetApplicationId(AppletApplication *a, u64 *application_id);

/**
 * @brief Pushes a LaunchParameter AppletStorage to the Application.
 * @note This uses \ref appletStorageClose automatically.
 * @param a \ref AppletApplication
 * @param[in] kind \ref AppletLaunchParameterKind
 * @param[in] s Input storage.
 */
Result appletApplicationPushLaunchParameter(AppletApplication *a, AppletLaunchParameterKind kind, AppletStorage* s);

/**
 * @brief Gets the \ref NacpStruct for the Application.
 * @note Not usable when the \ref AppletApplication is for an AppletType_SystemApplication.
 * @param a \ref AppletApplication
 * @param[out] nacp \ref NacpStruct
 */
Result appletApplicationGetApplicationControlProperty(AppletApplication *a, NacpStruct *nacp);

/**
 * @brief Gets the \ref AppletApplicationLaunchProperty for the Application.
 * @note Only available on [2.0.0+]. Not usable when the \ref AppletApplication is for an AppletType_SystemApplication.
 * @param a \ref AppletApplication
 * @param[out] out \ref AppletApplicationLaunchProperty
 */
Result appletApplicationGetApplicationLaunchProperty(AppletApplication *a, AppletApplicationLaunchProperty *out);

/**
 * @brief Gets the \ref AppletApplicationLaunchRequestInfo for the Application.
 * @note Only available on [6.0.0+].
 * @param a \ref AppletApplication
 * @param[out] out \ref AppletApplicationLaunchRequestInfo
 */
Result appletApplicationGetApplicationLaunchRequestInfo(AppletApplication *a, AppletApplicationLaunchRequestInfo *out);

/**
 * @brief SetUsers for the Application.
 * @note Only available on [6.0.0+].
 * @param a \ref AppletApplication
 * @param[in] uids Input array of \ref AccountUid.
 * @param[in] count Size of the uids array in entries, must be <=ACC_USER_LIST_SIZE.
 * @param[in] flag When this flag is true, this just clears the users_available state flag to 0 and returns.
 */
Result appletApplicationSetUsers(AppletApplication *a, const AccountUid *uids, s32 count, bool flag);

/**
 * @brief CheckRightsEnvironmentAvailable.
 * @note Only available on [6.0.0+].
 * @param a \ref AppletApplication
 * @param[out] out Output flag.
 */
Result appletApplicationCheckRightsEnvironmentAvailable(AppletApplication *a, bool *out);

/**
 * @brief GetNsRightsEnvironmentHandle.
 * @note Only available on [6.0.0+].
 * @param a \ref AppletApplication
 * @param[out] handle Output NsRightsEnvironmentHandle.
 */
Result appletApplicationGetNsRightsEnvironmentHandle(AppletApplication *a, u64 *handle);

/**
 * @brief Gets an array of userIds for the Application DesirableUids.
 * @note Only available on [6.0.0+].
 * @note qlaunch only uses 1 userId with this.
 * @param a \ref AppletApplication
 * @param[out] uids Output array of \ref AccountUid.
 * @param[in] count Size of the uids array in entries, must be at least the size stored in state.
 * @param[out] total_out Total output entries.
 */
Result appletApplicationGetDesirableUids(AppletApplication *a, AccountUid *uids, s32 count, s32 *total_out);

/**
 * @brief ReportApplicationExitTimeout.
 * @note Only available on [6.0.0+].
 * @param a \ref AppletApplication
 */
Result appletApplicationReportApplicationExitTimeout(AppletApplication *a);

/**
 * @brief Sets the \ref AppletApplicationAttribute for the Application.
 * @note Only available on [8.0.0+].
 * @param a \ref AppletApplication
 * @param[in] attr \ref AppletApplicationAttribute
 */
Result appletApplicationSetApplicationAttribute(AppletApplication *a, const AppletApplicationAttribute *attr);

/**
 * @brief Gets whether the savedata specified by the input ApplicationId is accessible.
 * @note Only available on [8.0.0+].
 * @param a \ref AppletApplication
 * @param[in] application_id ApplicationId for the savedata.
 * @param[out] out Output flag.
 */
Result appletApplicationHasSaveDataAccessPermission(AppletApplication *a, u64 application_id, bool *out);

/**
 * @brief Creates a storage using the specified input then pushes it to the FriendInvitation StorageChannel.
 * @note The system will clear the StorageChannel before pushing the storage.
 * @note Only available on [9.0.0+].
 * @param a \ref AppletApplication
 * @param[in] uid \ref AccountUid
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 */
Result appletApplicationPushToFriendInvitationStorageChannel(AppletApplication *a, AccountUid uid, const void* buffer, u64 size);

/**
 * @brief Creates a storage using the specified input then pushes it to the Notification StorageChannel.
 * @note The system will clear the StorageChannel before pushing the storage.
 * @note Only available on [9.0.0+].
 * @param a \ref AppletApplication
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 */
Result appletApplicationPushToNotificationStorageChannel(AppletApplication *a, const void* buffer, u64 size);

/**
 * @brief RequestApplicationSoftReset
 * @note Only available on [10.0.0+].
 * @param a \ref AppletApplication
 */
Result appletApplicationRequestApplicationSoftReset(AppletApplication *a);

/**
 * @brief RestartApplicationTimer
 * @note Only available on [10.0.0+].
 * @param a \ref AppletApplication
 */
Result appletApplicationRestartApplicationTimer(AppletApplication *a);

///@}

///@name ILibraryAppletSelfAccessor
///@{

/**
 * @brief Pops a storage from current-LibraryApplet input.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] s Storage object.
 */
Result appletPopInData(AppletStorage *s);

/**
 * @brief Pushes a storage for current-LibraryApplet output.
 * @note Only available with AppletType_LibraryApplet.
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 */
Result appletPushOutData(AppletStorage *s);

/**
 * @brief Pops a storage from current-LibraryApplet Interactive input.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] s Storage object.
 */
Result appletPopInteractiveInData(AppletStorage *s);

/**
 * @brief Pushes a storage for current-LibraryApplet Interactive output.
 * @note Only available with AppletType_LibraryApplet.
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 */
Result appletPushInteractiveOutData(AppletStorage *s);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletPopInData where previously no storage was available, this event is automatically cleared by the system once the last storage is popped.
 * @note Only available with AppletType_LibraryApplet.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetPopInDataEvent(Event *out_event);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletPopInteractiveInData where previously no storage was available, this event is automatically cleared by the system once the last storage is popped.
 * @note Only available with AppletType_LibraryApplet.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetPopInteractiveInDataEvent(Event *out_event);

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
 * @brief CanUseApplicationCore
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] out Output flag.
 */
Result appletCanUseApplicationCore(bool *out);

/**
 * @brief Gets the \ref AppletIdentityInfo for the CallerApplet.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] info \ref AppletIdentityInfo
 */
Result appletGetCallerAppletIdentityInfo(AppletIdentityInfo *info);

/**
 * @brief Gets the \ref NacpStruct for the MainApplet.
 * @note Only available with AppletType_LibraryApplet on [2.0.0+].
 * @param[out] nacp \ref NacpStruct
 */
Result appletGetMainAppletApplicationControlProperty(NacpStruct *nacp);

/**
 * @brief Gets the NcmStorageId for the MainApplet.
 * @note Only available with AppletType_LibraryApplet on [2.0.0+].
 * @param[out] storageId \ref NcmStorageId
 */
Result appletGetMainAppletStorageId(NcmStorageId *storageId);

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

/**
 * @brief Gets the DesirableKeyboardLayout previously set by \ref appletSetDesirableKeyboardLayout. An error is returned when it's not set.
 * @note Only available with AppletType_LibraryApplet on [4.0.0+].
 * @param[out] layout Output \ref SetKeyboardLayout.
 */
Result appletGetDesirableKeyboardLayout(SetKeyboardLayout *layout);

/**
 * @brief Pops a storage from current-LibraryApplet Extra input.
 * @note Only available with AppletType_LibraryApplet.
 * @param[out] s Storage object.
 */
Result appletPopExtraStorage(AppletStorage *s);

/**
 * @brief Gets an Event which is signaled when a new storage is available with \ref appletPopExtraStorage where previously no storage was available, this event is automatically cleared by the system once the last storage is popped.
 * @note Only available with AppletType_LibraryApplet.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetPopExtraStorageEvent(Event *out_event);

/**
 * @brief Unpop a storage for current-LibraryApplet input.
 * @note Only available with AppletType_LibraryApplet.
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 */
Result appletUnpopInData(AppletStorage *s);

/**
 * @brief Unpop a storage for current-LibraryApplet Extra input.
 * @note Only available with AppletType_LibraryApplet.
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 */
Result appletUnpopExtraStorage(AppletStorage *s);

/**
 * @brief Gets the IndirectLayerProducerHandle.
 * @note Only available with AppletType_LibraryApplet on [2.0.0+].
 * @param[out] out Output IndirectLayerProducerHandle.
 */
Result appletGetIndirectLayerProducerHandle(u64 *out);

/**
 * @brief Gets the DesiredLanguage for the MainApplet.
 * @note Only available with AppletType_LibraryApplet on [4.0.0+].
 * @param[out] LanguageCode Output LanguageCode, see set.h.
 */
Result appletGetMainAppletApplicationDesiredLanguage(u64 *LanguageCode);

/**
 * @brief Gets the ApplicationId for the currently running Application.
 * @note Only available with AppletType_LibraryApplet on [8.0.0+].
 * @param[out] application_id Output ApplicationId, 0 when no Application is running.
 */
Result appletGetCurrentApplicationId(u64 *application_id);

/**
 * @brief Exits the current applet. Same as \ref appletHolderRequestExit except this is for the current applet.
 * @note Only available with AppletType_LibraryApplet on [6.0.0+].
 */
Result appletRequestExitToSelf(void);

/**
 * @brief CreateGameMovieTrimmer. Do not use this directly, use \ref grcTrimGameMovie instead.
 * @note Only available with AppletType_LibraryApplet on [4.0.0+].
 * @note See also \ref appletReserveResourceForMovieOperation and \ref appletUnreserveResourceForMovieOperation.
 * @param[out] srv_out Output Service for grc IGameMovieTrimmer.
 * @param[in] tmem TransferMemory
 */
Result appletCreateGameMovieTrimmer(Service* srv_out, TransferMemory *tmem);

/**
 * @brief ReserveResourceForMovieOperation. Must be used at some point prior to \ref appletCreateGameMovieTrimmer.
 * @note Only available with AppletType_LibraryApplet on [5.0.0+].
 */
Result appletReserveResourceForMovieOperation(void);

/**
 * @brief UnreserveResourceForMovieOperation. Must be used at some point after all finished with GameMovieTrimmer usage (\ref appletCreateGameMovieTrimmer).
 * @note Only available with AppletType_LibraryApplet on [5.0.0+].
 */
Result appletUnreserveResourceForMovieOperation(void);

/**
 * @brief Gets an array of userIds for the MainApplet AvailableUsers.
 * @note Only available with AppletType_LibraryApplet on [6.0.0+].
 * @param[out] uids Output array of \ref AccountUid.
 * @param[in] count Size of the uids array in entries, must be at least ACC_USER_LIST_SIZE.
 * @param[out] flag When true, this indicates that no users are available.
 * @param[out] total_out Total output entries. This is -1 when flag is true.
 */
Result appletGetMainAppletAvailableUsers(AccountUid *uids, s32 count, bool *flag, s32 *total_out);

/**
 * @brief SetApplicationMemoryReservation
 * @note Only available with AppletType_LibraryApplet on [10.0.0+].
 * @note An Application must be currently running.
 * @param[in] val Input value.
 */
Result appletSetApplicationMemoryReservation(u64 val);

/**
 * @brief ShouldSetGpuTimeSliceManually
 * @note Only available with AppletType_LibraryApplet on [10.0.0+].
 * @param[out] out Output flag.
 */
Result appletShouldSetGpuTimeSliceManually(bool *out);

///@}

///@name IOverlayFunctions: IFunctions for AppletType_OverlayApplet.
///@{

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
 * @brief Gets the ApplicationId for displaying the logo screen during application launch.
 * @note Only available with AppletType_OverlayApplet.
 * @param[out] application_id Output ApplicationId, 0 when no application is running.
 */
Result appletGetApplicationIdForLogo(u64 *application_id);

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
 * @brief Start the system-shutdown sequence. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_OverlayApplet on [6.0.0+].
 */
Result appletStartShutdownSequenceForOverlay(void);

/**
 * @brief Start the system-reboot sequence. This will enter an infinite-sleep-loop on success.
 * @note Only available with AppletType_OverlayApplet on [6.0.0+].
 */
Result appletStartRebootSequenceForOverlay(void);

/**
 * @brief SetHealthWarningShowingState
 * @note Only available with AppletType_OverlayApplet on [9.0.0+].
 * @param[in] flag Flag
 */
Result appletSetHealthWarningShowingState(bool flag);

/**
 * @brief IsHealthWarningRequired
 * @note Only available with AppletType_OverlayApplet on [10.0.0+].
 * @param[out] out Output flag.
 */
Result appletIsHealthWarningRequired(bool *out);

/**
 * @brief Enables HID input for the OverlayApplet, without disabling input for the foreground applet. Generally \ref appletBeginToWatchShortHomeButtonMessage / appletEndToWatchShortHomeButtonMessage should be used instead.
 * @note Only available with AppletType_OverlayApplet on [5.0.0+].
 */
Result appletBeginToObserveHidInputForDevelop(void);

///@}

///@name IAppletCommonFunctions
///@{

/**
 * @brief Reads the ThemeStorage for the current applet.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [7.0.0+].
 * @note offset(+size) must be <=0x400.
 * @param[out] buffer Output buffer data.
 * @param[in] size Size to read.
 * @param[in] offset Offset within the ThemeStorage.
 * @param[out] transfer_size Actual read size.
 */
Result appletReadThemeStorage(void* buffer, size_t size, u64 offset, u64 *transfer_size);

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
 * @brief This is similar to \ref appletPushToAppletBoundChannelForDebug (no DebugMode check), except the used channel is loaded from elsewhere and must be in the range 31-32.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [9.0.0+].
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 */
Result appletPushToAppletBoundChannel(AppletStorage *s);

/**
 * @brief This is similar to \ref appletTryPopFromAppletBoundChannelForDebug (no DebugMode check), except the used channel is loaded from elsewhere and must be in the range 31-32.
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [9.0.0+].
 * @param[out] s Storage object.
 */
Result appletTryPopFromAppletBoundChannel(AppletStorage *s);

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

/**
 * @brief IsHomeButtonShortPressedBlocked
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [10.0.0+].
 * @param[out] out Output flag.
 */
Result appletIsHomeButtonShortPressedBlocked(bool *out);

/**
 * @brief IsVrModeCurtainRequired
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [11.0.0+].
 * @param[out] out Output flag.
 */
Result appletIsVrModeCurtainRequired(bool *out);

/**
 * @brief SetCpuBoostRequestPriority
 * @note Only available with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [11.0.0+].
 * @param[in] priority Priority
 */
Result appletSetCpuBoostRequestPriority(s32 priority);

///@}

///@name IDebugFunctions
///@{

/**
 * @brief Open an \ref AppletApplication for the currently running Application.
 * @note Should not be used when no Application is running.
 * @note Only available on [1.0.0-9.2.0].
 * @param[out] a \ref AppletApplication
 */
Result appletOpenMainApplication(AppletApplication *a);

/**
 * @brief Perform SystemButtonPressing with the specified \ref AppletSystemButtonType.
 * @param[in] type \ref AppletSystemButtonType
 */
Result appletPerformSystemButtonPressing(AppletSystemButtonType type);

/**
 * @brief InvalidateTransitionLayer.
 */
Result appletInvalidateTransitionLayer(void);

/**
 * @brief Requests to launch the specified Application, with the specified users.
 * @note Only available on [6.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[in] uids Input array of \ref AccountUid.
 * @param[in] total_uids Total input uids, must be <=ACC_USER_LIST_SIZE.
 * @param[in] flag Whether to use the specified buffer to create a storage which will be pushed for ::AppletLaunchParameterKind_UserChannel.
 * @param[in] buffer Buffer containing the above storage data.
 * @param[in] size Size of the storage buffer.
 */
Result appletRequestLaunchApplicationWithUserAndArgumentForDebug(u64 application_id, const AccountUid *uids, s32 total_uids, bool flag, const void* buffer, size_t size);

/**
 * @brief Gets the \ref AppletResourceUsageInfo.
 * @note Only available on [6.0.0+].
 * @param[out] info \ref AppletResourceUsageInfo
 */
Result appletGetAppletResourceUsageInfo(AppletResourceUsageInfo *info);

/**
 * @brief The channel must match the value already stored in state when the state value is non-zero, otherwise an error is returned. When the state value is 0, the channel is written into state. Then the input storage is pushed to the StorageChannel.
 * @note Only available on [9.0.0+]. DebugMode must be enabled.
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 * @param[in] channel Channel.
 */
Result appletPushToAppletBoundChannelForDebug(AppletStorage *s, s32 channel);

/**
 * @brief The channel must not be 0 and must match the value previously saved by \ref appletPushToAppletBoundChannelForDebug, otherwise errors are returned. Then the output storage is popped from the StorageChannel.
 * @note Only available on [9.0.0+]. DebugMode must be enabled.
 * @param[out] s Storage object.
 * @param[in] channel Channel.
 */
Result appletTryPopFromAppletBoundChannelForDebug(AppletStorage *s, s32 channel);

/**
 * @brief Clears a StorageChannel, pushes the input storage there, and writes the ApplicationId into state.
 * @note Only available on [9.0.0+].
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 * @param[in] application_id ApplicationId
 */
Result appletAlarmSettingNotificationEnableAppEventReserve(AppletStorage *s, u64 application_id);

/**
 * @brief Clears the StorageChannel/saved-ApplicationId used by \ref appletAlarmSettingNotificationEnableAppEventReserve.
 * @note Only available on [9.0.0+].
 */
Result appletAlarmSettingNotificationDisableAppEventReserve(void);

/**
 * @brief Same as \ref appletApplicationPushToNotificationStorageChannel except this uses the MainApplication.
 * @note Only available on [9.0.0+].
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 */
Result appletAlarmSettingNotificationPushAppEventNotify(const void* buffer, u64 size);

/**
 * @brief Clears a StorageChannel, pushes the input storage there, and writes the ApplicationId into state.
 * @note Only available on [9.0.0+].
 * @note This uses \ref appletStorageClose automatically.
 * @param[in] s Storage object.
 * @param[in] application_id ApplicationId
 */
Result appletFriendInvitationSetApplicationParameter(AppletStorage *s, u64 application_id);

/**
 * @brief Clears the StorageChannel/saved-ApplicationId used by \ref appletFriendInvitationSetApplicationParameter.
 * @note Only available on [9.0.0+].
 */
Result appletFriendInvitationClearApplicationParameter(void);

/**
 * @brief Same as \ref appletApplicationPushToFriendInvitationStorageChannel except this uses the MainApplication.
 * @note Only available on [9.0.0+].
 * @param[in] uid \ref AccountUid
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 */
Result appletFriendInvitationPushApplicationParameter(AccountUid uid, const void* buffer, u64 size);

/**
 * @brief Creates a GeneralStorage with the specified Id and size.
 * @note Only available on [18.0.0+].
 * @param[in] id Id value, high-byte must be 0.
 * @param[in] size Storage size.
 */
Result appletCreateGeneralStorageForDebug(u64 id, u64 size);

/**
 * @brief Reads data from the buffer previously allocated by \ref appletCreateGeneralStorageForDebug.
 * @note Only available on [18.0.0+].
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size.
 * @param[in] id Id value which was previously used with \ref appletCreateGeneralStorageForDebug, high-byte must be 0.
 * @param[in] offset Offset in the storage.
 * @param[out] out_size Output size.
 */
Result appletReadGeneralStorageForDebug(void* buffer, size_t size, u64 id, u64 offset, u64 *out_size);

/**
 * @brief Writes data to the buffer previously allocated by \ref appletCreateGeneralStorageForDebug.
 * @note Only available on [18.0.0+].
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 * @param[in] id Id value which was previously used with \ref appletCreateGeneralStorageForDebug, high-byte must be 0.
 * @param[in] offset Offset in the storage.
 */
Result appletWriteGeneralStorageForDebug(const void* buffer, size_t size, u64 id, u64 offset);

///@}

///@name Common cmds
///@{

/**
 * @brief SetTerminateResult
 * @note Only available with AppletType_*Application. Or with AppletType_SystemApplet, AppletType_LibraryApplet, or AppletType_OverlayApplet, on [9.0.0+].
 * @param[in] res Result
 */
Result appletSetTerminateResult(Result res);

/**
 * @brief Gets the LaunchStorageInfo.
 * @note Only available with AppletType_*Application on [2.0.0+], or with AppletType_LibraryApplet on [9.0.0+].
 * @param[out] app_storageId Same as AppletApplicationLaunchProperty::app_storageId.
 * @param[out] update_storageId Same as AppletApplicationLaunchProperty::update_storageId.
 */
Result appletGetLaunchStorageInfoForDebug(NcmStorageId *app_storageId, NcmStorageId *update_storageId);

/**
 * @brief Gets an Event which is signaled for GpuErrorDetected.
 * @note Only available with AppletType_*Application on [8.0.0+], or with AppletType_LibraryApplet on [9.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @note Official sw waits on this Event from a seperate thread, triggering an abort when it's signaled.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result appletGetGpuErrorDetectedSystemEvent(Event *out_event);

/**
 * @brief Sets HandlingHomeButtonShortPressedEnabled.
 * @note Only available with AppletType_OverlayApplet on [8.0.0+]. Or with non-AppletType_OverlayApplet on [9.1.0+].
 * @param[in] flag Flag
 */
Result appletSetHandlingHomeButtonShortPressedEnabled(bool flag);

///@}

///@name State / other
///@{

/**
 * @brief Gets the cached \ref AppletInfo loaded during \ref appletInitialize. This will return NULL when the info is not initialized, due to not running as AppletType_LibraryApplet, or when any of the used cmds fail.
 * @note Only available with AppletType_LibraryApplet.
 */
AppletInfo *appletGetAppletInfo(void);

/**
 * @brief Gets the event associated with notification messages.
 */
Event *appletGetMessageEvent(void);

/**
 * @brief Gets a notification message, see \ref AppletMessage.
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
AppletOperationMode appletGetOperationMode(void);
ApmPerformanceMode appletGetPerformanceMode(void);
AppletFocusState appletGetFocusState(void);

/**
 * @brief Sets the current \ref AppletFocusHandlingMode.
 * @note Should only be called with AppletType_Application.
 */
Result appletSetFocusHandlingMode(AppletFocusHandlingMode mode);

///@}
