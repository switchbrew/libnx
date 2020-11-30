/**
 * @file ringcon.h
 * @brief Wrapper for using the Ring-Con attached to a Joy-Con, with hidbus. See also: https://switchbrew.org/wiki/Ring-Con
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/hidbus.h"

#define RINGCON_CAL_MAGIC -0x3502 // 0xCAFE

/// Whether the output data is valid.
typedef enum {
    RingConDataValid_Ok  = 0,                     ///< Valid.
    RingConDataValid_CRC = 1,                     ///< Bad CRC.
    RingConDataValid_Cal = 2,                     ///< Only used with \ref ringconReadUserCal. Calibration is needed via \ref ringconUpdateUserCal.
} RingConDataValid;

typedef enum {
    RingConErrorFlag_BadUserCalUpdate   = 0,      ///< The output from \ref ringconReadUserCal doesn't match the input used with \ref ringconWriteUserCal, or the \ref RingConDataValid is not ::RingConDataValid_Ok.
    RingConErrorFlag_BadFlag            = 4,      ///< The output flag from \ref ringconCmdx00020105 when successful is invalid.
    RingConErrorFlag_BadUserCal         = 5,      ///< BadUserCal
    RingConErrorFlag_BadManuCal         = 6,      ///< BadManuCal
} RingConErrorFlag;

/// Ring-Con firmware version.
typedef struct {
    u8 fw_main_ver;                               ///< Main firmware version.
    u8 fw_sub_ver;                                ///< Sub firmware version.
} RingConFwVersion;

/// Ring-Con manufacturer calibration.
typedef struct {
    s16 os_max;                                   ///< (manu_)os_max
    s16 hk_max;                                   ///< (manu_)hk_max
    s16 zero_min;                                 ///< (manu_)zero_min
    s16 zero_max;                                 ///< (manu_)zero_max
} RingConManuCal;

/// Ring-Con user calibration.
typedef struct {
    s16 os_max;                                   ///< (user_)os_max
    s16 hk_max;                                   ///< (user_)hk_max
    s16 zero;                                     ///< (user_)zero
    RingConDataValid data_valid;                  ///< \ref RingConDataValid
} RingConUserCal;

/// Polling data extracted from \ref HidbusJoyPollingReceivedData.
typedef struct {
    s16 data;                                     ///< Sensor state data.
    u64 sampling_number;                          ///< SamplingNumber
} RingConPollingData;

/// Ring-Con state object.
typedef struct {
    bool bus_initialized;
    HidbusBusHandle handle;
    void* workbuf;
    size_t workbuf_size;
    u64 polling_last_sampling_number;
    u32 error_flags;

    u64 id_l, id_h;
    RingConFwVersion fw_ver;
    u32 flag;
    s16 unk_cal;
    s32 total_push_count;

    RingConManuCal manu_cal;
    RingConUserCal user_cal;
} RingCon;

/**
 * @brief Creates a \ref RingCon object, and handles the various initialization for it.
 * @param c \ref RingCon
 * @param[in] id \ref HidNpadIdType. A Ring-Con must be attached to this controller.
 */
Result ringconCreate(RingCon *c, HidNpadIdType id);

/**
 * @brief Close a \ref RingCon.
 * @param c \ref RingCon
 */
void ringconClose(RingCon *c);

/**
 * @brief Gets the error flags field.
 * @param c \ref RingCon
 */
NX_CONSTEXPR u32 ringconGetErrorFlags(RingCon *c) {
    return c->error_flags;
}

/**
 * @brief Gets the value of an error flag, set by \ref ringconSetErrorFlag.
 * @param c \ref RingCon
 * @param[in] flag \ref RingConErrorFlag
 */
NX_CONSTEXPR bool ringconGetErrorFlag(RingCon *c, RingConErrorFlag flag) {
    return (c->error_flags & BIT(flag)) != 0;
}

/**
 * @brief Gets the \ref RingConFwVersion previously loaded by \ref ringconCreate.
 * @param c \ref RingCon
 * @param[out] out \ref RingConFwVersion
 */
NX_CONSTEXPR RingConFwVersion ringconGetFwVersion(RingCon *c) {
    return c->fw_ver;
}

/**
 * @brief Gets the Id previously loaded by \ref ringconCreate.
 * @param c \ref RingCon
 * @param[out] id_l Id low.
 * @param[out] id_h Id high.
 */
NX_CONSTEXPR void ringconGetId(RingCon *c, u64 *id_l, u64 *id_h) {
    *id_l = c->id_l;
    *id_h = c->id_h;
}

/**
 * @brief Gets the unk_cal previously loaded by \ref ringconCreate with \ref ringconReadUnkCal. Only valid when the output flag from \ref ringconCmdx00020105 is valid.
 * @param c \ref RingCon
 */
NX_CONSTEXPR s16 ringconGetUnkCal(RingCon *c) {
    return c->unk_cal;
}

/**
 * @brief Gets the total-push-count previously loaded by \ref ringconCreate.
 * @param c \ref RingCon
 * @param[out] out total_push_count
 */
NX_CONSTEXPR s32 ringconGetTotalPushCount(RingCon *c) {
    return c->total_push_count;
}

/**
 * @brief Gets the \ref RingConManuCal previously loaded by \ref ringconCreate.
 * @param c \ref RingCon
 * @param[out] out \ref RingConManuCal
 */
NX_CONSTEXPR void ringconGetManuCal(RingCon *c, RingConManuCal *out) {
    *out = c->manu_cal;
}

/**
 * @brief Gets the \ref RingConUserCal previously loaded by \ref ringconCreate.
 * @note The Ring-Con UserCal doesn't seem to be calibrated normally?
 * @param c \ref RingCon
 * @param[out] out \ref RingConUserCal
 */
NX_CONSTEXPR void ringconGetUserCal(RingCon *c, RingConUserCal *out) {
    *out = c->user_cal;
}

/**
 * @brief Updates the \ref RingConUserCal.
 * @note The input \ref RingConUserCal is used with \ref ringconWriteUserCal, and the output from \ref ringconReadUserCal is verified with the input \ref RingConUserCal. This does not update the \ref RingConUserCal returned by \ref ringconGetUserCal.
 * @note The Ring-Con UserCal doesn't seem to be calibrated normally?
 * @param c \ref RingCon
 * @param[in] cal \ref RingConUserCal
 */
Result ringconUpdateUserCal(RingCon *c, RingConUserCal cal);

/**
 * @brief Reads the \ref RingConFwVersion.
 * @note This is used internally by \ref ringconCreate. Normally you should use \ref ringconGetFwVersion instead.
 * @param c \ref RingCon
 * @param[out] out \ref RingConFwVersion
 */
Result ringconReadFwVersion(RingCon *c, RingConFwVersion *out);

/**
 * @brief Reads the Id.
 * @note This is used internally by \ref ringconCreate. Normally you should use \ref ringconGetId instead.
 * @param c \ref RingCon
 * @param[out] id_l Id low.
 * @param[out] id_h Id high.
 */
Result ringconReadId(RingCon *c, u64 *id_l, u64 *id_h);

/**
 * @brief Gets the \ref RingConPollingData. Only returns entries which are new since the last time this was called (or if not previously called, all available entries up to count).
 * @param c \ref RingCon
 * @param[out] out Output array of \ref RingConPollingData. Entry order is newest -> oldest.
 * @param[in] count Total size of the out array in entries, max value is 0x9.
 * @param[out] total_out Total output entries.
 */
Result ringconGetPollingData(RingCon *c, RingConPollingData *out, s32 count, s32 *total_out);

/**
 * @brief Uses cmd 0x00020105.
 * @note Used internally by \ref ringconCreate.
 * @param c \ref RingCon
 * @param[out] out Output value.
 */
Result ringconCmdx00020105(RingCon *c, u32 *out);

/**
 * @brief Reads the \ref RingConManuCal.
 * @note Used internally by \ref ringconCreate and \ref ringconReadUnkCal.
 * @param c \ref RingCon
 * @param[out] out \ref RingConManuCal
 */
Result ringconReadManuCal(RingCon *c, RingConManuCal *out);

/**
 * @brief Gets the unknown value derived from the output of cmd 0x00020504 and \ref ringconReadManuCal.
 * @note Used internally by \ref ringconCreate.
 * @param c \ref RingCon
 * @param[out] out Output value.
 */
Result ringconReadUnkCal(RingCon *c, s16 *out);

/**
 * @brief Reads the \ref RingConUserCal.
 * @note Used internally by \ref ringconCreate and \ref ringconUpdateUserCal.
 * @param c \ref RingCon
 * @param[out] out \ref RingConUserCal
 */
Result ringconReadUserCal(RingCon *c, RingConUserCal *out);

/**
 * @brief Reads the rep-count for Multitask Mode.
 * @param c \ref RingCon
 * @param[out] out Output value. Official sw using this clamps the output to range 0-500.
 * @param[out] data_valid \ref RingConDataValid
 */
Result ringconReadRepCount(RingCon *c, s32 *out, RingConDataValid *data_valid);

/**
 * @brief Reads the total-push-count, for Multitask Mode.
 * @note Used internally by \ref ringconCreate. Normally \ref ringconGetTotalPushCount should be used instead.
 * @param c \ref RingCon
 * @param[out] out Output value.
 * @param[out] data_valid \ref RingConDataValid
 */
Result ringconReadTotalPushCount(RingCon *c, s32 *out, RingConDataValid *data_valid);

/**
 * @brief This resets the value returned by \ref ringconReadRepCount to 0.
 * @param c \ref RingCon
 */
Result ringconResetRepCount(RingCon *c);

/**
 * @brief Writes the \ref RingConUserCal.
 * @note Used internally by \ref ringconUpdateUserCal.
 * @param c \ref RingCon
 * @param[in] cal \ref RingConUserCal
 */
Result ringconWriteUserCal(RingCon *c, RingConUserCal cal);

