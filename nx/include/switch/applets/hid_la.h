/**
 * @file hid_la.h
 * @brief Wrapper for using the controller LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/hid.h"

/// Mode values for HidLaControllerSupportArgPrivate::mode.
typedef enum {
    HidLaControllerSupportMode_ShowControllerSupport           = 0,    ///< ShowControllerSupport
    HidLaControllerSupportMode_ShowControllerStrapGuide        = 1,    ///< [3.0.0+] ShowControllerStrapGuide
    HidLaControllerSupportMode_ShowControllerFirmwareUpdate    = 2,    ///< [3.0.0+] ShowControllerFirmwareUpdate
} HidLaControllerSupportMode;

/// ControllerSupportCaller
typedef enum {
    HidLaControllerSupportCaller_Application                   = 0,    ///< Application, this is the default.
    HidLaControllerSupportCaller_System                        = 1,    ///< System. Skips the firmware-update confirmation dialog. This has the same affect as using the controller-update option from qlaunch System Settings.
} HidLaControllerSupportCaller;

/// ControllerSupportArgPrivate
typedef struct {
    u32 private_size;                                                  ///< Size of this ControllerSupportArgPrivate struct.
    u32 arg_size;                                                      ///< Size of the storage following this one (\ref HidLaControllerSupportArg or \ref HidLaControllerFirmwareUpdateArg).
    u8 flag0;                                                          ///< Flag0
    u8 flag1;                                                          ///< Flag1
    u8 mode;                                                           ///< \ref HidLaControllerSupportMode
    u8 controller_support_caller;                                      ///< \ref HidLaControllerSupportCaller. Always zero except with \ref hidLaShowControllerFirmwareUpdateForSystem, which sets this to the input param.
    u32 npad_style_set;                                                ///< Output from \ref hidGetSupportedNpadStyleSet. With ShowControllerSupportForSystem on pre-3.0.0 this is value 0.
    u32 npad_joy_hold_type;                                            ///< Output from \ref hidGetNpadJoyHoldType. With ShowControllerSupportForSystem on pre-3.0.0 this is value 1.
} HidLaControllerSupportArgPrivate;

/// Common header used by HidLaControllerSupportArg*.
/// max_supported_players is 4 on pre-8.0.0, 8 on [8.0.0+]. player_count_min and player_count_max are overriden with value 4 when larger than value 4, during conversion handling for \ref HidLaControllerSupportArg on pre-8.0.0.
typedef struct {
    s8 player_count_min;                                               ///< playerCountMin. Must be >=0 and <=max_supported_players.
    s8 player_count_max;                                               ///< playerCountMax. Must be >=1 and <=max_supported_players.
    u8 enable_take_over_connection;                                    ///< enableTakeOverConnection, non-zero to enable. Disconnects the controllers when not enabled.
    u8 enable_left_justify;                                            ///< enableLeftJustify, non-zero to enable.
    u8 enable_permit_joy_dual;                                         ///< enablePermitJoyDual, non-zero to enable.
    u8 enable_single_mode;                                             ///< enableSingleMode, non-zero to enable. Enables using a single player in handheld-mode, dual-mode, or single-mode (player_count_* are overridden). Using handheld-mode is not allowed if this is not enabled.
    u8 enable_identification_color;                                    ///< When non-zero enables using identification_color.
} HidLaControllerSupportArgHeader;

/// Identification color used by HidLaControllerSupportArg*. When HidLaControllerSupportArgHeader::enable_identification_color is set this controls the color of the UI player box outline.
typedef struct {
    u8 r;                                                              ///< Red color component.
    u8 g;                                                              ///< Green color component.
    u8 b;                                                              ///< Blue color component.
    u8 a;                                                              ///< Alpha color component.
} HidLaControllerSupportArgColor;

/// ControllerSupportArg for [1.0.0+].
typedef struct {
    HidLaControllerSupportArgHeader hdr;                               ///< \ref HidLaControllerSupportArgHeader
    HidLaControllerSupportArgColor identification_color[4];            ///< \ref HidLaControllerSupportArgColor for each player, see HidLaControllerSupportArgHeader::enable_identification_color.
    u8 enable_explain_text;                                            ///< Enables using the ExplainText data when non-zero.
    char explain_text[4][0x81];                                        ///< ExplainText for each player, NUL-terminated UTF-8 strings.
} HidLaControllerSupportArgV3;

/// ControllerSupportArg for [8.0.0+], converted to \ref HidLaControllerSupportArgV3 on pre-8.0.0.
typedef struct {
    HidLaControllerSupportArgHeader hdr;                               ///< \ref HidLaControllerSupportArgHeader
    HidLaControllerSupportArgColor identification_color[8];            ///< \ref HidLaControllerSupportArgColor for each player, see HidLaControllerSupportArgHeader::enable_identification_color.
    u8 enable_explain_text;                                            ///< Enables using the ExplainText data when non-zero.
    char explain_text[8][0x81];                                        ///< ExplainText for each player, NUL-terminated UTF-8 strings.
} HidLaControllerSupportArg;

/// ControllerFirmwareUpdateArg
typedef struct {
    u8 enable_force_update;                                            ///< enableForceUpdate, non-zero to enable. Default is 0. Forces a firmware update when enabled, without an UI option to skip it.
    u8 pad[3];                                                         ///< Padding.
} HidLaControllerFirmwareUpdateArg;

/// ControllerSupportResultInfo. First 8-bytes from the applet output storage.
typedef struct {
    s8 player_count;                                                   ///< playerCount.
    u8 pad[3];                                                         ///< Padding.
    u32 selected_id;                                                   ///< \ref HidControllerID, selectedId.
} HidLaControllerSupportResultInfo;

/// Struct for the applet output storage.
typedef struct {
    HidLaControllerSupportResultInfo info;                             ///< \ref HidLaControllerSupportResultInfo
    u32 res;                                                           ///< Output res value.
} HidLaControllerSupportResultInfoInternal;

/**
 * @brief Initializes a \ref HidLaControllerSupportArg with the defaults.
 * @note This clears the arg, then does: HidLaControllerSupportArgHeader::player_count_min = 0, HidLaControllerSupportArgHeader::player_count_max = 4, HidLaControllerSupportArgHeader::enable_take_over_connection = 1, HidLaControllerSupportArgHeader::enable_left_justify = 1, and HidLaControllerSupportArgHeader::enable_permit_joy_dual = 1.
 * @note If preferred, you can also memset \ref HidLaControllerSupportArg manually and initialize it yourself.
 * @param[out] arg \ref HidLaControllerSupportArg
 */
void hidLaCreateControllerSupportArg(HidLaControllerSupportArg *arg);

/**
 * @brief Initializes a \ref HidLaControllerFirmwareUpdateArg with the defaults.
 * @note This just uses memset() with the arg.
 * @param[out] arg \ref HidLaControllerFirmwareUpdateArg
 */
void hidLaCreateControllerFirmwareUpdateArg(HidLaControllerFirmwareUpdateArg *arg);

/**
 * @brief Sets the ExplainText for the specified player and \ref HidLaControllerSupportArg.
 * @note This string is displayed in the UI box for the player.
 * @note HidLaControllerSupportArg::enable_explain_text must be set, otherwise this ExplainText is ignored.
 * @param arg \ref HidLaControllerSupportArg
 * @param[in] str Input ExplainText UTF-8 string, max length is 0x80 excluding NUL-terminator.
 + @oaram[in] id Player controller, must be <8.
 */
Result hidLaSetExplainText(HidLaControllerSupportArg *arg, const char *str, HidControllerID id);

/**
 * @brief Launches the applet for ControllerSupport.
 * @note This seems to only display the applet UI when doing so is actually needed? This doesn't apply to \ref hidLaShowControllerSupportForSystem.
 * @param[out] result_info \ref HidLaControllerSupportResultInfo. Optional, can be NULL.
 * @param[in] arg \ref HidLaControllerSupportArg
 */
Result hidLaShowControllerSupport(HidLaControllerSupportResultInfo *result_info, const HidLaControllerSupportArg *arg);

/**
 * @brief Launches the applet for ControllerStrapGuide.
 * @note Only available on [3.0.0+].
 */
Result hidLaShowControllerStrapGuide(void);

/**
 * @brief Launches the applet for ControllerFirmwareUpdate.
 * @note Only available on [3.0.0+].
 * @param[in] arg \ref HidLaControllerFirmwareUpdateArg
 */
Result hidLaShowControllerFirmwareUpdate(const HidLaControllerFirmwareUpdateArg *arg);

/**
 * @brief This is the system version of \ref hidLaShowControllerSupport.
 * @param[out] result_info \ref HidLaControllerSupportResultInfo. Optional, can be NULL.
 * @param[in] arg \ref HidLaControllerSupportArg
 * @param[in] flag Input flag. When true, the applet displays the menu as if launched by qlaunch.
 */
Result hidLaShowControllerSupportForSystem(HidLaControllerSupportResultInfo *result_info, const HidLaControllerSupportArg *arg, bool flag);

/**
 * @brief This is the system version of \ref hidLaShowControllerFirmwareUpdate.
 * @note Only available on [3.0.0+].
 * @param[in] arg \ref HidLaControllerFirmwareUpdateArg
 * @param[in] caller \ref HidLaControllerSupportCaller
 */
Result hidLaShowControllerFirmwareUpdateForSystem(const HidLaControllerFirmwareUpdateArg *arg, HidLaControllerSupportCaller caller);

