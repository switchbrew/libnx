/**
 * @file nfp_la.h
 * @brief Wrapper for using the cabinet (amiibo) LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/nfc.h"

/// Values for NfpLaStartParamForAmiiboSettings::type.
typedef enum {
    NfpLaStartParamTypeForAmiiboSettings_NicknameAndOwnerSettings = 0,   ///< NicknameAndOwnerSettings
    NfpLaStartParamTypeForAmiiboSettings_GameDataEraser           = 1,   ///< GameDataEraser
    NfpLaStartParamTypeForAmiiboSettings_Restorer                 = 2,   ///< Restorer
    NfpLaStartParamTypeForAmiiboSettings_Formatter                = 3,   ///< Formatter
} NfpLaStartParamTypeForAmiiboSettings;

/// AmiiboSettingsStartParam
typedef struct {
    u8 unk_x0[0x8];                       ///< Unknown
    u8 unk_x8[0x20];                      ///< Unknown
    u8 unk_x28;                           ///< Unknown
} NfpLaAmiiboSettingsStartParam;

/// StartParamForAmiiboSettings
typedef struct {
    u8 unk_x0;                            ///< Unknown
    u8 type;                              ///< \ref NfpLaStartParamTypeForAmiiboSettings
    u8 flags;                             ///< Flags
    u8 unk_x3;                            ///< NfpLaAmiiboSettingsStartParam::unk_x28
    u8 unk_x4[0x8];                       ///< NfpLaAmiiboSettingsStartParam::unk_x0
    NfpTagInfo tag_info;                  ///< \ref NfpTagInfo, only enabled when flags bit1 is set.
    NfpRegisterInfo register_info;        ///< \ref NfpRegisterInfo, only enabled when flags bit2 is set.
    u8 unk_x164[0x20];                    ///< NfpLaAmiiboSettingsStartParam::unk_x8
    u8 unk_x184[0x24];                    ///< Unknown
} NfpLaStartParamForAmiiboSettings;

/// ReturnValueForAmiiboSettings
typedef struct {
    u8 flags;                             ///< 0 = error, non-zero = success.
    u8 pad[3];                            ///< Padding
    NfcDeviceHandle handle;               ///< \ref NfcDeviceHandle
    NfpTagInfo tag_info;                  ///< \ref NfpTagInfo
    NfpRegisterInfo register_info;        ///< \ref NfpRegisterInfo, only available when flags bit2 is set.
    u8 unk_x164[0x24];                    ///< Unknown
} NfpLaReturnValueForAmiiboSettings;

/**
 * @brief Launches the applet for NicknameAndOwnerSettings.
 * @note Official sw does not expose functionality for using input/output \ref NfpTagInfo at the same time.
 * @param[in] in_param \ref NfpLaAmiiboSettingsStartParam
 * @param[in] in_tag_info \ref NfpTagInfo. Optional, can be NULL. If specified, this must match the scanned amiibo.
 * @param[in] in_reg_info \ref NfpRegisterInfo. Optional, can be NULL. If specified, this sets the \ref NfpRegisterInfo which will be used for writing, with an option for the user to change it.
 * @param[out] out_tag_info \ref NfpTagInfo. Optional, can be NULL.
 * @param[out] handle \ref NfcDeviceHandle
 * @param[out] reg_info_flag Flag indicating whether the data for out_reg_info is set. Optional, can be NULL.
 * @param[out] out_reg_info \ref NfpRegisterInfo, see reg_info_flag. Optional, can be NULL.
 */
Result nfpLaStartNicknameAndOwnerSettings(const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, const NfpRegisterInfo *in_reg_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle, bool *reg_info_flag, NfpRegisterInfo *out_reg_info);

/**
 * @brief Launches the applet for GameDataEraser.
 * @note Official sw does not expose functionality for using input/output \ref NfpTagInfo at the same time.
 * @param[in] in_param \ref NfpLaAmiiboSettingsStartParam
 * @param[in] in_tag_info \ref NfpTagInfo. Optional, can be NULL. If specified, this must match the scanned amiibo.
 * @param[out] out_tag_info \ref NfpTagInfo. Optional, can be NULL.
 * @param[out] handle \ref NfcDeviceHandle
 */
Result nfpLaStartGameDataEraser(const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle);

/**
 * @brief Launches the applet for Restorer.
 * @note Official sw does not expose functionality for using input/output \ref NfpTagInfo at the same time.
 * @param[in] in_param \ref NfpLaAmiiboSettingsStartParam
 * @param[in] in_tag_info \ref NfpTagInfo. Optional, can be NULL. If specified, this must match the scanned amiibo.
 * @param[out] out_tag_info \ref NfpTagInfo. Optional, can be NULL.
 * @param[out] handle \ref NfcDeviceHandle
 */
Result nfpLaStartRestorer(const NfpLaAmiiboSettingsStartParam *in_param, const NfpTagInfo *in_tag_info, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle);

/**
 * @brief Launches the applet for Formatter.
 * @param[in] in_param \ref NfpLaAmiiboSettingsStartParam
 * @param[out] out_tag_info \ref NfpTagInfo
 * @param[out] handle \ref NfcDeviceHandle
 */
Result nfpLaStartFormatter(const NfpLaAmiiboSettingsStartParam *in_param, NfpTagInfo *out_tag_info, NfcDeviceHandle *handle);

