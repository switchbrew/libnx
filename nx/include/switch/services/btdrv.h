/**
 * @file btdrv.h
 * @brief Bluetooth driver (btdrv) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

/// Address
typedef struct {
    u8 address[0x6];           ///< Address
} BtdrvAddress;

/// AdapterProperty
typedef struct {
    u8 unk_x0[0x103];          ///< Unknown
} BtdrvAdapterProperty;

/// BluetoothPinCode
typedef struct {
    char code[0x10];           ///< PinCode
} BtdrvBluetoothPinCode;

/// HidData, for pre-9.0.0.
typedef struct {
    u16 size;                  ///< Size of data.
    u8 data[0x280];            ///< Data
} BtdrvHidData;

/// HidReport, for [9.0.0+].
typedef struct {
    u16 size;                  ///< Size of data.
    u8 data[0x2BC];            ///< Data
} BtdrvHidReport;

/// PlrStatistics
typedef struct {
    u8 unk_x0[0x84];           ///< Unknown
} BtdrvPlrStatistics;

/// PlrList
typedef struct {
    u8 unk_x0[0xA4];           ///< Unknown
} BtdrvPlrList;

/// ChannelMapList
typedef struct {
    u8 unk_x0[0x88];           ///< Unknown
} BtdrvChannelMapList;

/// LeConnectionParams
typedef struct {
    u8 unk_x0[0x14];           ///< Unknown
} BtdrvLeConnectionParams;

/// BleConnectionParameter
typedef struct {
    u8 unk_x0[0xC];           ///< Unknown
} BtdrvBleConnectionParameter;

/// BleAdvertisePacketData
typedef struct {
    u8 unk_x0[0xCC];           ///< Unknown
} BtdrvBleAdvertisePacketData;

/// BleAdvertiseFilter
typedef struct {
    u8 unk_x0[0x3E];           ///< Unknown
} BtdrvBleAdvertiseFilter;

/// GattAttributeUuid
typedef struct {
    u8 unk_x0[0x14];           ///< Unknown
} BtdrvGattAttributeUuid;

/// GattId
typedef struct {
    u8 unk_x0[0x18];           ///< Unknown
} BtdrvGattId;

/// Initialize btdrv.
Result btdrvInitialize(void);

/// Exit btdrv.
void btdrvExit(void);

/// Gets the Service object for the actual btdrv service session.
Service* btdrvGetServiceSession(void);

/**
 * @brief ReadGattCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] unk2 Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btdrvReadGattCharacteristic(bool flag, u8 unk, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief ReadGattDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] unk2 Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 */
Result btdrvReadGattDescriptor(bool flag, u8 unk, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2);

/**
 * @brief WriteGattCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] flag2 Flag
 * @param[in] unk2 Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 */
Result btdrvWriteGattCharacteristic(bool flag, u8 unk, bool flag2, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1, const void* buffer, size_t size);

/**
 * @brief WriteGattDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] unk2 Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 */
Result btdrvWriteGattDescriptor(bool flag, u8 unk, u32 unk2, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, const void* buffer, size_t size);

/**
 * @brief RegisterGattNotification
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btdrvRegisterGattNotification(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief UnregisterGattNotification
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btdrvUnregisterGattNotification(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief GetLeEventInfo
 * @note Only available on [5.0.0+].
 * @note The state used by this is reset after writing the data to output.
 * @param[in] buffer Output buffer. 0x400-bytes from state is written here.
 * @param[in] size Output buffer size.
 * @oaram[out] type Output BleEventType.
 */
Result btdrvGetLeEventInfo(void* buffer, size_t size, u32 *type);

/**
 * @brief RegisterBleHidEvent
 * @note Only available on [5.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btdrvRegisterBleHidEvent(Event* out_event);

