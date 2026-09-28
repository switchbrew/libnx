/**
 * @file btm_types.h
 * @brief btm service types.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// BtmState
typedef enum {
    BtmState_NotInitialized        = 0,    ///< NotInitialized
    BtmState_RadioOff              = 1,    ///< RadioOff
    BtmState_MinorSlept            = 2,    ///< MinorSlept
    BtmState_RadioOffMinorSlept    = 3,    ///< RadioOffMinorSlept
    BtmState_Slept                 = 4,    ///< Slept
    BtmState_RadioOffSlept         = 5,    ///< RadioOffSlept
    BtmState_Initialized           = 6,    ///< Initialized
    BtmState_Working               = 7,    ///< Working
} BtmState;

/// BluetoothMode
typedef enum {
    BtmBluetoothMode_Dynamic2Slot  = 0,    ///< Dynamic2Slot
    BtmBluetoothMode_StaticJoy     = 1,    ///< StaticJoy
} BtmBluetoothMode;

/// WlanMode
typedef enum {
    BtmWlanMode_Local4             = 0,    ///< Local4
    BtmWlanMode_Local8             = 1,    ///< Local8
    BtmWlanMode_None               = 2,    ///< None
} BtmWlanMode;

/// TsiMode
typedef enum {
    BtmTsiMode_0Fd3Td3Si10         = 0,    ///< 0Fd3Td3Si10
    BtmTsiMode_1Fd1Td1Si5          = 1,    ///< 1Fd1Td1Si5
    BtmTsiMode_2Fd1Td3Si10         = 2,    ///< 2Fd1Td3Si10
    BtmTsiMode_3Fd1Td5Si15         = 3,    ///< 3Fd1Td5Si15
    BtmTsiMode_4Fd3Td1Si10         = 4,    ///< 4Fd3Td1Si10
    BtmTsiMode_5Fd3Td3Si15         = 5,    ///< 5Fd3Td3Si15
    BtmTsiMode_6Fd5Td1Si15         = 6,    ///< 6Fd5Td1Si15
    BtmTsiMode_7Fd1Td3Si15         = 7,    ///< 7Fd1Td3Si15
    BtmTsiMode_8Fd3Td1Si15         = 8,    ///< 8Fd3Td1Si15
    BtmTsiMode_9Fd1Td1Si10         = 9,    ///< 9Fd1Td1Si10
    BtmTsiMode_10Fd1Td1Si15        = 10,   ///< 10Fd1Td1Si15
    BtmTsiMode_Active              = 255,  ///< Active
} BtmTsiMode;

typedef enum {
    BtmSniffMode_5ms               = 0,    ///< 5ms
    BtmSniffMode_10ms              = 1,    ///< 10ms
    BtmSniffMode_15ms              = 2,    ///< 15ms
    BtmSniffMode_Active            = 3,    ///< Active
} BtmSniffMode;

/// SlotMode
typedef enum {
    BtmSlotMode_2                  = 0,    ///< 2
    BtmSlotMode_4                  = 1,    ///< 4
    BtmSlotMode_6                  = 2,    ///< 6
    BtmSlotMode_Active             = 3,    ///< Active
} BtmSlotMode;

/// Profile
typedef enum {
    BtmProfile_None                = 0,    ///< None
    BtmProfile_Hid                 = 1,    ///< Hid
    BtmProfile_Audio               = 2,    ///< [13.0.0+] Audio
} BtmProfile;

/// BdName
typedef struct {
    char name[0x20];           ///< Name string.
} BtmBdName;

/// ClassOfDevice
typedef struct {
    u8 class_of_device[0x3];   ///< ClassOfDevice
} BtmClassOfDevice;

/// LinkKey
typedef struct {
    u8 link_key[0x10];         ///< LinkKey
} BtmLinkKey;

/// ZeroRetransmissionList
typedef struct {
    u8 enabled_report_id_count;   ///< EnabledReportIdCount
    u8 enabled_report_id[0x10];   ///< Array of EnabledReportId.
} BtmZeroRetransmissionList;

/// HidDeviceCondition
typedef struct {
    u32 sniff_mode;                                         ///< \ref BtmSniffMode
    u32 slot_mode;                                          ///< \ref BtmSlotMode
    bool is_burst_mode;                                     ///< IsBurstMode
    BtmZeroRetransmissionList zero_retransmission_list;     ///< \ref BtmZeroRetransmissionList
    u16 vid;                                                ///< Vid
    u16 pid;                                                ///< Pid
} BtmHidDeviceCondition;

/// AudioDeviceCondition [13.0.0+]
typedef struct {
    u8 unk_x0;                                    ///< Unknown
} BtmAudioDeviceCondition;

/// HidDeviceInfo
typedef struct {
    u16 vid;                                      ///< Vid
    u16 pid;                                      ///< Pid
} BtmHidDeviceInfo;

/// AudioDeviceInfo [13.0.0+]
typedef struct {
    union {
        struct {
            u8 source_volume;                             ///< SourceVolume
        } v13;                                            ///< [13.0.0-13.2.1]

        struct {
            u8 source_volume;                             ///< SourceVolume
            bool is_first_audio_control_connection;       ///< IsFirstAudioControlConnection
            u8 unk_0x2;                                   ///< Unknown
        } v14;                                            ///< [14.0.0-14.1.2]

        struct {
            u8 source_volume;                             ///< SourceVolume
            u8 sink_volume;                               ///< SinkVolume
            bool is_first_audio_control_connection;       ///< IsFirstAudioControlConnection
            bool unk_0x3;                                 ///< Unknown     
        } v15;                                            ///< [15.0.0+]
    };
} BtmAudioDeviceInfo;

/// HostDeviceProperty
typedef struct {
    union {
        struct {
            BtdrvAddress addr;                    ///< Same as BtdrvAdapterProperty::addr.
            BtmClassOfDevice class_of_device;     ///< Same as BtdrvAdapterProperty::class_of_device.
            BtmBdName name;                       ///< Same as BtdrvAdapterProperty::name (except the last byte which is always zero).
            u8 feature_set;                       ///< Same as BtdrvAdapterProperty::feature_set.
        } v1;                                     ///< [1.0.0-12.1.0]

        struct {
            BtdrvAddress addr;                    ///< Same as BtdrvAdapterProperty::addr.
            BtmClassOfDevice class_of_device;     ///< Same as BtdrvAdapterProperty::class_of_device.
            char name[0xF9];                      ///< Same as BtdrvAdapterProperty::name (except the last byte which is always zero).
            u8 feature_set;                       ///< Same as BtdrvAdapterProperty::feature_set.
        } v13;                                    ///< [13.0.0+]
    };
} BtmHostDeviceProperty;

/// DeviceCondition [1.0.0-12.1.0]
typedef struct {
    BtdrvAddress address;                         ///< \ref BtdrvAddress
    u8 pad[2];                                    ///< Padding
    u32 profile;                                  ///< \ref BtmProfile
    char name[0x20];                              ///< BdName
    union {
        u8 data[0x20];                            ///< Empty (Profile = None)
        BtmHidDeviceCondition hid;                ///< \ref BtmHidDeviceCondition (Profile = Hid)
    } profile_condition;
    u8 reserved[0x20];                            ///< Reserved
} BtmDeviceConditionLegacy;

/// DeviceCondition [13.0.0+]
typedef struct {
    BtdrvAddress address;                         ///< \ref BtdrvAddress
    u8 pad[2];                                    ///< Padding
    u32 profile;                                  ///< \ref BtmProfile
    union {
        u8 data[0x20];                            ///< Empty (Profile = None)
        BtmHidDeviceCondition hid;                ///< \ref BtmHidDeviceCondition (Profile = Hid)
        BtmAudioDeviceCondition audio;            ///< \ref BtmAudioDeviceCondition (Profile = Audio)
    } profile_condition;
    u8 reserved[0x20];                            ///< Reserved
    char name[0xF9];                              ///< Name
    u8 pad2[3];                                   ///< Padding
} BtmDeviceCondition;

/// DeviceConditionList [1.0.0-5.0.2]
typedef struct {
    u32 bluetooth_mode;                     ///< \ref BtmBluetoothMode
    u32 wlan_mode;                          ///< \ref BtmWlanMode
    bool is_slot_saving_for_pairing;        ///< IsSlotSavingForPairing
    bool is_slot_saving;                    ///< IsSlotSaving
    u8 connection_capacity;                 ///< ConnectionCapacity
    u8 device_count;                        ///< DeviceCount
    BtmDeviceConditionLegacy devices[8];    ///< Array of \ref BtmDeviceConditionLegacy with the above count.
} BtmDeviceConditionListV100;

/// DeviceConditionList [5.1.0-7.0.1]
typedef struct {
    u32 bluetooth_mode;                     ///< \ref BtmBluetoothMode
    u32 wlan_mode;                          ///< \ref BtmWlanMode
    bool is_slot_saving_for_pairing;        ///< IsSlotSavingForPairing
    bool is_slot_saving;                    ///< IsSlotSaving
    u8 unk_0xA;                             ///< Unknown
    u8 connection_capacity;                 ///< ConnectionCapacity
    u8 device_count;                        ///< DeviceCount
    u8 pad[3];                              ///< Padding
    BtmDeviceConditionLegacy devices[8];    ///< Array of \ref BtmDeviceConditionLegacy with the above count.
} BtmDeviceConditionListV510;

/// DeviceConditionList [8.0.0-8.1.1]
typedef struct {
    u32 bluetooth_mode;                     ///< \ref BtmBluetoothMode
    u32 wlan_mode;                          ///< \ref BtmWlanMode
    bool is_slot_saving_for_pairing;        ///< IsSlotSavingForPairing
    bool is_slot_saving;                    ///< IsSlotSaving
    u8 connection_capacity;                 ///< ConnectionCapacity
    u8 device_count;                        ///< DeviceCount
    BtmDeviceConditionLegacy devices[8];    ///< Array of \ref BtmDeviceConditionLegacy with the above count.
} BtmDeviceConditionListV800;

/// DeviceConditionList [9.0.0-12.1.0]
typedef struct {
    u32 wlan_mode;                          ///< \ref BtmWlanMode
    bool is_slot_saving_for_pairing;        ///< IsSlotSavingForPairing
    bool is_slot_saving;                    ///< IsSlotSaving
    u8 connection_capacity;                 ///< ConnectionCapacity
    u8 device_count;                        ///< DeviceCount
    BtmDeviceConditionLegacy devices[8];    ///< Array of \ref BtmDeviceConditionLegacy with the above count.
} BtmDeviceConditionListV900;

/// DeviceConditionList [1.0.0-12.1.0]
typedef union {
    BtmDeviceConditionListV100 v100;
    BtmDeviceConditionListV510 v510;
    BtmDeviceConditionListV800 v800;
    BtmDeviceConditionListV900 v900;
} BtmDeviceConditionList;

/// DeviceSlotMode
typedef struct {
    BtdrvAddress addr;            ///< \ref BtdrvAddress
    u8 reserved[2];               ///< Reserved
    u32 slot_mode;                ///< \ref BtmSlotMode
} BtmDeviceSlotMode;

/// DeviceSlotModeList
typedef struct {
    u8 device_count;              ///< DeviceCount
    u8 reserved[3];               ///< Reserved
    BtmDeviceSlotMode devices[8]; ///< Array of \ref BtmDeviceSlotMode with the above count.
} BtmDeviceSlotModeList;

/// DeviceInfo [1.0.0-12.1.0]
typedef struct {
    BtdrvAddress addr;                    ///< \ref BtdrvAddress
    BtmClassOfDevice class_of_device;     ///< ClassOfDevice
    BtmBdName name;                       ///< BdName
    BtmLinkKey link_key;                  ///< LinkKey
    u8 reserved[3];                       ///< Reserved
    u32 profile;                          ///< \ref BtmProfile
    union {
        u8 data[0x20];                    ///< Empty (Profile = None)
        BtmHidDeviceInfo hid;             ///< \ref BtmHidDeviceInfo (Profile = Hid)
    } profile_info;
} BtmDeviceInfoLegacy;

/// DeviceInfo [13.0.0+]
typedef struct {
    BtdrvAddress addr;                    ///< \ref BtdrvAddress
    BtmClassOfDevice class_of_device;     ///< ClassOfDevice
    BtmLinkKey link_key;                  ///< LinkKey
    u8 key_type;                          ///< KeyType
    u8 reserved[2];                       ///< Reserved
    u32 profile;                          ///< \ref BtmProfile
    union {
        u8 data[0x20];                    ///< Empty (Profile = None)
        BtmHidDeviceInfo hid;             ///< \ref BtmHidDeviceInfo (Profile = Hid)
        BtmAudioDeviceInfo audio;         ///< \ref BtmAudioDeviceInfo (Profile = Audio)
    } profile_info;
    char name[0xF9];                      ///< Name
    u8 pad[3];                            ///< Padding
} BtmDeviceInfo;

/// DeviceInfoList
typedef struct {
    u8 device_count;                      ///< DeviceCount
    u8 reserved[3];                       ///< Reserved
    BtmDeviceInfoLegacy devices[10];      ///< Array of \ref BtmDeviceInfoLegacy with the above count.
} BtmDeviceInfoList;

/// DeviceProperty
typedef struct {
    BtdrvAddress addr;                    ///< \ref BtdrvAddress
    BtmClassOfDevice class_of_device;     ///< ClassOfDevice
    BtmBdName name;                       ///< BdName
} BtmDeviceProperty;

/// DevicePropertyList
typedef struct {
    u8 device_count;                ///< DeviceCount
    BtmDeviceProperty devices[15];  ///< Array of \ref BtmDeviceProperty.
} BtmDevicePropertyList;

/// GattClientConditionList
typedef struct {
    u8 unk_x0[0x74];              ///< Unknown
} BtmGattClientConditionList;

/// GattService
typedef struct {
    u8 unk_x0[0x4];               ///< Unknown
    BtdrvGattAttributeUuid uuid;  ///< \ref BtdrvGattAttributeUuid
    u16 handle;                   ///< Handle
    u8 unk_x1A[0x2];              ///< Unknown
    u16 instance_id;              ///< InstanceId
    u16 end_group_handle;         ///< EndGroupHandle
    u8 primary_service;           ///< PrimaryService
    u8 pad[3];                    ///< Padding
} BtmGattService;

/// GattCharacteristic
typedef struct {
    u8 unk_x0[0x4];               ///< Unknown
    BtdrvGattAttributeUuid uuid;  ///< \ref BtdrvGattAttributeUuid
    u16 handle;                   ///< Handle
    u8 unk_x1A[0x2];              ///< Unknown
    u16 instance_id;              ///< InstanceId
    u8 properties;                ///< Properties
    u8 unk_x1F[0x5];              ///< Unknown
} BtmGattCharacteristic;

/// GattDescriptor
typedef struct {
    u8 unk_x0[0x4];               ///< Unknown
    BtdrvGattAttributeUuid uuid;  ///< \ref BtdrvGattAttributeUuid
    u16 handle;                   ///< Handle
    u8 unk_x1A[0x6];              ///< Unknown
} BtmGattDescriptor;

/// BleDataPath
typedef struct {
    u8 unk_x0;                    ///< Unknown
    u8 pad[3];                    ///< Padding
    BtdrvGattAttributeUuid uuid;  ///< \ref BtdrvGattAttributeUuid
} BtmBleDataPath;

/// AudioDevice
typedef struct {
    BtdrvAddress addr;            ///< Device address
    char name[0xF9];              ///< Device name
} BtmAudioDevice;
