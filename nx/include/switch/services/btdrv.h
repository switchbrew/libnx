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

/// BluetoothPropertyType
typedef enum {
    BtdrvBluetoothPropertyType_Name         =    1,    ///< Name. String, max length 0xF8 excluding NUL-terminator.
    BtdrvBluetoothPropertyType_Address      =    2,    ///< \ref BtdrvAddress
    BtdrvBluetoothPropertyType_Unknown3     =    3,    ///< Only available with \ref btdrvSetAdapterProperty. Unknown. 3-bytes.
    BtdrvBluetoothPropertyType_Unknown5     =    5,    ///< Unknown. 3-bytes.
    BtdrvBluetoothPropertyType_Unknown6     =    6,    ///< Unknown. 1-byte. The default is value 0x68.
} BtdrvBluetoothPropertyType;

/// Address
typedef struct {
    u8 address[0x6];           ///< Address
} BtdrvAddress;

/// AdapterProperty
typedef struct {
    BtdrvAddress addr;         ///< Same as the data for ::BtdrvBluetoothPropertyType_Address.
    u8 type5[0x3];             ///< Same as the data for ::BtdrvBluetoothPropertyType_Unknown5.
    char name[0xF9];           ///< Same as the data for ::BtdrvBluetoothPropertyType_Name (last byte is not initialized).
    u8 type6;                  ///< Set to hard-coded value 0x68 (same as the data for ::BtdrvBluetoothPropertyType_Unknown6).
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
 * @brief GetAdapterProperties
 * @param[out] property \ref BtdrvAdapterProperty
 */
Result btdrvGetAdapterProperties(BtdrvAdapterProperty *property);

/**
 * @brief GetAdapterProperty
 * @param[in] type \ref BtdrvBluetoothPropertyType
 * @param[out] buffer Output buffer, see \ref BtdrvBluetoothPropertyType for the contents.
 * @param[in] size Output buffer size.
 */
Result btdrvGetAdapterProperty(BtdrvBluetoothPropertyType type, void* buffer, size_t size);

/**
 * @brief SetAdapterProperty
 * @param[in] type \ref BtdrvBluetoothPropertyType
 * @param[in] buffer Input buffer, see \ref BtdrvBluetoothPropertyType for the contents.
 * @param[in] size Input buffer size.
 */
Result btdrvSetAdapterProperty(BtdrvBluetoothPropertyType type, const void* buffer, size_t size);

/**
 * @brief WriteHidData
 * @param[in] addr \ref BtdrvAddress
 * @param[in] buffer Input \ref BtdrvHidReport, on pre-9.0.0 this is \ref BtdrvHidData.
 */
Result btdrvWriteHidData(BtdrvAddress addr, BtdrvHidReport *buffer);

/**
 * @brief WriteHidData2
 * @param[in] addr \ref BtdrvAddress
 * @param[in] buffer Input buffer, same as the buffer for \ref btdrvWriteHidData.
 * @param[in] size Input buffer size.
 */
Result btdrvWriteHidData2(BtdrvAddress addr, const void* buffer, size_t size);

/**
 * @brief SetHidReport
 * @param[in] addr \ref BtdrvAddress
 * @param[in] type BluetoothHhReportType
 * @param[in] buffer Input \ref BtdrvHidReport, on pre-9.0.0 this is \ref BtdrvHidData.
 */
Result btdrvSetHidReport(BtdrvAddress addr, u32 type, BtdrvHidReport *buffer);

/**
 * @brief GetHidReport
 * @param[in] addr \ref BtdrvAddress
 * @param[in] unk Unknown
 * @param[in] type BluetoothHhReportType
 */
Result btdrvGetHidReport(BtdrvAddress addr, u8 unk, u32 type);

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
 * @param[in] size Input buffer size, must be <=0x258.
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
 * @param[in] size Input buffer size, must be <=0x258.
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

