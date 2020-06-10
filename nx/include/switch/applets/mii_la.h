/**
 * @file mii_la.h
 * @brief Wrapper for using the MiiEdit LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/mii.h"

/// AppletMode
typedef enum {
    NfpLaMiiLaAppletMode_ShowMiiEdit         = 0,   ///< ShowMiiEdit
    NfpLaMiiLaAppletMode_AppendMii           = 1,   ///< AppendMii
    NfpLaMiiLaAppletMode_AppendMiiImage      = 2,   ///< AppendMiiImage
    NfpLaMiiLaAppletMode_UpdateMiiImage      = 3,   ///< UpdateMiiImage
} MiiLaAppletMode;

/// AppletInput
typedef struct {
    u32 unk_x0;                ///< Always set to value 0x3.
    u32 mode;                  ///< \ref MiiLaAppletMode
    s32 special_key_code;      ///< \ref MiiSpecialKeyCode
    Uuid valid_uuid_array[8];  ///< ValidUuidArray. Only used with \ref MiiLaAppletMode ::NfpLaMiiLaAppletMode_AppendMiiImage / ::NfpLaMiiLaAppletMode_UpdateMiiImage.
    Uuid used_uuid;            ///< UsedUuid. Only used with \ref MiiLaAppletMode ::NfpLaMiiLaAppletMode_UpdateMiiImage.
    u8 unk_x9C[0x64];          ///< Unused
} MiiLaAppletInput;

/// AppletOutput
typedef struct {
    u32 res;                   ///< Result: 0 = Success, 1 = Cancel.
    s32 index;                 ///< Index. Only set when Result is Success, where \ref MiiLaAppletMode isn't ::NfpLaMiiLaAppletMode_ShowMiiEdit.
    u8 unk_x8[0x18];           ///< Unused
} MiiLaAppletOutput;

/**
 * @brief Launches the applet for ShowMiiEdit.
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 */
Result miiLaShowMiiEdit(MiiSpecialKeyCode special_key_code);

/**
 * @brief Launches the applet for AppendMii.
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 * @param[out] index Output Index.
 */
Result miiLaAppendMii(MiiSpecialKeyCode special_key_code, s32 *index);

/**
 * @brief Launches the applet for AppendMiiImage.
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 * @param[in] valid_uuid_array Input array of Uuid.
 * @oaram[in] count Total entries for the valid_uuid_array. Must be 0-8.
 * @param[out] index Output Index.
 */
Result miiLaAppendMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, s32 *index);

/**
 * @brief Launches the applet for UpdateMiiImage.
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 * @param[in] valid_uuid_array Input array of Uuid.
 * @oaram[in] count Total entries for the valid_uuid_array. Must be 0-8.
 * @param[in] used_uuid UsedUuid
 * @param[out] index Output Index.
 */
Result miiLaUpdateMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, Uuid used_uuid, s32 *index);

