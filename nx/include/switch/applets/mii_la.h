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
    MiiLaAppletMode_ShowMiiEdit         = 0,   ///< ShowMiiEdit
    MiiLaAppletMode_AppendMii           = 1,   ///< AppendMii
    MiiLaAppletMode_AppendMiiImage      = 2,   ///< AppendMiiImage
    MiiLaAppletMode_UpdateMiiImage      = 3,   ///< UpdateMiiImage
    MiiLaAppletMode_CreateMii           = 4,   ///< [10.2.0+] CreateMii
    MiiLaAppletMode_EditMii             = 5,   ///< [10.2.0+] EditMii
} MiiLaAppletMode;

/// AppletInput
typedef struct {
    s32 version;               ///< Version
    u32 mode;                  ///< \ref MiiLaAppletMode
    s32 special_key_code;      ///< \ref MiiSpecialKeyCode
    union {
        Uuid valid_uuid_array[8];  ///< ValidUuidArray. Only used with \ref MiiLaAppletMode ::NfpLaMiiLaAppletMode_AppendMiiImage / ::NfpLaMiiLaAppletMode_UpdateMiiImage.
        struct {
            MiiCharInfo char_info;     ///< \ref MiiCharInfo
            u8 unused_x64[0x28];       ///< Unused
        } char_info;
    };
    Uuid used_uuid;            ///< UsedUuid. Only used with \ref MiiLaAppletMode ::NfpLaMiiLaAppletMode_UpdateMiiImage.
    u8 unk_x9C[0x64];          ///< Unused
} MiiLaAppletInput;

/// AppletOutput
typedef struct {
    u32 res;                   ///< Result: 0 = Success, 1 = Cancel.
    s32 index;                 ///< Index. Only set when Result is Success, where \ref MiiLaAppletMode isn't ::NfpLaMiiLaAppletMode_ShowMiiEdit.
    u8 unk_x8[0x18];           ///< Unused
} MiiLaAppletOutput;

/// AppletOutputForCharInfoEditing
typedef struct {
    u32 res;                   ///< MiiLaAppletOutput::res
    MiiCharInfo char_info;     ///< \ref MiiCharInfo
    u8 unused[0x24];           ///< Unused
} MiiLaAppletOutputForCharInfoEditing;

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
 * @param[in] count Total entries for the valid_uuid_array. Must be 0-8.
 * @param[out] index Output Index.
 */
Result miiLaAppendMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, s32 *index);

/**
 * @brief Launches the applet for UpdateMiiImage.
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 * @param[in] valid_uuid_array Input array of Uuid.
 * @param[in] count Total entries for the valid_uuid_array. Must be 0-8.
 * @param[in] used_uuid UsedUuid
 * @param[out] index Output Index.
 */
Result miiLaUpdateMiiImage(MiiSpecialKeyCode special_key_code, const Uuid *valid_uuid_array, s32 count, Uuid used_uuid, s32 *index);

/**
 * @brief Launches the applet for CreateMii.
 * @note This creates a Mii and returns it, without saving it in the database.
 * @note Only available on [10.2.0+].
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 * @param[out] out_char \ref MiiCharInfo
 */
Result miiLaCreateMii(MiiSpecialKeyCode special_key_code, MiiCharInfo *out_char);

/**
 * @brief Launches the applet for EditMii.
 * @note This edits the specified Mii and returns it, without saving it in the database.
 * @note Only available on [10.2.0+].
 * @param[in] special_key_code \ref MiiSpecialKeyCode
 * @param[in] in_char \ref MiiCharInfo
 * @param[out] out_char \ref MiiCharInfo
 */
Result miiLaEditMii(MiiSpecialKeyCode special_key_code, const MiiCharInfo *in_char, MiiCharInfo *out_char);

