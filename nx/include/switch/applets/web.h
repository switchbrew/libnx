/**
 * @file web.h
 * @brief Wrapper for using the WifiWebAuthApplet LibraryApplet.
 * @author p-sam, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

/// This indicates the type of web-applet.
typedef enum {
    WebShimKind_Login = 2,
    WebShimKind_Share = 4,
    WebShimKind_Web   = 5,
    WebShimKind_Wifi  = 6,
    WebShimKind_Lobby = 7,
} WebShimKind;

typedef struct {
    u32 unk_x0;                 ///< Official sw sets this to 0 with appletStorageWrite, separately from the rest of the config struct.
    char conntest_url[0x100];
    char initial_url[0x400];
    u128 userID;
    u32 unk_x514;
} PACKED WebWifiPageArg;

typedef struct {
    u32 unk_x0;
    Result res;
} PACKED WebWifiReturnValue;

typedef struct {
    WebWifiPageArg arg;
} WebWifiConfig;

/// TLV storage, starts with \ref WebArgHeader followed by \ref WebArgTLV entries.
typedef struct {
    u8 data[0x2000];
} WebCommonTLVStorage;

typedef struct {
    u32 exitReason;
    u32 pad;
    char lastUrl[0x1000];
    u64 lastUrlSize;
} PACKED WebCommonReturnValue;

/// Header struct at offset 0 in the web Arg storage (non-webWifi).
typedef struct {
    u16 total_entries;  ///< Total \ref WebArgTLV entries following this struct.
    u16 pad;
    WebShimKind shimKind;
} PACKED WebArgHeader;

/// Web TLV used in the web Arg storage.
typedef struct {
    u16 type;       ///< Type of this arg.
    u16 size;       ///< Size of the arg data following this struct.
    u8 pad[4];
} PACKED WebArgTLV;

typedef struct {
    WebCommonTLVStorage arg;
} WebPageConfig;

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
 * @param out Optional output applet reply data, can be NULL.
 */
Result webWifiShow(WebWifiConfig* config, WebWifiReturnValue *out);

/**
 * @brief Creates the config for WebApplet. This applet uses an URL whitelist loaded from the user-process host title.
 * @param config WebPageConfig object.
 * @param url Initial URL navigated to by the applet.
 */
void webPageCreate(WebPageConfig* config, const char* url);

/**
 * @brief Launches WebApplet with the specified config and waits for it to exit.
 * @param config WebPageConfig object.
 * @param out Optional output applet reply data, can be NULL.
 */
Result webPageShow(WebPageConfig* config, WebCommonReturnValue *out);

