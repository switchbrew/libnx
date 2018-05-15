# Changelog

## Version 1.2.0
* Revise fsdev initialization
  * Removed fsdevInit/Exit
  * Now automatically initializing fsdev state on first mount
  * Added fsdevMountSdmc (replaces fsdevInit)
  * Added fsdevUnmountAll (replaces fsdevExit)
* improve usbComms support
  * Allow using multiple interfaces.
  * Added usbComms Ex funcs.
  * Use RwLock.
* hid updates
  * Change the HidControllerLayoutType param to HidControllerType.
  * Copy HidControllerHeader for each controller into hid state.
  * Added hidGetHandheldMode().
  * Added disabled hidGetControllerType().
  * Added hidSetNpadJoyAssignmentModeSingleByDefault()
  * Added hidSetNpadJoyAssignmentModeDual().
* Added support for Services: fsp-ldr, fsp-pr, lr, csrng, spl, all Loader services. 
* Full hid vibration support for pre-4.0.0 hid commands.
* Replace g_gfxPixelFormat/gfxSetPixelFormat with constant since changing this value has no effect.
* Added system calls: svcCreatePort, svcConnectToPort, svcUnmapProcessMemory, svcGetSystemInfo, svcSetThreadPriority, svcGetCurrentProcessorNumber, svcSignalEvent, svcSendSyncRequestWithUserBuffer, svcSendAsyncRequestWithUserBuffer, svcGetThreadId, svcReplyAndReceiveWithUserBuffer, svcCreateEvent, svcReadWriteRegister, svcCreateInterruptEvent, svcMapDeviceAddressSpaceByForce, svcTerminateProcess, svcMap/UnmapPhysicalMemory(Unsafe), svcSetUnsafeLimit, svcGetProcessInfo
* Added envGetLastLoadResult().
* Audio input implementation and fixes
* Add rules for building .npdm, .nsp and .kip.
  * Now building .npdm from .json if specified
  * .pfs0 now embeds previously built npdm if available
  * .nsp accepted as an alternative file extension to .pfs0
  * .kip can be built from elf and json descriptor (subset of .npdm)
* add support for finding nxlink host
* improve bsdSockets support
* Added mounting for SystemSaveData.
* Further improvements to overall system stability and other minor adjustments to enhance the user experience.

--- stuff to organise ---

    Added pm:info and GetTitleId
    Implement sm:m.
    Add ipcAddSendSmart, ipcAddRecvSmart, use where applicable
    Add FS commands for SD card state change detection.
    Added plGetSharedFontByType().
    Changed nsApplicationControlData to NsApplicationControlData.
    Added ns impl, and added nsGetApplicationControlData. Imported nacp.h from nx-hbmenu with adjustments.
    Added FsStorageId.
    Added pmshell init/exit and pmshellLaunchProcess.
    Fix IN6_IS_ADDR macros in netinet/in.h (#68)
    Changed ContentStorageId in FsSave to SaveDataType.
    Added enums FsSaveDataSpaceId and FsSaveDataType.
    Removed FS_MOUNTSAVEDATA_INVAL_DEFAULT/FS_MOUNTSYSTEMSAVEDATA_INVAL_DEFAULT.
 
 
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
