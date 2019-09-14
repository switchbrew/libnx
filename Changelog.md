# Changelog

## Version 2.5.0

#### system
* Corrected type of id0 in svcGetInfo.

#### filesystem
* Added fsExtendSaveDataFileSystem, fsOpenCustomStorageFileSystem, fsSetGlobalAccessLogMode, fsGetGlobalAccessLogMode.
* Added FsCustomStorageId enum.
* Fixed by-name RomFS mount lookups to actually compare the entire name.

#### graphics
* Added binder session argument to nwindowCreate (nwindowCreateFromLayer not affected).
* Binder code now supports domain objects.
* Fixed bug in nvAddressSpaceCreate.

#### input
* **Explicitly announce support for System/SystemExt layouts, which in turn fixes input on 9.0.0+. It is of utmost importance that all homebrew be recompiled as soon as possible in order to work properly on 9.0.0+.**
* **Fixed Hdls to work on 9.0.0+.**
  * Several Hdls structs were redefined in 9.0.0+ and libnx was updated to reflect the new struct definitions (this is a breaking change). The old structs are still available but they now have a `V7` suffix, and libnx transparently handles conversion to/from the new 9.x structs at runtime on older system versions; so the new structs are *always* used regardless of system version. List of structs affected:
    * HiddbgHdlsDeviceInfo(V7)
    * HiddbgHdlsState(V7)
    * HiddbgHdlsStateListEntry(V7)
    * HiddbgHdlsStateList(V7)
  * Added hiddbgGetUniquePadDeviceTypeSetInternal.
* Added hidGetNpadInterfaceType.
* Added HidNpadInterfaceType, HidDeviceTypeBits, HidDeviceType enums.
* Prevent AbstractedPad/VirtualPad commands from being used on 9.0.0+ since they were removed.
* Corrected several commands by internally calling hidControllerIDToOfficial.

#### applet
* Added AppletAttribute, AppletProcessLaunchReason, AppletInfo, AppletApplicationLaunchProperty, AppletApplicationLaunchRequestInfo, AppletResourceUsageInfo structs.
* Added AppletApplicationExitReason, AppletSystemButtonType, AppletProgramSpecifyKind enums.
* Renamed AppletNotificationMessage to AppletMessage.
* Renamed AppletLaunchParameterKind_Application to AppletLaunchParameterKind_UserChannel.
* Added appletGetServiceSession_* funcs.
* Added appletGetAppletInfo.
* Changed appletRequestToShutdown/appletRequestToReboot and appletStartShutdownSequenceForOverlay/appletStartRebootSequenceForOverlay on success to enter an infinite sleep loop.
  * This is also used with `_appletExitProcess` when the exit commands were successful, which is used during `__nx_applet_exit_mode` handling.
* Use OpenLibraryAppletProxy command on 3.0.0+ when running appletInitialize for AppletType_LibraryApplet.
* Added libappletArgsPop and libappletSetJumpFlag.

* **ILibraryAppletSelfAccessor**:
  * Added appletPopInData, appletPushOutData, appletPopInteractiveInData, appletPushInteractiveOutData, appletGetPopInDataEvent, appletGetPopInteractiveInDataEvent.
  * Added appletCanUseApplicationCore, appletGetMainAppletApplicationControlProperty, appletGetMainAppletStorageId, appletGetDesirableKeyboardLayout.
  * Added appletPopExtraStorage, appletGetPopExtraStorageEvent, appletUnpopInData, appletUnpopExtraStorage.
  * Added appletGetIndirectLayerProducerHandle, appletGetMainAppletApplicationDesiredLanguage, appletGetCurrentApplicationId.
  * Added appletCreateGameMovieTrimmer, appletReserveResourceForMovieOperation, appletUnreserveResourceForMovieOperation.
  * Added appletGetMainAppletAvailableUsers.
* **IProcessWindingController**:
  * Added appletPushContext, appletPopContext.
* **IDebugFunctions**:
  * Added appletOpenMainApplication, appletPerformSystemButtonPressing, appletInvalidateTransitionLayer, appletRequestLaunchApplicationWithUserAndArgumentForDebug, appletGetAppletResourceUsageInfo.
* **ILibraryAppletAccessor**:
  * Added appletHolderTerminate, appletHolderRequestExitOrTerminate.
* **IProcessWindingController**:
  * Added appletHolderJump.
* **IOverlayFunctions**:
  * Added appletBeginToObserveHidInputForDevelop.
* **IHomeMenuFunctions**:
  * Added appletPopRequestLaunchApplicationForDebug, appletLaunchDevMenu.
* **IApplicationCreator**:
  * Added support for AppletApplication.
  * Added appletCreateApplication, appletPopLaunchRequestedApplication, appletCreateSystemApplication, appletPopFloatingApplicationForDevelopment.
* **ILibraryAppletCreator**:
  * Added appletTerminateAllLibraryApplets, appletAreAnyLibraryAppletsLeft.
* **IApplicationFunctions**:
  * Added appletGetLaunchStorageInfoForDebug, appletRequestFlushGamePlayingMovieForDebug, appletExitAndRequestToShowThanksMessage.
  * Added appletExecuteProgram, appletJumpToSubApplicationProgramForDevelopment, appletRestartProgram, and appletGetPreviousProgramIndex.
  * Added appletCreateMovieMaker and appletPrepareForJit.

#### libapplets
* Added support for launching the Album applet via albumLa.

#### other services
* **Added GRC service support** (video recording, streaming and trimming).
* Changes to caps (capture service) wrappers:
  * Added support for the caps:u service.
  * Added CapsAlbumFileDateTime, CapsAlbumEntryId, CapsApplicationData, CapsUserIdList, CapsScreenShotAttributeForApplication, CapsScreenShotDecodeOption, CapsApplicationAlbumFileEntry, CapsLoadAlbumScreenShotImageOutputForApplication structs.
  * Added AlbumReportOption, CapsContentType enums.
  * Added capsGetShimLibraryVersion (not an actual IPC wrapper).
  * Added capsGetDefaultStartDateTime, capsGetDefaultEndDateTime, capsConvertApplicationAlbumFileEntryToApplicationAlbumEntry, capsConvertApplicationAlbumEntryToApplicationAlbumFileEntry helper functions.
  * Added capssuSaveScreenShotWithUserData, capssuSaveScreenShotWithUserIds, capssuSaveScreenShotEx1, capssuSaveScreenShotEx2.
  * Improved definition of capssuSaveScreenShot and capssuSaveScreenShotEx0.
  * Improved definition of CapsScreenShotAttribute, CapsApplicationAlbumEntry structs.
  * Changed caps:su wrapper to call SetShimLibraryVersion on 7.0.0+.
  * Renamed capsscCaptureScreenshot with capsscCaptureRawImageWithTimeout.
* Fixed lr RedirectApplication commands on 9.0.0+.
    * Renamed lrLrResolveLegalInformationPath to lrLrResolveApplicationLegalInformationPath.
    * Renamed lrLrRedirectLegalInformationPath to lrLrRedirectApplicationLegalInformationPath.
* Added missing fields to NacpStruct and other miscellaneous corrections.
* Fixed definition of setsysGetServiceSession.

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments have been made to enhance the user experience.

## Version 2.4.0

#### system
* **Added support for new Homebrew ABI keys**, including: UserIdStorage, HosVersion.
* TLS destructors now run after clearing the corresponding TLS value, as per the standard.

#### applet
* Added AppletIdentityInfo and LibAppletInfo structs.
* Added AppletId_application.
* Updated LibAppletMode enum.
* Added AppletSystemButtonType enum.
* Added AppletHookType_RequestToDisplay and AppletNotificationMessage_RequestToDisplay.
* Added appletHolderSetOutOfFocusApplicationSuspendingEnabled and appletHolderGetLibraryAppletInfo.
* Replaced appletHomeButtonReaderLockAccessorGetEvent with appletGetHomeButtonReaderLockAccessor.
* Added appletGetReaderLockAccessorEx, appletGetWriterLockAccessorEx, and appletGetHomeButtonWriterLockAccessor.
* Added support for AppletLockAccessor.
* **ISelfController**: Added appletSetScreenShotAppletIdentityInfo, appletSetControllerFirmwareUpdateSection, appletSetDesirableKeyboardLayout, appletIsSystemBufferSharingEnabled, appletGetSystemSharedLayerHandle, appletGetSystemSharedBufferHandle, appletSetHandlesRequestToDisplay, appletApproveToDisplay, appletOverrideAutoSleepTimeAndDimmingTime, appletSetIdleTimeDetectionExtension, appletGetIdleTimeDetectionExtension, appletSetInputDetectionSourceSet, appletReportUserIsActive, appletSetAutoSleepDisabled, appletIsAutoSleepDisabled, appletSetWirelessPriorityMode, appletGetProgramTotalActiveTime.
* **ILibraryAppletSelfAccessor**: Added appletGetMainAppletIdentityInfo, appletGetCallerAppletIdentityInfo, appletGetCallerAppletIdentityInfoStack, appletGetNextReturnDestinationAppletIdentityInfo, appletGetLibraryAppletInfo.
* **ICommonStateGetter**: Added appletGetCradleStatus, appletGetBootMode, appletRequestToAcquireSleepLock, appletReleaseSleepLock, appletReleaseSleepLockTransiently, appletGetCradleFwVersion, AppletTvPowerStateMatchingMode, appletSetLcdBacklightOffEnabled, appletIsInControllerFirmwareUpdateSection, appletGetDefaultDisplayResolution, appletGetDefaultDisplayResolutionChangeEvent, appletGetHdcpAuthenticationState, appletGetHdcpAuthenticationStateChangeEvent, appletSetTvPowerStateMatchingMode, appletGetApplicationIdByContentActionName, appletPerformSystemButtonPressingIfInFocus, appletSetPerformanceConfigurationChangedNotification, appletGetOperationModeSystemInfo.
* **IWindowController**: Added appletGetAppletResourceUserIdOfCallerApplet, appletSetAppletWindowVisibility, appletSetAppletGpuTimeSlice.
* **Added full support for IAudioController**.
* **IDisplayController**: Added appletUpdateLastForegroundCaptureImage, appletUpdateCallerAppletCaptureImage, appletGetLastForegroundCaptureImageEx, appletGetLastApplicationCaptureImageEx, appletGetCallerAppletCaptureImageEx, appletTakeScreenShotOfOwnLayer, appletCopyBetweenCaptureBuffers, appletClearCaptureBuffer, appletClearAppletTransitionBuffer, appletAcquireLastApplicationCaptureSharedBuffer, appletReleaseLastApplicationCaptureSharedBuffer, appletAcquireLastForegroundCaptureSharedBuffer, appletReleaseLastForegroundCaptureSharedBuffer, appletAcquireCallerAppletCaptureSharedBuffer, appletReleaseCallerAppletCaptureSharedBuffer, appletTakeScreenShotOfOwnLayerEx.
* **Added full support for IAppletCommonFunctions**.
* **IHomeMenuFunctions**: Added appletRequestToGetForeground, appletLockForeground, appletUnlockForeground, appletPopFromGeneralChannel, appletGetPopFromGeneralChannelEvent.
* **IGlobalStateController**: Added appletStartSleepSequence, appletStartShutdownSequence, appletStartRebootSequence, appletIsAutoPowerDownRequested, appletLoadAndApplyIdlePolicySettings, appletNotifyCecSettingsChanged, appletSetDefaultHomeButtonLongPressTime, appletUpdateDefaultDisplayResolution, appletShouldSleepOnBoot, appletGetHdcpAuthenticationFailedEvent.
* **IOverlayFunctions**: Added appletGetApplicationIdForLogo, appletSetGpuTimeSliceBoost, appletSetAutoSleepTimeAndDimmingTimeEnabled, appletTerminateApplicationAndSetReason, appletSetScreenShotPermissionGlobally, appletStartShutdownSequenceForOverlay, appletStartRebootSequenceForOverlay, appletSetHandlingHomeButtonShortPressedEnabled.

#### graphics
* Added viDestroyManagedLayer.
* Added ViPowerState_On_Deprecated to ViPowerState enum.
* Fixed bug in nvAddressSpaceModify.

#### filesystem
* Added fsOpenGameCardFileSystem.
* Added fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId, fsReadSaveDataFileSystemExtraData, fsWriteSaveDataFileSystemExtraData.
* Added FsSaveDataExtraData struct.
* Added FsGameCardPartiton, FsSaveDataFlags enums.

#### input
* Added hidGetSupportedNpadStyleSet, hidsysGetSupportedNpadStyleSetOfCallerApplet.

#### other services
* **Added UserIdStorage caching support to the account IPC wrappers**: the current account can be read using accountGetPreselectedUser.
* Added AlbumImageOrientation enum.
* Renamed accountGetActiveUser to accountGetLastOpenedUser.
* Corrected name of pdmqryGetServiceSession.
* Fixed NFC service IPC bugs.
* Updated PdmPlayEvent's unk_x8 union.

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments have been made to enhance the user experience.

## Version 2.3.0

#### system
* **Added xxGetServiceSession functions for most services** (those which already had GetSessionService were renamed to GetServiceSession).
* Added InfoType, SystemInfoType, TickCountInfo, InitialProcessIdRangeInfo and PhysicalMemoryInfo enums for syscalls.

#### applet
* **Added 8.0+ web applet functions**: webConfigSetMediaPlayerUi, webReplyGetMediaPlayerAutoClosedByCompletion.
* **Added 8.0+ software keyboard functions**: swkbdConfigSetUnkFlag, swkbdConfigSetTrigger, swkbdInlineSetChangedStringV2Callback, swkbdInlineSetMovedCursorV2Callback.
* Added swkbdInlineLaunchForLibraryApplet, swkbdInlineSetDecidedCancelCallback.
* **Added many new applet command wrappers**, some of which expose functions introduced in more recent system versions: appletQueryApplicationPlayStatisticsByUid, appletGetGpuErrorDetectedSystemEvent, appletGetDisplayVersion, appletBeginBlockingHomeButtonShortAndLongPressed, appletEndBlockingHomeButtonShortAndLongPressed, appletRequestToShutdown, appletRequestToReboot, appletInitializeApplicationCopyrightFrameBuffer, appletSetApplicationCopyrightImage, appletSetApplicationCopyrightVisibility, appletGetPseudoDeviceId, appletSetApplicationAlbumUserData, appletEnterFatalSection, appletLeaveFatalSection, appletSetRestartMessageEnabled, appletSetRequiresCaptureButtonShortPressedMessage, appletSetAlbumImageTakenNotificationEnabled.
* Added AppletScreenShotPermission enum (now used by appletSetScreenShotPermission).
* Added AppletNotificationMessage enum, which describes the currently known return values of appletGetMessage.
* Added AppletHookType_OnRestart, AppletHookType_OnCaptureButtonShortPressed and AppletHookType_OnAlbumImageTaken hook types.
* Renamed appletSetScreenShotImageOrientation to appletSetAlbumImageOrientation.
* Moved AppletApplicationPlayStatistics struct to PDM (now it's called PdmApplicationPlayStatistics).
* Fixed web applet arg passing code to actually use 6.0+ format on 6.0+.
* Fixed appletQueryApplicationPlayStatistics.

#### filesystem
* Added fsdevCreateFile, fsdevDeleteDirectoryRecursively, fsdevGetLastResult.
* Added fsOpenFileSystemWithPatch, fsOpenContentStorageFileSystem, fsGetRightsIdByPath, fsGetRightsIdAndKeyGenerationByPath, fsCreateSaveDataFileSystemBySystemSaveDataId, fsDeleteSaveDataFileSystemBySaveDataSpaceId, fsDisableAutoSaveDataCreation, fsCreate_SystemSaveDataWithOwner, fsCreate_SystemSaveData.
* Added fsFileOperateRange, fsStorageOperateRange.
* Added option parameter to fsFileRead, fsFileWrite.
* Added romfsMountFromCurrentProcess.
* Added FsBisStorageId, FsFileCreateFlags, FsReadOption, FsWriteOption and FsOperationId enums.
* Added FS_DIROPEN_NO_FILE_SIZE to FsDirectoryFlags enum.
* Added FsRangeInfo struct.
* Fixed romfs unmount corruption.
* Fixed IPC fail in fsFsCreateFile.

#### input
* **Added partial support for SevenSixAxisSensor**.
* **Added hid:dbg service support (virtual HID controllers)**.
* Added hid commands: hidGetControllerDeviceType, hidGetControllerFlags, hidGetControllerPowerInfo.
* Added hid:sys commands: hidsysGetUniquePadSerialNumber.
* Fixed bug in irsGetIrCameraHandle.
* Fixed IPC fail in hidsysSetNotificationLedPattern.

#### other services
* Added pm:bm service support.
* Added pdm:qry service support.
* Added i2c commands: i2csessionReceiveAuto, i2csessionExecuteCommandList.
* Added ncm commands: ncmContentMetaDatabaseGetAttributes.
* Added NcmContentMetaType and NcmContentMetaAttributa enums.
* Added pm:shell commands: pmshellBoostSystemThreadResourceLimit.
* Added set:sys commands: setsysGetDeviceNickname, setsysSetDeviceNickname.
* Added time commands: timeGetDeviceLocationName, timeSetDeviceLocationName, timeGetTotalLocationNameCount, timeLoadLocationNameList, timeLoadTimeZoneRule, timeToPosixTime, timeToPosixTimeWithMyRule, timeToCalendarTime.
* Added and corrected several /dev/nvhost-ctrl-gpu ioctls.
* Added nvGpuGetZcullInfo, nvGpuGetTpcMasks, nvGpuZbcGetActiveSlotMask, nvGpuZbcAddColor, nvGpuZbcAddDepth.
* Improved I2cDevice enum.
* Improved accountInitialize to internally call InitializeApplicationInfo.
* Changed binderInitSession to accept an arbitrary relay session handle.
* Renamed PdmAccountEvent::eventType to type.
* Corrected names of several spl functions: splSetBootReason, splGetBootReason.
* Corrected ro service initialization code.
* Corrected bug in internal USB IPC code.
* Corrected bug in binderInitSession.
* Corrected bug in parcelReadData.
* Corrected bug in viInitialize when using Manager/System.

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 2.2.0

#### system
* **Introduced hardware accelerated cryptography API**, including support for:
  * Hardware accelerated AES-128/AES-192/AES-256 with single-block ECB and multi-block CBC/CTR/XTS modes
  * Hardware accelerated SHA-1/SHA-256
  * Hardware accelerated AES-128-CMAC/AES-192-CMAC/AES-256-CMAC
  * Hardware accelerated HMAC-SHA1/HMAC-SHA256
* Added a custom MOD0 extension to make it easier to locate the got section.
* Added syscall: svcQueryProcessMemory.
* Improved smInitialize robustness during early system initialization in order to avoid race conditions when SM itself hasn't fully started up.
* **Fixed TLS slot issue on 8.0.0 - please recompile all your homebrew applications with this libnx release in order to fix crashes on 8.0.0.**

#### applet
* **Added support for VR mode, available on 6.0.0+**:
  * Added appletIsVrModeEnabled (3.0.0+) and appletSetVrModeEnabled.
* **Added support for CPU boost, available on 7.0.0+**:
  * Added appletSetCpuBoostMode and appletGetCurrentPerformanceConfiguration.
* **Added support for the light sensor, available on 3.0.0+**:
  * appletGetCurrentIlluminance, appletGetCurrentIlluminanceEx (5.0.0+) and appletIsIlluminanceAvailable.
* Added support for the pctlauth library applet.
* Added libappletStart and libappletLaunch.
* Updated swkbd support with new functionality present in 6.0.0+, and added accessor functions for the SwkbdConfig struct:
  * Added swkbdConfigSetType, swkbdConfigSetDicFlag, swkbdConfigSetKeySetDisableBitmask, swkbdConfigSetInitialCursorPos, swkbdConfigSetStringLenMax, swkbdConfigSetStringLenMaxExt, swkbdConfigSetPasswordFlag, swkbdConfigSetTextDrawType, swkbdConfigSetReturnButtonFlag, swkbdConfigSetBlurBackground and swkbdConfigSetTextGrouping.
  * Added swkbdInlineAppearEx, swkbdInlineSetCustomizedDictionaries and swkbdInlineUnsetCustomizedDictionaries.
  * Users are advised to use the new accessor functions and stop manipulating directly the contents of the SwkbdConfig struct.
* Updated web support with new functionality present in 6.0.0+:
  * Added webConfigSetMediaAutoPlay, webConfigSetMediaPlayerSpeedControl, webConfigAddAlbumEntryAndMediaData, webConfigSetBootFooterButtonVisible, webConfigSetOverrideWebAudioVolume and webConfigSetOverrideMediaAudioVolume.

#### filesystem
* Added romfsMountFromFsdev.
* Added fsdevTranslatePath.
* Updated FsSave/FsSaveDataInfo structures.
* Fixed leakage of `0x402` error codes (now they get converted to `EEXIST`).

#### hid
* **Added support for the HOME-button notification LED, available on 7.0.0+**:
  * Added NotificationLed struct for describing LED patterns.
  * Added hidsysSetNotificationLedPattern.
* Added hidsysGetUniquePadsFromNpad and hidsysGetUniquePadIds.

#### other services
* Added clkrst service wrappers.
* Added pctl service wrappers.
* Added ro:1 service wrappers.
* Added ldr:ro command: ldrRoLoadNrrEx.
* Improved pcv support for 8.0.0 changes:
  * Added PcvModuleId enum.
  * Added pcvGetModuleId for converting PcvModule to PcvModuleId.
  * Added checks to avoid calling pcv commands removed in 8.0.0 (use clkrst instead).
* Fixed LoaderModuleInfo struct.
* Fixed signature of roDmntGetModuleInfos and ldrDmntGetModuleInfos.
* Fixed signature of splCryptoCryptAesCtr.
* Removed `apm:p` service support in order to support 8.0.0.

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 2.1.0

#### system
* **Introduced support for POSIX threads (pthreads) and C++ `std::thread` APIs**, with the help of devkitA64 r13.
* **Introduced hosversion API**, which replaces the old kernelAbove*X* functions for nearly all use cases. By default HOS version is sourced from set:sys during app startup if available. **Make sure to call hosversionSet with appropriately sourced system version information if you are overriding `__appInit`**.
* Added support for TLS slots.
* Homebrew now embeds the module name at the beginning of rodata. This allows Atmosphère to display the proper name of homebrew executable modules in crash reports and in other locations.
* Added detectJitKernelPatch, detectIgnoreJitKernelPatch and detectKernelVersion.
* Corrected definition of svcSleepThread.
* Optimized implementation of the Barrier and RwLock synchronization primitives.

#### applet
* **Added support for SwkbdInline.**
* **Added support for the WifiWebAuth, Web, WebOffline, WebShare and WebLobby library applets**. Please note that using some of these applets requires Atmosphère 0.8.6 or higher.
* **Added support for the Error library applet.**
* Added appletHolderActive, appletHolderCheckFinished and appletHolderRequestExit.
* Added appletQueryApplicationPlayStatistics.
* Added appletRequestLaunchApplication and appletRequestLaunchApplicationForQuest.
* Added appletBeginToWatchShortHomeButtonMessage, appletEndToWatchShortHomeButtonMessage and appletHomeButtonReaderLockAccessorGetEvent.
* Added appletGetMessage and appletProcessMessage. appletMainLoop is now a wrapper for these two.
* Added libappletReadStorage and libappletPopOutData.
* Added libappletCreateWriteStorage (previously it was an internal function).

#### filesystem
* **Refactored romfs device to support multiple romfs mounts in a sane way**.
* Added fsOpenDataStorageByDataId.
* Added romfsMountFromDataArchive.

#### graphics
* **Legacy deprecated gfx API has been removed**.
* Fixed bug that would return unusable default NWindow dimensions on 1.x.
* Corrected mistakes in NvColorFormat enum.
* Added vi commands: viGetIndirectLayerImageMap, viGetIndirectLayerImageRequiredMemoryInfo.
* Fixed stray layer creation on 7.0.0+.

#### hid
* Introduced `touchPosition::id` to allow multi-touch support to be usable.
* Added hidMouseMultiRead.
* Added hidControllerIDToOfficial and hidControllerIDFromOfficial (previously they were internal functions).
* Added hid:sys service wrappers.
* Changed types for fields in MousePosition to s32.

#### network
* Added wlan:inf service wrappers.
* Added nifm commands: nifmIsWirelessCommunicationEnabled, nifmIsEthernetCommunicationEnabled, nifmIsAnyForegroundRequestAccepted, nifmPutToSleep, nifmWakeUp, nifmGetInternetConnectionStatus.
* Added nifm:a/nifm:s command: nifmSetWirelessCommunicationEnabled.
* Added nifmSetServiceType, which allows selecting the privilege level of the nifm service (nifm:u/nifm:s/nifm:a).
* Fixed definition of struct ifreq's ifr_flags/ifr_flagshigh fields.
* Fixed IPC bug in bsdRead.
* Corrected nxlinkStdio to return the socket fd instead of zero on success, allowing for it to be closed later on.

#### other services
* Added caps:sc/caps:su service wrappers.
* Added nfp:user service wrappers.
* Added lbl commands: lblSetCurrentBrightnessSetting, lblGetCurrentBrightnessSetting, lblEnableAutoBrightnessControl, lblDisableAutoBrightnessControl, lblIsAutoBrightnessControlEnabled.
* Added pmdmntGetServiceSession for retrieving the pm:dmnt session.
* Renamed usbDsEndpoint_StallCtrl to usbDsEndpoint_Stall.

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 2.0.0

#### system
* **Introduced multi-wait infrastructure, allowing for user mode synchronization primitives. Users are advised to stop using svcWaitSynchronization(Single) manually and move to the new wait API.**
* **Added UEvent and UTimer user mode synchronization primitives.**
* **Added full list of KernelError result codes, as well as a KERNELRESULT macro.**
* Added eventActive.
* Added tmemCreateFromMemory.
* Added syscall: svcTerminateDebugProcess.
* Corrected syscalls: svcMapPhysicalMemory, svcUnmapPhysicalMemory.
* Fixed bug in ipcParseDomainResponse.
* Fixed bug in ipcPrepareHeaderForDomain.
* Added serviceSendObject.

#### graphics
* **Major refactor and redesign of the entire libnx graphics stack.**
  * **Introduced NWindow (Native Window) API**, allowing for direct management of a renderable surface. It is possible to create a NWindow out of a ViLayer (or other sources of an IGBP binder object); or use nwindowGetDefault to retrieve the default native window.
  * **Introduced Framebuffer API** (used for creating and managing a software rendered framebuffer on a NWindow).
  * **Deprecated the old gfx API, scheduled for removal in the next libnx release**. Users are advised to move to the new NWindow API (and Framebuffer if applicable). Please see switch-examples for more information on how to use this new API.
  * **The default software rendered console backend now uses NWindow and Framebuffer** instead of the old gfx API. Therefore, it is now **mandatory** to use consoleUpdate/consoleExit.
  * Optimized software console scrolling; now using 128-bit copies and RGB565 framebuffer format (which requires 50% less memory bandwidth).
  * Completely redesigned Nvidia ioctl wrapper objects, now more closely matching official logic.
  * Miscellaneous fixes in vi, parcel, IGBP, etc code.
* Added vi commands: viSetContentVisibility, viGetDisplayLogicalResolution, viSetDisplayMagnification, viSetDisplayPowerState, viSetDisplayAlpha, viGetDisplayMinimumZ, viGetDisplayMaximumZ, viSetLayerSize, viSetLayerZ, viSetLayerPosition.
* Improved ViScalingMode enum.

#### applet
* **Introduced libapplet (library applet) launching support**.
* **Added swkbd (software keyboard) libapplet wrapper**.
* Added applet commands: appletPopLaunchParameter, appletPushToGeneralChannel, appletSetTerminateResult, appletSetMediaPlaybackState.
* Added account applet wrapper: accountGetPreselectedUser.
* Added libappletRequestHomeMenu, libappletRequestJumpToSystemUpdate.
* Added AppletStorage object.
* Added AppletHolder object.
* Added LibAppletArgs object.
* Corrected and added missing AppletFocusState and AppletFocusHandlingMode enum values.
* Now using domains for am services.

#### usb
* Added usb:hs service wrapper.
* Now using domains for usb:ds.
* Miscellaneous fixes and refactoring.

#### other services
* Added fs command: fsIsExFatSupported.
* Added hid command: hidAcquireNpadStyleSetUpdateEventHandle.
* Added ldr:ro and ro:dmnt service wrappers.
* Added ns:dev commands: nsdevLaunchProgram, nsdevGetShellEvent, nsdevGetShellEventInfo, nsdevTerminateApplication, nsdevPrepareLaunchProgramFromHost, nsdevLaunchApplication, nsdevLaunchApplicationWithStorageId, nsdevIsSystemMemoryResourceLimitBoosted, nsdevGetRunningApplicationProcessId, nsdevSetCurrentApplicationRightsEnvironmentCanBeActive.
* Added pm:dmnt commands: pmdmntGetDebugProcesses, pmdmntDisableDebug.
* Added pm:shell commands: pmshellTerminateProcessByProcessId, pmshellGetProcessEvent, pmshellGetProcessEventInfo, pmshellFinalizeDeadProcess, pmshellClearProcessExceptionOccurred, pmshellNotifyBootFinished, pmshellBoostSystemMemoryResourceLimit.
* Renamed ldrDmntGetNsoInfos to ldrDmntGetModuleInfos.
* Changed psm wrapper to dynamically open and close IPC sessions instead of leaving one open at all times. Introduced PsmSession object, used to manage this session.
* Fixed IPC bug in splSetConfig.

#### miscellaneous
* Added sys/poll.h as an alias for poll.h.
* Fixed compatibility with C99.
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.6.0

#### system
* **Added support for userland exception handling**.

#### filesystem
* **Added support for retrieving file timestamps on 3.0.0+**.
* Added fsdevSetArchiveBit, for setting the archive bit on a directory.
* Added fs commands: fsFsQueryEntry, fsFsSetArchiveBit, fsDeviceOperatorIsGameCardInserted, fsDeviceOperatorGetGameCardHandle, fsDeviceOperatorGetGameCardAttribute.

#### audio
* **Added hwopus service wrapper**.
* **Added audren support for system versions prior to 3.0** by automatically using the latest available revision number for a given version (1.0.0-4.0.0+).
* Added auddev (IAudioDevice) service wrapper.

#### hid
* **Added support for SL/SR buttons on right JoyCons**.
* Added HidJoyHoldType enum.
* Changed hid to use hidSetNpadJoyHoldType during init/exit.

#### other services
* Added psc (psc:m) service wrapper.
* Added spsm service wrapper.
* Added pcv service wrapper.
* Added lbl service wrapper.
* Added i2c service wrapper.
* Added gpio service wrapper.
* Added bpc commands: bpcGetSleepButtonState.
* Added setsys commands: setsysBindFatalDirtyFlagEvent, setsysGetFatalDirtyFlags.
* Added fatal commands: fatalWithContext.
* Added PsmBatteryVoltageState enum.
* Added SetSysFlag_RequiresRunRepairTimeReviser.
* Added appletSetFocusHandlingMode (previously it existed but it was not exposed).

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.5.0

#### system
* **Improved service IPC support for domains**.
* Added RandomSeed HBABI key handling, which fixes broken/identical random number generation output when launching homebrew NROs multiple times with nx-hbloader.
* Added Barrier synchronization primitive.
* Added threadGetCurHandle.
* Fixed several C11 thread support standard compliance issues.
* Fixed virtmem handling on 1.x.
* Fixed ProcessState enumeration names and descriptions.
* Fixed bug in eventCreate.
* Fixed bug in jitCreate.

#### services
* fsp-srv and fsp-ldr services:
  * **Now using domains for IPC**, which solves problems related to having a large number of filesystem resources open.
  * Added fsdevGetDeviceFileSystem.
* time services:
  * Added timezone support.
  * The user system clock is now used by default instead of the network clock.
  * The user system clock is now also used as fallback if the specified system clock override (__nx_time_type) is not available.
* applet services:
  * **Added support for running with nx-hbloader as an Application**.
  * **Added support for enabling video recording** when running as an Application, although it needs a specific flag set in the host title's nacp to be accessible.
  * Added appletLockExit/appletUnlockExit, which can be used to ensure that user processes get a chance to finish before the application is closed by the system.
  * Added __nx_applet_exit_mode, used to control the application's behavior on close (including self-exit support).
  * Added AppletHookType_OnExitRequest.
  * Fixed issues related to running as an Application.
  * Minor internal refactor to use Event objects instead of raw handles.
* hid (input) services:
  * **Added SixAxisSensor support**.
  * Vibration partially fixed, although it needs disable/enable in system settings for it to be effective.
  * Added hidSetSupportedNpadStyleSet, hidSetSupportedNpadIdType, hidSetNpadJoyHoldType, hidGetControllerType, hidGetControllerColors, hidIsControllerConnected.
* USB services:
  * **Major refactor which adds support for 5.x+ systems**.
  * Added usbCommsSetErrorHandling, which now disables USB fatal errors by default.
* audin/audout services: minor internal refactor to use Event objects instead of raw handles.
* Added pm:shell command: pmshellGetApplicationPid.
* Added set:sys command: setsysGetFirmwareVersion.
* Added psm commands: psmGetBatteryVoltageState, psmBindStateChangeEvent, psmWaitStateChangeEvent, psmUnbindStateChangeEvent.
* fatal services: on systems prior to 3.x, FatalType_ErrorReportAndErrorScreen is used again due to FatalType_ErrorScreen not existing. **Beware of fatal errors on 1.x and 2.x** since they now create reports again (instead of silently hanging the system).

#### graphics
* **Refactored console device**:
  * The console now has an interface that can be used to override the default software rendering backend with a custom implementation (including GPU rendering).
  * The software renderer now takes care of calling gfxInitDefault, gfxSwapBuffers, gfxExit. gfx functions should not be called explicitly by console users.
  * Added consoleUpdate and consoleExit.
  * Old code is still compatible, although it is **strongly advised** to update it to use the new functions instead of calling gfx functions directly.
* Fixed NvFence regression on 1.x.
* Increased default nvservices transfermem heap size to 8 MB, which allows for more complex GPU homebrew to run.
* Changed nvBufferCreate to support different settings for cpu/gpu cacheability.

#### network
* Added nifm handling to socketInitialize/Exit, which makes gethostid/gethostname work by default.
* Added missing declarations to netinet/in.h.
* Changed nifm to use IPC domains.
* Optimized select/poll to avoid using malloc when possible.
* Fixed poll to accept -1 fds.

#### miscellaneous
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.4.1

* Restored compatibility with C++11.
* Added arm/thread_context.h, containing definitions for the ThreadContext structure.
* Added new syscalls: svcGetDebugThreadContext, svcSetDebugThreadContext, svcGetThreadContext3.
* Fixed signature of svcContinueDebugEvent, which changed in 3.0. The old signature is available as svcLegacyContinueDebugEvent.
* Added threadDumpContext.
* Added ipcCloneSession.
* Added gfxAppendFence.
* psm services: added psmGetChargerType.
* pm:dmnt services: fixed for 5.0+.
* nv services: added nvIoctl2 & handling for a separate cloned session, matching official software.
* Nvidia ioctl wrappers: added nvioctlNvhostAsGpu_UnmapBuffer, nvioctlNvmap_Free, nvioctlChannel_KickoffPb
* Further changes, fixes and improvements to the experimental Nvidia wrapper objects, which are used by the ported mesa/nouveau-based OpenGL stack.
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.4.0

#### system
* **Added support for C11 threads**, which are preemptively multitasked and load balanced across cores.
* **Added Event object**, which wraps kernel revent/wevent handles with optional autoclear.
* **Changed CondVar interface** to have the mutex be passed to condvarWait* instead of condvarInit, which is consistent with both the concept of a condition variable and with other common threading APIs.
* Added armGetSystemTick (which supersedes svcGetSystemTick), and armGetSystemTickFreq.
* Added rwlockInit.
* Added kernelAbove600.
* Added system calls: svcGetThreadCoreMask, svcSetThreadCoreMask.
* Added MOD0 header to binaries compiled with libnx.
* Fixed semaphoreTryWait.
* Fixed a memory leak in tmemCreate.

#### services
* Added bpc service (used for rebooting and shutting down the console).
* Added psm service (needed to get the battery status).
* Added ns commands: nsListApplicationRecord, nsListApplicationContentMetaStatus.
* Minor enhancements to applet service:
  * Added missing AppletType_SystemApplet initialization in applet code.
  * Added appletBeginBlockingHomeButton and appletEndBlockingHomeButton.
* The Event object is now used to return system events from service wrappers when possible, also providing the correct autoclear mode.
* Corrected fsOpenFileSystem and fsOpenFileSystemWithId.
* Corrected a bug in hidInitializeVibrationDevices.
* Corrected a bug in socket error conversion.
* Fixed nifm not initializing properly for < 3.0.0.
* Service manager (sm) session now closes properly.
* hid, irs, vi and nv services now acquire a reference to applet services.

#### audio
* **Added audren:u** service wrapper (presently requiring 3.0.0+, will be addressed in a future update).
* **Added AudioDriver** wrapper around audren, providing a higher level interface that can be used to mix and play sounds.

#### graphics
* **Major rewrite and refactoring work in the gfx wrapper** which brings reliability and usability improvements:
  * **Removed GfxMode_TiledSingle** mode due to it causing problems and potential (temporary) hardware damage.
  * **The default transform behavior no longer vertically flips the framebuffer**.
  * **Removed gfxSetDrawFlip** since it's no longer needed thanks to the change in the default transform behavior.
  * **It is not necessary to call gfxWaitForVsync in most situations** because gfxSwapBuffers already implicitly synchronizes with the display (this is mandated by the Android surface compositor and buffer producer system).
  * **Dequeue fatal errors should be solved**.
  * Simplified and streamlined logic.
  * nvgfx stripped down to the minimum that is actually necessary to allocate framebuffers.
  * Binder logic now more closely matches both Android code and official software.
  * Proper fence and event wait code is now used.
* **Console code no longer performs a forced flush/swap/vblank wait when printing a newline** due to performance reasons. Users of the console device must make sure that gfxFlushBuffers and gfxSwapBuffers are periodically called, preferably in the main loop of the application.
* **Added experimental wrapper objects for the Nvidia driver**, needed in order to use the GPU. These wrappers are still in RE phase and will be subject to change in a future release.
* **Major redesign of the VI service wrapper** that allows future users to use VI directly to create a display layer.
* Binder services & buffer producer wrappers were enhanced and redesigned.
  * Binder now holds less state and always uses the VI binder relay service session.
  * Added Module_LibnxBinder error codes.
  * IGraphicBufferProducer binder service wrappers now have the `bq` prefix and explicitly accept a Binder object.
  * Added bqCancelBuffer.
  * bqGraphicBufferInit was renamed to bqSetPreallocatedBuffer.
* Enhancements and additions to nvidia ioctl wrappers.
* Added definitions for some more Android enumerations.

#### miscellaneous
* The `ALIGN` macro was removed in favor of the C11/C++11 alignas attribute.
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.3.2

* **Fixed critical IPC bug in fatalWithType that rendered the type parameter ignored (necessary for error report creation prevention)**.

## Version 1.3.1

* **Fixed regression in cwd handling**.
* Added fs commands: fsOpenBisStorage, fsOpenBisFilesystem, fsOpenFileSystem(WithId), fsStorageWrite/Flush/GetSize/SetSize, fsFsCleanDirectoryRecursively.
* Added ncm commands: ncmContentMetaDatabaseList/DisableForcibly, ncmContentStorageXYZ family of functions.
* HID service now works in sysmodules.
* Fixed errors in ncmContentMetaDatabaseListApplication and ncmContentMetaDatabaseSet/Get.
* Fixed data races in kernel version detection functions.
* Fixed return-from-main to honor atexit handlers.
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.3.0

#### system
* **Fixed critical bug in mutexUnlock**.
* **Expanded support for standard time functions**, including gettimeofday, clock functions and nanosleep.
* Added Semaphore synchronization primitive.
* Added system calls: svcGetDebugThreadParam, svcGetThreadList.
* Improved IpcParsedCommand support for domain message information.

#### services
* **fatalSimple no longer creates an error report that gets uploaded to Nintendo's servers** - also added fatalWithType.
* Added ncm service.
* Added nifm service.
* Added ns:dev service.
* Added ns:vm service.
* Added account commands: accountGetUserCount, accountListAllUsers.
* Added ns commands: nsGetTotalSpaceSize, nsGetFreeSpaceSize.
* Added pm:shell commands: pmshellTerminateProcessByTitleId.
* Added set:sys commands: setsysGetFlag, setsysSetFlag, setsysGetSettingsItemValueSize, setsysGetSettingsItemValue, setsysGetColorSetId, setsysSetColorSetId.

#### sockets
* **Fixed errno** - now properly translating Horizon BSD errno to newlib errno values.
* Fixed bugs in inet_pton4, ioctl, socketDeserializeAddrInfo.
* Fixed _IOC macro.
* Fixed netdb.h by readding socklen_t definition.
* Implemented gethostid and gethostname.

#### miscellaneous
* Added overridable userAppInit and userAppExit functions that add to instead of replacing __appInit and __appExit.
* Now embedding GNU build id in compiled binaries, used by creport crash dumps (such as those generated by Atmosphère's creport implementation).
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.2.1

* Added hidMergeSingleJoyAsDualJoy.
* Added setGetSerialNumber.
* Renamed the JIT syscalls to match their official names.
* Fixed bug in nvgfxEventWait that prevented the graphics subsystem from working on certain conditions.
* Fixed JIT support on 5.0.0+ by falling back on JitType_CodeMemory.
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.2.0

#### filesystem
* Revise fsdev initialization
  * Removed fsdevInit/Exit
  * Now automatically initializing fsdev state on first mount
  * Added fsdevMountSdmc (replaces fsdevInit)
  * Added fsdevUnmountAll (replaces fsdevExit)
* Add FS commands for SD card state change detection.
* Added mounting for SystemSaveData.
* Use Service for all fs sessions. This fixes an issue with savedata commit.
* Implemented FsSaveDataIterator (aka ISaveDataInfoReader).
* Changed ContentStorageId in FsSave to SaveDataType.
* Added FsStorageId.
* Added enums FsSaveDataSpaceId and FsSaveDataType.
* Removed FS_MOUNTSAVEDATA_INVAL_DEFAULT/FS_MOUNTSYSTEMSAVEDATA_INVAL_DEFAULT.

####  hid updates
* Added hidGetHandheldMode().
* Added hidSetNpadJoyAssignmentModeSingleByDefault()
* hidSetNpadJoyAssignmentModeDual().
* Check serviceIsActive() in hidExit().
* Use hidSetNpadJoyAssignmentModeDual() for all controllers during hidInitialize()/hidExit().
* Full hid vibration support for pre-4.0.0 hid commands.

#### network
* add support for finding nxlink host
* improve bsdSockets support
* added nxlink stdio redirection
* Fix IN6_IS_ADDR macros in netinet/in.h (#68)

#### system
* Added support for Services: fsp-ldr, fsp-pr, lr, csrng, spl, pm:info, sm:m, pl, ns, all Loader services.
* IPC: Improve information available in IpcParsedCommand
* Added system calls: svcCreatePort, svcConnectToPort, svcUnmapProcessMemory, svcGetSystemInfo, svcSetThreadPriority, svcGetCurrentProcessorNumber, svcSignalEvent, svcSendSyncRequestWithUserBuffer, svcSendAsyncRequestWithUserBuffer, svcGetThreadId, svcReplyAndReceiveWithUserBuffer, svcCreateEvent, svcReadWriteRegister, svcCreateInterruptEvent, svcMapDeviceAddressSpaceByForce, svcTerminateProcess, svcMap/UnmapPhysicalMemory(Unsafe), svcSetUnsafeLimit, svcGetProcessInfo, svcGetResourceLimitLimitValue, svcGetResourceLimitCurrentValue, svcGetProcessInfo, svcCreateResourceLimit, svcSetResourceLimitLimitValue.
* IPC improvements and fixes.
* Added envGetLastLoadResult().

#### improve usbComms support
* Allow using multiple interfaces.
* Added usbComms Ex funcs.
* Use RwLock.

#### buildsystem
* Add rules for building .npdm, .nsp and .kip.
  * Now building .npdm from .json if specified
  * .pfs0 now embeds previously built npdm if available
  * .nsp accepted as an alternative file extension to .pfs0
  * .kip can be built from elf and json descriptor (subset of .npdm)
* Added impl for accountProfile\*. If the inital smGetService fails, attempt to use 'acc:u0'.
* Added nsGetApplicationControlData. Imported nacp.h from nx-hbmenu with adjustments.
* Add ipcAddSendSmart, ipcAddRecvSmart, use where applicable
* Audio input implementation and audio output fixes.
* add portlibs bin folder to path

#### miscellaneous
* Detect 5.0.0 properly.
* Added pmshell init/exit and pmshellLaunchProcess.
* Introduce atomics
  * atomicIncrement32
  * atomicDecrement32
  * atomicIncrement64
  * atomicDecrement64
* Added nacpGetLanguageEntry and SetLanguage_Total.
* [irs] Name image transfer config variables
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

## Version 1.1.0

* Fixed a race condition in HID causing sporadic incorrect key-releases when using hidKeysHeld().
* Unix socket API is now supported.
* Time support, currently only UTC.
* Added hidMouseRead().
* Added settings-services support.
* Added gfxSetDrawFlip() and gfxConfigureTransform().
* Proper (libnx-side) RomFS support. Initial fsStorage support / other fs(dev) changes.
* The console font is now 16x16.
* Fixed args parsing with quotes.
* Various audio adjustments + added audoutWaitPlayFinish().
* More irs (irsensor) support.
* Added usleep().
* General system stability improvements to enhance the user's experience.
