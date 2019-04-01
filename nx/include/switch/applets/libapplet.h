/**
 * @file libapplet.h
 * @brief LibraryApplet wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

/// CommonArguments
typedef struct {
    u32 CommonArgs_version;
    u32 CommonArgs_size;

    u32 LaVersion;           ///< LibraryApplet API version
    s32 ExpectedThemeColor;  ///< Set to the output from \ref appletGetThemeColorType by \ref libappletArgsCreate.
    u8  PlayStartupSound;    ///< bool flag, default is false.
    u8  pad[7];
    u64 tick;
} LibAppletArgs;

/**
 * @brief Creates a LibAppletArgs struct.
 * @param a LibAppletArgs struct.
 * @param version LaVersion for \ref LibAppletArgs.
 */
void libappletArgsCreate(LibAppletArgs* a, u32 version);

/**
 * @brief Sets the PlayStartupSound field in \ref LibAppletArgs.
 * @param a LibAppletArgs struct.
 * @param flag Value for \ref LibAppletArgs PlayStartupSound.
 */
void libappletArgsSetPlayStartupSound(LibAppletArgs* a, bool flag);

/**
 * @brief Creates an AppletStorage with the specified size and writes the buffer contents to that storage at offset 0.
 * @param[out] s Storage object.
 * @param buffer Input buffer.
 * @param size Size to write.
 */
Result libappletCreateWriteStorage(AppletStorage* s, const void* buffer, size_t size);

/**
 * @brief Reads data from offset 0 from the specified storage into the buffer. If the storage-size is smaller than the size param, the storage-size is used instead.
 * @param s Storage object.
 * @param buffer Output buffer.
 * @param size Size to read.
 * @param transfer_size Optional output size field for the actual size used for the read, can be NULL.
 */
Result libappletReadStorage(AppletStorage* s, void* buffer, size_t size, size_t *transfer_size);

/**
 * @brief Sets the tick field in LibAppletArgs, then creates a storage with it which is pushed to the AppletHolder via \ref appletHolderPushInData.
 * @param a LibAppletArgs struct.
 * @param h AppletHolder object.
 */
Result libappletArgsPush(LibAppletArgs* a, AppletHolder *h);

/**
 * @brief Creates a storage using the input buffer which is pushed to the AppletHolder via \ref appletHolderPushInData.
 * @param h AppletHolder object.
 * @param buffer Input data buffer.
 * @param size Input data size.
 */
Result libappletPushInData(AppletHolder *h, const void* buffer, size_t size);

/**
 * @brief Pops a storage via \ref appletHolderPopOutData, uses \ref libappletReadStorage, then closes the storage. 
 * @param h AppletHolder object.
 * @param buffer Output buffer.
 * @param size Size to read.
 * @param transfer_size Optional output size field for the actual size used for the read, can be NULL.
 */
Result libappletPopOutData(AppletHolder *h, void* buffer, size_t size, size_t *transfer_size);

/**
 * @brief Starts the applet and waits for it to finish, then checks the \ref LibAppletExitReason.
 * @note Uses \ref appletHolderStart and \ref appletHolderJoin.
 * @param h AppletHolder object.
 */
Result libappletStart(AppletHolder *h);

/**
 * @brief Creates a LibraryApplet with the specified input storage data, uses \ref libappletStart, and reads the output storage reply data via \ref libappletPopOutData.
 * @param id \ref AppletId
 * @param commonargs \ref LibAppletArgs struct.
 * @param arg Input storage data buffer. Optional, can be NULL.
 * @param arg_size Size of the arg buffer.
 * @param reply Output storage data buffer. Optional, can be NULL.
 * @param reply_size Size to read for the reply buffer.
 * @param out_reply_size Actual read reply data size, see \ref libappletPopOutData.
 */
Result libappletLaunch(AppletId id, LibAppletArgs *commonargs, const void* arg, size_t arg_size, void* reply, size_t reply_size, size_t *out_reply_size);

/// Wrapper for \ref appletPushToGeneralChannel, see appletPushToGeneralChannel regarding the requirements for using this.
/// Returns to the main Home Menu, equivalent to pressing the HOME button.
Result libappletRequestHomeMenu(void);

/// Wrapper for \ref appletPushToGeneralChannel, see appletPushToGeneralChannel regarding the requirements for using this.
/// Equivalent to entering "System Update" under System Settings. When leaving this, it returns to the main Home Menu.
Result libappletRequestJumpToSystemUpdate(void);

