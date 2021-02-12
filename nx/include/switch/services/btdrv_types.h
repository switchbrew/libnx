/**
 * @file btdrv_types.h
 * @brief Bluetooth driver (btdrv) service types (see btdrv.h for the rest).
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// BluetoothPropertyType
typedef enum {
    BtdrvBluetoothPropertyType_Name           =    1,    ///< Name. String, max length 0xF8 excluding NUL-terminator.
    BtdrvBluetoothPropertyType_Address        =    2,    ///< \ref BtdrvAddress
    BtdrvBluetoothPropertyType_Unknown3       =    3,    ///< Only available with \ref btdrvSetAdapterProperty. Unknown, \ref BtdrvAddress.
    BtdrvBluetoothPropertyType_ClassOfDevice  =    5,    ///< 3-bytes, Class of Device.
    BtdrvBluetoothPropertyType_FeatureSet     =    6,    ///< 1-byte, FeatureSet. The default is value 0x68.
} BtdrvBluetoothPropertyType;

/// EventType
typedef enum {
    BtdrvEventType_Unknown0                 =    0,    ///< Unused
    BtdrvEventType_InquiryDevice            =    3,    ///< Device found during Inquiry.
    BtdrvEventType_InquiryStatus            =    4,    ///< Inquiry status changed.
    BtdrvEventType_PairingPinCodeRequest    =    5,    ///< Pairing PIN code request.
    BtdrvEventType_SspRequest               =    6,    ///< SSP confirm request / SSP passkey notification.
    BtdrvEventType_Connection               =    7,    ///< Connection
    BtdrvEventType_BluetoothCrash           =    13,   ///< BluetoothCrash
} BtdrvEventType;

/// ConnectionEventType
typedef enum {
    BtdrvConnectionEventType_Status              =     0,   ///< BtdrvEventInfo::connection::status
    BtdrvConnectionEventType_SspConfirmRequest   =     1,   ///< SSP confirm request.
    BtdrvConnectionEventType_Suspended           =     2,   ///< ACL Link is now Suspended.
} BtdrvConnectionEventType;

/// ExtEventType
typedef enum {
    BtdrvExtEventType_SetTsi                     =     0,   ///< SetTsi (\ref btdrvSetTsi)
    BtdrvExtEventType_ExitTsi                    =     1,   ///< ExitTsi (\ref btdrvSetTsi)
    BtdrvExtEventType_SetBurstMode               =     2,   ///< SetBurstMode (\ref btdrvEnableBurstMode)
    BtdrvExtEventType_ExitBurstMode              =     3,   ///< ExitBurstMode (\ref btdrvEnableBurstMode)
    BtdrvExtEventType_SetZeroRetransmission      =     4,   ///< \ref btdrvSetZeroRetransmission
    BtdrvExtEventType_PendingConnections         =     5,   ///< \ref btdrvGetPendingConnections
    BtdrvExtEventType_MoveToSecondaryPiconet     =     6,   ///< \ref btdrvMoveToSecondaryPiconet
} BtdrvExtEventType;

/// BluetoothHhReportType
/// Bit0-1 directly control the HID bluetooth transaction report-type value.
/// Bit2-3: these directly control the Parameter Reserved field for SetReport, for GetReport these control the Parameter Reserved and Size bits.
typedef enum {
    BtdrvBluetoothHhReportType_Other        =    0,    ///< Other
    BtdrvBluetoothHhReportType_Input        =    1,    ///< Input
    BtdrvBluetoothHhReportType_Output       =    2,    ///< Output
    BtdrvBluetoothHhReportType_Feature      =    3,    ///< Feature
} BtdrvBluetoothHhReportType;

/// HidEventType
typedef enum {
    BtdrvHidEventType_Connection            =    0,    ///< Connection. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventType_Data                  =    4,    ///< DATA report on the Interrupt channel.
    BtdrvHidEventType_Ext                   =    7,    ///< Response for extensions. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventType_SetReport             =    8,    ///< Response to SET_REPORT.
    BtdrvHidEventType_GetReport             =    9,    ///< Response to GET_REPORT.
} BtdrvHidEventType;

/// This determines the u16 data to write into a CircularBuffer.
typedef enum {
    BtdrvFatalReason_Invalid                =    0,    ///< Only for \ref BtdrvEventInfo: invalid.
    BtdrvFatalReason_Unknown1               =    1,    ///< Can only be triggered by \ref btdrvEmulateBluetoothCrash, not triggered by the sysmodule otherwise.
    BtdrvFatalReason_CommandTimeout         =    2,    ///< HCI command timeout.
    BtdrvFatalReason_HardwareError          =    3,    ///< HCI event HCI_Hardware_Error occurred.
    BtdrvFatalReason_Enable                 =    7,    ///< Only for \ref BtdrvEventInfo: triggered after enabling bluetooth, depending on the value of a global state field.
} BtdrvFatalReason;

/// Address
typedef struct {
    u8 address[0x6];           ///< Address
} BtdrvAddress;

/// AdapterProperty
typedef struct {
    BtdrvAddress addr;         ///< Same as the data for ::BtdrvBluetoothPropertyType_Address.
    u8 class_of_device[0x3];   ///< Same as the data for ::BtdrvBluetoothPropertyType_ClassOfDevice.
    char name[0xF9];           ///< Same as the data for ::BtdrvBluetoothPropertyType_Name (last byte is not initialized).
    u8 feature_set;            ///< Set to hard-coded value 0x68 (same as the data for ::BtdrvBluetoothPropertyType_FeatureSet).
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

/// BtdrvBleAdvertisePacketDataEntry
typedef struct {
    u16 unk_x0;                                      ///< Unknown
    u8 unused[0x12];                                 ///< Unused
} BtdrvBleAdvertisePacketDataEntry;

/// BleAdvertisePacketData
typedef struct {
    u32 unk_x0;                                      ///< Unknown
    u8 unk_x4;                                       ///< Unknown
    u8 size0;                                        ///< Size of the data at unk_x6.
    u8 unk_x6[0x1F];                                 ///< Unknown, see size0.
    u8 pad[3];                                       ///< Padding
    u8 count;                                        ///< Total array entries, see entries.
    u8 pad2[7];                                      ///< Padding
    BtdrvBleAdvertisePacketDataEntry entries[0x5];   ///< \ref BtdrvBleAdvertisePacketDataEntry
    u8 pad3[0x10];                                   ///< Padding
    u8 size2;                                        ///< Size of the data at unk_xA8.
    u8 unk_xA5;                                      ///< Unknown
    u8 pad4[2];                                      ///< Padding
    u8 unk_xA8[0x1F];                                ///< Unknown, see size2.
    u8 unk_xC7;                                      ///< Unknown
    u8 unk_xC8;                                      ///< Unknown
    u8 pad5[3];                                      ///< Padding
} BtdrvBleAdvertisePacketData;

/// BleAdvertiseFilter
typedef struct {
    u8 unk_x0[0x3E];           ///< Unknown
} BtdrvBleAdvertiseFilter;

/// BleAdvertisePacketParameter
typedef struct {
    u8 data[0x8];              ///< Unknown
} BtdrvBleAdvertisePacketParameter;

/// BleScanResult
typedef struct {
    u8 unk_x0;                 ///< Unknown
    BtdrvAddress addr;         ///< \ref BtdrvAddress
    u8 unk_x7[0x139];          ///< Unknown
    s32 unk_x140;              ///< Unknown
    s32 unk_x144;              ///< Unknown
} BtdrvBleScanResult;

/// BleConnectionInfo
typedef struct {
    u32 connection_handle;     ///< ConnectionHandle, 0xFFFFFFFF ([5.0.0-5.0.2] 0xFFFF) is invalid.
    BtdrvAddress addr;         ///< \ref BtdrvAddress
    u8 pad[2];                 ///< Padding
} BtdrvBleConnectionInfo;

/// GattAttributeUuid
typedef struct {
    u32 size;                  ///< UUID size, must be 0x2, 0x4, or 0x10.
    u8 uuid[0x10];             ///< UUID with the above size.
} BtdrvGattAttributeUuid;

/// GattId
typedef struct {
    u8 instance_id;                        ///< InstanceId
    u8 pad[3];                             ///< Padding
    BtdrvGattAttributeUuid uuid;           ///< \ref BtdrvGattAttributeUuid
} BtdrvGattId;

/// LeEventInfo
typedef struct {
    u32 unk_x0;                            ///< Unknown
    u32 unk_x4;                            ///< Unknown
    u8 unk_x8;                             ///< Unknown
    u8 pad[3];                             ///< Padding
    BtdrvGattAttributeUuid uuid0;          ///< \ref BtdrvGattAttributeUuid
    BtdrvGattAttributeUuid uuid1;          ///< \ref BtdrvGattAttributeUuid
    BtdrvGattAttributeUuid uuid2;          ///< \ref BtdrvGattAttributeUuid
    u16 size;                              ///< Size of the below data.
    u8 data[0x3B6];                        ///< Data.
} BtdrvLeEventInfo;

/// BleClientGattOperationInfo
typedef struct {
    u8 unk_x0;                              ///< Converted from BtdrvLeEventInfo::unk_x0.
    u8 pad[3];                              ///< Padding
    u32 unk_x4;                             ///< BtdrvLeEventInfo::unk_x4
    u8 unk_x8;                              ///< BtdrvLeEventInfo::unk_x8
    u8 pad2[3];                             ///< Padding
    BtdrvGattAttributeUuid uuid0;           ///< BtdrvLeEventInfo::uuid0
    BtdrvGattAttributeUuid uuid1;           ///< BtdrvLeEventInfo::uuid1
    BtdrvGattAttributeUuid uuid2;           ///< BtdrvLeEventInfo::uuid2
    u64 size;                               ///< BtdrvLeEventInfo::size
    u8 data[0x200];                         ///< BtdrvLeEventInfo::data
} BtdrvBleClientGattOperationInfo;

