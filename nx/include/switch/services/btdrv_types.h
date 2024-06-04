/**
 * @file btdrv_types.h
 * @brief Bluetooth driver (btdrv) service types (see btdrv.h for the rest).
 * @author yellows8, ndeadly
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "btdrv_ids.h"

/// BluetoothPropertyType [1.0.0-11.0.1]
typedef enum {
    BtdrvBluetoothPropertyType_Name              =     1,    ///< Name. String, max length 0xF8 excluding NUL-terminator.
    BtdrvBluetoothPropertyType_Address           =     2,    ///< \ref BtdrvAddress
    BtdrvBluetoothPropertyType_Unknown3          =     3,    ///< Only available with \ref btdrvSetAdapterProperty. Unknown, \ref BtdrvAddress.
    BtdrvBluetoothPropertyType_ClassOfDevice     =     5,    ///< 3-bytes, Class of Device.
    BtdrvBluetoothPropertyType_FeatureSet        =     6,    ///< 1-byte, FeatureSet. The default is value 0x68.
} BtdrvBluetoothPropertyType;

/// AdapterPropertyType [12.0.0+]
typedef enum {
    BtdrvAdapterPropertyType_Address             =     0,    ///< \ref BtdrvAddress
    BtdrvAdapterPropertyType_Name                =     1,    ///< Name. String, max length 0xF8 excluding NUL-terminator.
    BtdrvAdapterPropertyType_ClassOfDevice       =     2,    ///< 3-bytes, Class of Device.
    BtdrvAdapterPropertyType_Unknown3            =     3,    ///< Only available with \ref btdrvSetAdapterProperty. Unknown, \ref BtdrvAddress.
} BtdrvAdapterPropertyType;

/// EventType
typedef enum {
    ///< BtdrvEventType_* should be used on [12.0.0+]
    BtdrvEventType_InquiryDevice                 =     0,    ///< Device found during Inquiry.
    BtdrvEventType_InquiryStatus                 =     1,    ///< Inquiry status changed.
    BtdrvEventType_PairingPinCodeRequest         =     2,    ///< Pairing PIN code request.
    BtdrvEventType_SspRequest                    =     3,    ///< SSP confirm request / SSP passkey notification.
    BtdrvEventType_Connection                    =     4,    ///< Connection
    BtdrvEventType_Tsi                           =     5,    ///< SetTsi (\ref btdrvSetTsi)
    BtdrvEventType_BurstMode                     =     6,    ///< SetBurstMode (\ref btdrvEnableBurstMode)
    BtdrvEventType_SetZeroRetransmission         =     7,    ///< \ref btdrvSetZeroRetransmission
    BtdrvEventType_PendingConnections            =     8,    ///< \ref btdrvGetPendingConnections
    BtdrvEventType_MoveToSecondaryPiconet        =     9,    ///< \ref btdrvMoveToSecondaryPiconet
    BtdrvEventType_BluetoothCrash                =    10,    ///< BluetoothCrash

    ///< BtdrvEventTypeOld_* should be used on [1.0.0-11.0.1]
    BtdrvEventTypeOld_Unknown0                   =     0,    ///< Unused
    BtdrvEventTypeOld_InquiryDevice              =     3,    ///< Device found during Inquiry.
    BtdrvEventTypeOld_InquiryStatus              =     4,    ///< Inquiry status changed.
    BtdrvEventTypeOld_PairingPinCodeRequest      =     5,    ///< Pairing PIN code request.
    BtdrvEventTypeOld_SspRequest                 =     6,    ///< SSP confirm request / SSP passkey notification.
    BtdrvEventTypeOld_Connection                 =     7,    ///< Connection
    BtdrvEventTypeOld_BluetoothCrash             =    13,    ///< BluetoothCrash
} BtdrvEventType;

/// BtdrvInquiryStatus 
typedef enum {
    BtdrvInquiryStatus_Stopped                   =     0,    ///< Inquiry stopped.
    BtdrvInquiryStatus_Started                   =     1,    ///< Inquiry started.
} BtdrvInquiryStatus;

/// ConnectionEventType
typedef enum {
    BtdrvConnectionEventType_Status              =     0,   ///< BtdrvEventInfo::connection::status
    BtdrvConnectionEventType_SspConfirmRequest   =     1,   ///< SSP confirm request.
    BtdrvConnectionEventType_Suspended           =     2,   ///< ACL Link is now Suspended.
} BtdrvConnectionEventType;

/// ExtEventType [1.0.0-11.0.1]
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
    BtdrvBluetoothHhReportType_Other             =    0,    ///< Other
    BtdrvBluetoothHhReportType_Input             =    1,    ///< Input
    BtdrvBluetoothHhReportType_Output            =    2,    ///< Output
    BtdrvBluetoothHhReportType_Feature           =    3,    ///< Feature
} BtdrvBluetoothHhReportType;

/// HidEventType
typedef enum {
    ///< BtdrvHidEventType_* should be used on [12.0.0+]
    BtdrvHidEventType_Connection         =    0,    ///< Connection. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventType_Data               =    1,    ///< DATA report on the Interrupt channel.
    BtdrvHidEventType_SetReport          =    2,    ///< Response to SET_REPORT.
    BtdrvHidEventType_GetReport          =    3,    ///< Response to GET_REPORT.

    ///< BtdrvHidEventTypeOld_* should be used on [1.0.0-11.0.1]
    BtdrvHidEventTypeOld_Connection      =    0,    ///< Connection. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventTypeOld_Data            =    4,    ///< DATA report on the Interrupt channel.
    BtdrvHidEventTypeOld_Ext             =    7,    ///< Response for extensions. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventTypeOld_SetReport       =    8,    ///< Response to SET_REPORT.
    BtdrvHidEventTypeOld_GetReport       =    9,    ///< Response to GET_REPORT.
} BtdrvHidEventType;

/// HidConnectionStatus [12.0.0+]
typedef enum {
    ///< BtdrvHidConnectionStatus_* should be used on [12.0.0+]
    BtdrvHidConnectionStatus_Closed      =    0,
    BtdrvHidConnectionStatus_Opened      =    1,
    BtdrvHidConnectionStatus_Failed      =    2,

    ///< BtdrvHidConnectionStatusOld_* should be used on [1.0.0-11.0.1]
    BtdrvHidConnectionStatusOld_Opened   =    0,
    BtdrvHidConnectionStatusOld_Closed   =    2,
    BtdrvHidConnectionStatusOld_Failed   =    8,
} BtdrvHidConnectionStatus;

/// This determines the u16 data to write into a CircularBuffer.
typedef enum {
    BtdrvFatalReason_Invalid                =    0,    ///< Only for \ref BtdrvEventInfo: invalid.
    BtdrvFatalReason_Unknown1               =    1,    ///< Can only be triggered by \ref btdrvEmulateBluetoothCrash, not triggered by the sysmodule otherwise.
    BtdrvFatalReason_CommandTimeout         =    2,    ///< HCI command timeout.
    BtdrvFatalReason_HardwareError          =    3,    ///< HCI event HCI_Hardware_Error occurred.
    BtdrvFatalReason_Enable                 =    7,    ///< Only for \ref BtdrvEventInfo: triggered after enabling bluetooth, depending on the value of a global state field.
    BtdrvFatalReason_Audio                  =    9,    ///< [12.0.0+] Only for \ref BtdrvEventInfo: triggered by Audio cmds in some cases.
} BtdrvFatalReason;

/// BleEventType
typedef enum {
    BtdrvBleEventType_ClientRegistration            =    0,    ///< GATT client registration.
    BtdrvBleEventType_ServerRegistration            =    1,    ///< GATT server registration.
    BtdrvBleEventType_ConnectionUpdate              =    2,    ///< Connection update.
    BtdrvBleEventType_PreferredConnectionParameters =    3,    ///< Preferred connection parameters.
    BtdrvBleEventType_ClientConnection              =    4,    ///< GATT client connection.
    BtdrvBleEventType_ServerConnection              =    5,    ///< GATT server connection.
    BtdrvBleEventType_ScanResult                    =    6,    ///< Scan result.
    BtdrvBleEventType_ScanFilter                    =    7,    ///< Scan filter status.
    BtdrvBleEventType_ClientNotify                  =    8,    ///< GATT client notify.
    BtdrvBleEventType_ClientCacheSave               =    9,    ///< GATT client cache save.
    BtdrvBleEventType_ClientCacheLoad               =   10,    ///< GATT client cache load.
    BtdrvBleEventType_ClientConfigureMtu            =   11,    ///< GATT client configure MTU.
    BtdrvBleEventType_ServerAddAttribute            =   12,    ///< GATT server add attribute.
    BtdrvBleEventType_ServerAttributeOperation      =   13,    ///< GATT server attribute operation.
} BtdrvBleEventType;

/// GattAttributeType
typedef enum {
    BtdrvGattAttributeType_IncludedService  =    0,    ///< Included service
    BtdrvGattAttributeType_Characteristic   =    1,    ///< Characteristic
    BtdrvGattAttributeType_Descriptor       =    2,    ///< Descriptor
    BtdrvGattAttributeType_Service          =    3,    ///< Service
} BtdrvGattAttributeType;

/// GattAttributePermission
typedef enum {
    BtdrvGattAttributePermission_Read                    = BIT(0),
    BtdrvGattAttributePermission_ReadEncrypted           = BIT(1),
    BtdrvGattAttributePermission_ReadEncryptedMitm       = BIT(2),
    BtdrvGattAttributePermission_Write                   = BIT(4),
    BtdrvGattAttributePermission_WriteEncrypted          = BIT(5),
    BtdrvGattAttributePermission_WriteEncryptedMitm      = BIT(6),
    BtdrvGattAttributePermission_WriteSigned             = BIT(7),
    BtdrvGattAttributePermission_WriteSignedMitm         = BIT(8),

    BtdrvGattAttributePermission_ReadAllowed             = BtdrvGattAttributePermission_Read | BtdrvGattAttributePermission_ReadEncrypted | BtdrvGattAttributePermission_ReadEncryptedMitm,
    BtdrvGattAttributePermission_ReadAuthRequired        = BtdrvGattAttributePermission_ReadEncrypted,
    BtdrvGattAttributePermission_ReadMitmRequired        = BtdrvGattAttributePermission_ReadEncryptedMitm,
    BtdrvGattAttributePermission_ReadEncryptedRequired   = BtdrvGattAttributePermission_ReadEncrypted | BtdrvGattAttributePermission_ReadEncryptedMitm,

    BtdrvGattAttributePermission_WriteAllowed            = BtdrvGattAttributePermission_Write | BtdrvGattAttributePermission_WriteEncrypted | BtdrvGattAttributePermission_WriteEncryptedMitm | BtdrvGattAttributePermission_WriteSigned | BtdrvGattAttributePermission_WriteSignedMitm,
    BtdrvGattAttributePermission_WriteAuthRequired       = BtdrvGattAttributePermission_WriteEncrypted | BtdrvGattAttributePermission_WriteSigned,
    BtdrvGattAttributePermission_WriteMitmRequired       = BtdrvGattAttributePermission_WriteEncryptedMitm | BtdrvGattAttributePermission_WriteSignedMitm,
    BtdrvGattAttributePermission_WriteEncryptedRequired  = BtdrvGattAttributePermission_WriteEncrypted | BtdrvGattAttributePermission_WriteEncryptedMitm,
    BtdrvGattAttributePermission_WriteSignedRequired     = BtdrvGattAttributePermission_WriteSigned | BtdrvGattAttributePermission_WriteSignedMitm,
} BtdrvGattAttributePermission;

/// GattCharacteristicProperty
typedef enum {
    BtdrvGattCharacteristicProperty_Broadcast            = BIT(0),
    BtdrvGattCharacteristicProperty_Read                 = BIT(1),
    BtdrvGattCharacteristicProperty_WriteNoResponse      = BIT(2),
    BtdrvGattCharacteristicProperty_Write                = BIT(3),
    BtdrvGattCharacteristicProperty_Notify               = BIT(4),
    BtdrvGattCharacteristicProperty_Indicate             = BIT(5),
    BtdrvGattCharacteristicProperty_Authentication       = BIT(6),
    BtdrvGattCharacteristicProperty_ExtendedProperties   = BIT(7),
} BtdrvGattCharacteristicProperty;

/// GattAuthReqType
typedef enum {
    BtdrvGattAuthReqType_None           =    0,
    BtdrvGattAuthReqType_NoMitm         =    1,
    BtdrvGattAuthReqType_Mitm           =    2,
    BtdrvGattAuthReqType_SignedNoMitm   =    3,
    BtdrvGattAuthReqType_SignedMitm     =    4,
} BtdrvGattAuthReqType;

/// BtdrvBleAdBit
typedef enum {
    BtdrvBleAdBit_DeviceName    =  BIT(0),
    BtdrvBleAdBit_Flags         =  BIT(1),
    BtdrvBleAdBit_Manufacturer  =  BIT(2),
    BtdrvBleAdBit_TxPower       =  BIT(3),
    BtdrvBleAdBit_Service32     =  BIT(4),
    BtdrvBleAdBit_IntRange      =  BIT(5),
    BtdrvBleAdBit_Service       =  BIT(6),
    BtdrvBleAdBit_ServiceSol    =  BIT(7),
    BtdrvBleAdBit_ServiceData   =  BIT(8),
    BtdrvBleAdBit_SignData      =  BIT(9),
    BtdrvBleAdBit_Service128Sol = BIT(10),
    BtdrvBleAdBit_Appearance    = BIT(11),
    BtdrvBleAdBit_PublicAddress = BIT(12),
    BtdrvBleAdBit_RandomAddress = BIT(13),
    BtdrvBleAdBit_Service32Sol  = BIT(14),
    BtdrvBleAdBit_Proprietary   = BIT(15),
    BtdrvBleAdBit_Service128    = BIT(16),
} BtdrvBleAdBit;

/// BtdrvBleAdFlag
typedef enum {
    BtdrvBleAdFlag_None                         = 0,
    BtdrvBleAdFlag_LimitedDiscovery             = BIT(0),
    BtdrvBleAdFlag_GeneralDiscovery             = BIT(1),
    BtdrvBleAdFlag_BrEdrNotSupported            = BIT(2),
    BtdrvBleAdFlag_DualModeControllerSupport    = BIT(3),
    BtdrvBleAdFlag_DualModeHostSupport          = BIT(4),
} BtdrvBleAdFlag;

/// AudioEventType
typedef enum {
    BtdrvAudioEventType_None                =     0,   ///< None
    BtdrvAudioEventType_Connection          =     1,   ///< Connection
} BtdrvAudioEventType;

/// AudioOutState
typedef enum {
    BtdrvAudioOutState_Stopped              =     0,   ///< Stopped
    BtdrvAudioOutState_Started              =     1,   ///< Started
} BtdrvAudioOutState;

/// AudioCodec
typedef enum {
    BtdrvAudioCodec_Pcm                     =     0,   ///< Raw PCM
} BtdrvAudioCodec;

/// Address
typedef struct {
    u8 address[0x6];           ///< Address
} BtdrvAddress;

/// ClassOfDevice
typedef struct {
    u8 class_of_device[0x3];   ///< ClassOfDevice
} BtdrvClassOfDevice;

/// AdapterProperty [1.0.0-11.0.1]
typedef struct {
    BtdrvAddress addr;                      ///< Same as the data for ::BtdrvBluetoothPropertyType_Address.
    BtdrvClassOfDevice class_of_device;     ///< Same as the data for ::BtdrvBluetoothPropertyType_ClassOfDevice.
    char name[0xF9];                        ///< Same as the data for ::BtdrvBluetoothPropertyType_Name (last byte is not initialized).
    u8 feature_set;                         ///< Set to hard-coded value 0x68 (same as the data for ::BtdrvBluetoothPropertyType_FeatureSet).
} BtdrvAdapterPropertyOld;

/// AdapterProperty [12.0.0+]
typedef struct {
    u8 type;                                ///< \ref BtdrvAdapterPropertyType
    u8 size;                                ///< Data size.
    u8 data[0x100];                         ///< Data (above size), as specified by the type.
} BtdrvAdapterProperty;

/// AdapterPropertySet [12.0.0+]
typedef struct {
    BtdrvAddress addr;                      ///< Same as the data for ::BtdrvBluetoothPropertyType_Address.
    BtdrvClassOfDevice class_of_device;     ///< Same as the data for ::BtdrvBluetoothPropertyType_ClassOfDevice.
    char name[0xF9];                        ///< Same as the data for ::BtdrvBluetoothPropertyType_Name.
} BtdrvAdapterPropertySet;

/// BluetoothPinCode [1.0.0-11.0.1]
typedef struct {
    char code[0x10];           ///< PinCode
} BtdrvBluetoothPinCode;

/// BtdrvPinCode [12.0.0+]
typedef struct {
    char code[0x10];           ///< PinCode
    u8 length;                 ///< Length 
} BtdrvPinCode;

/// HidData [1.0.0-8.1.1]
typedef struct {
    u16 size;                  ///< Size of data.
    u8 data[0x280];            ///< Data
} BtdrvHidData;

/// HidReport [9.0.0+].
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

/// GattAttribute
typedef struct {
    BtdrvGattId id;                        ///< \ref BtdrvGattId
    u16 type;                              ///< \ref BtdrvGattAttributeType
    u16 handle;
    u16 group_end_handle;
    u8 property;                           ///< Only used when type is characteristic. \ref BtdrvGattCharacteristicProperty
    bool is_primary;                       ///< Only used when type is service
} BtdrvGattAttribute;

/// LeConnectionParams [5.0.0-8.1.1]
typedef struct {
    BtdrvAddress addr;         ///< \ref BtdrvAddress
    u16 min_conn_interval;     ///< Minimum connection interval
    u16 max_conn_interval;     ///< Maximum connection interval
    u16 scan_interval;         ///< Scan interval
    u16 scan_window;           ///< Scan window
    u16 slave_latency;         ///< Slave latency
    u16 supervision_tout;      ///< Connection supervision timeout multiplier
    u8 preference;             ///< Unused
    u8 pad;                    ///< Padding
} BtdrvLeConnectionParams;

/// BleConnectionParameter [9.0.0+]
typedef struct {
    u16 min_conn_interval;     ///< Minimum connection interval
    u16 max_conn_interval;     ///< Maximum connection interval
    u16 scan_interval;         ///< Scan interval
    u16 scan_window;           ///< Scan window
    u16 slave_latency;         ///< Slave latency
    u16 supervision_tout;      ///< Connection supervision timeout multiplier
} BtdrvBleConnectionParameter;

/// BtdrvBleAdvertisePacketData
typedef struct {
    u32 adv_data_mask;                      ///< Bitmask of following AD data to be included in advertising packets \ref BtdrvBleAdBit
    u8 flag;                                ///< AD flag value to be advertised \ref BtdrvBleAdFlag. Included with BtdrvBleAdBit_Flags
    u8 manu_data_len;                       ///< Size of manu_data below
    u8 manu_data[0x1F];                     ///< Manufacturer-specific data to be advertised. Included with BtdrvBleAdBit_Manufacturer
    u8 pad[1];                              ///< Padding
    u16 appearance_data;                    ///< Device appearance data to be advertised \ref BtdrvAppearanceType. Included with BtdrvBleAdBit_Appearance
    u8 num_service;                         ///< Number of services in uuid_val array below
    u8 pad2[3];                             ///< Padding
    BtdrvGattAttributeUuid uuid_val[0x6];   ///< Array of 16-bit UUIDs to be advertised \ref BtdrvGattAttributeUuid. Included with BtdrvBleAdBit_Service
    u8 service_data_len;                    ///< Size of service_data below
    u8 pad3[1];                             ///< Padding
    u16 service_data_uuid;                  ///< 16-bit UUID of service_data below
    u8 service_data[0x1F];                  ///< Service data to be advertised. Included with BtdrvBleAdBit_ServiceData
    bool is_scan_rsp;                       ///< Whether this is an inquiry scan response or advertising data
    u8 tx_power;                            ///< Inquiry transmit power to be advertised. Included with BtdrvBleAdBit_TxPower
    u8 pad4[3];                             ///< Padding
} BtdrvBleAdvertisePacketData;

/// BleAdvertisement
typedef struct {
    u8 size;                    ///< Size of data below.
    u8 type;                    ///< \ref BtdrvAdType
    u8 data[0x1D];              ///< Advertisement data
} BtdrvBleAdvertisement;

/// BleAdvertiseFilter
typedef struct {
    u8 index;                   ///< Filter index
    BtdrvBleAdvertisement adv;  ///< \ref BtdrvBleAdvertisement
    u8 mask[0x1D];              ///< Mask for advertisement data above
    u8 mask_size;               ///< Size of mask above.
} BtdrvBleAdvertiseFilter;

/// BleAdvertisePacketParameter
typedef struct {
    u16 company_id;
    u8 pattern_data[6];
} BtdrvBleAdvertisePacketParameter;

/// BleScanResult
typedef struct {
    u8 unk_x0;                 ///< Unknown
    BtdrvAddress addr;         ///< \ref BtdrvAddress
    u8 unk_x7[0x139];          ///< Unknown
    s32 count;                 ///< Unknown
    s32 unk_x144;              ///< Unknown
} BtdrvBleScanResult;

/// BleConnectionInfo
typedef struct {
    u32 connection_handle;     ///< ConnectionHandle, 0xFFFFFFFF ([5.0.0-5.0.2] 0xFFFF) is invalid.
    BtdrvAddress addr;         ///< \ref BtdrvAddress
    u8 pad[2];                 ///< Padding
} BtdrvBleConnectionInfo;

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

/// PcmParameter
typedef struct {
    u32 unk_x0;                             ///< Must be 0-3. Controls number of channels: 0 = mono, non-zero = stereo.
    s32 sample_rate;                        ///< Sample rate. Must be one of the following: 16000, 32000, 44100, 48000.
    u32 bits_per_sample;                    ///< Bits per sample. Must be 8 or 16.
} BtdrvPcmParameter;

/// AudioControlButtonState
typedef struct {
    u8 unk_x0[0x10];                        ///< Unknown
} BtdrvAudioControlButtonState;

