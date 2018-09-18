# Changelog

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
