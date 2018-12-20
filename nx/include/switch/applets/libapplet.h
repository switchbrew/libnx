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
 * @brief Sets the tick field in LibAppletArgs, then creates a storage with it which is pushed to the AppletHolder via \ref appletHolderPushInData.
 * @param a LibAppletArgs struct.
 * @param h AppletHolder object.
 */
Result libappletArgsPush(LibAppletArgs* a, AppletHolder *h);

