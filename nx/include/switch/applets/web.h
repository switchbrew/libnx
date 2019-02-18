/**
 * @file web.h
 * @brief Wrapper for using the WifiWebAuthApplet LibraryApplet.
 * @author p-sam, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

typedef struct {
    u32 unk_x0;                 ///< Official sw sets this to 0 with appletStorageWrite, seperately from the rest of the config struct.
    char conntest_url[0x100];
    char initial_url[0x400];
    u128 userID;
    u32 unk_x514;
} PACKED WebWifiPageArg;

typedef struct {
    WebWifiPageArg arg;
} WebWifiConfig;

/**
 * @brief Creates the config for WifiWebAuthApplet.
 * @param config WebWifiConfig object.
 * @param conntest_url URL used for the connection-test requests, if NULL initial_url is used for this.
 * @param initial_url Initial URL navigated to by the applet.
 * @param userID Account userID, 0 for common.
 * @param unk Value written to \ref WebWifiPageArg unk_x514, this can be 0.
 */
void webWifiCreate(WebWifiConfig* config, const char* conntest_url, const char* initial_url, u128 userID, u32 unk);

/**
 * @brief Launches WifiWebAuthApplet with the specified config and waits for it to exit.
 * @param config WebWifiConfig object.
 */
Result webWifiShow(WebWifiConfig* config);

