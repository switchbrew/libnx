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
    BtdrvHidEventType_Unknown0              =    0,    ///< Unknown. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventType_Unknown4              =    4,    ///< Unknown.
    BtdrvHidEventType_Unknown7              =    7,    ///< Unknown. Only used with \ref btdrvGetHidEventInfo.
    BtdrvHidEventType_Unknown8              =    8,    ///< Unknown.
    BtdrvHidEventType_Unknown9              =    9,    ///< Unknown.
} BtdrvHidEventType;

/// This determines the u16 data to write into the CircularBuffer (name "BLE CORE").
typedef enum {
    BtdrvFatalReason_Unknown1               =    1,    ///< u16 data = 0x850.
    BtdrvFatalReason_Unknown2               =    2,    ///< u16 data = 0x851.
    BtdrvFatalReason_Unknown3               =    3,    ///< Reason values which aren't 1/2: u16 data = 0x852.
} BtdrvFatalReason;

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

/// Data for \ref btdrvGetHidReportEventInfo. The data stored here depends on the \ref BtdrvHidEventType.
typedef struct {
    union {
        u8 data[0x480];                  ///< Raw data.

        struct {
            u32 unk_x0;                  ///< Always 0.
            u8 unk_x4;                   ///< Always 0.
            BtdrvAddress addr;           ///< \ref BtdrvAddress
            u8 pad;                      ///< Padding
            u16 size;                    ///< Size of the below data.
            u8 data[];                   ///< Data.
        } type4;                         ///< ::BtdrvHidEventType_Unknown4

        struct {
            union {
                u8 data[0xC];                ///< Raw data.

                struct {
                    u32 res;                 ///< 0 = success, non-zero = error.
                    BtdrvAddress addr;       ///< \ref BtdrvAddress
                    u8 pad[2];               ///< Padding
                };
            };
        } type8;                             ///< ::BtdrvHidEventType_Unknown8

        struct {
            union {
                union {
                    u8 rawdata[0x290];           ///< Raw data.

                    struct {
                        BtdrvAddress addr;       ///< \ref BtdrvAddress
                        u8 pad[2];               ///< Padding
                        u32 unk_x0;              ///< Unknown. hid-sysmodule only uses the below data when this field is 0.
                        BtdrvHidData data;       ///< \ref BtdrvHidData
                        u8 pad2[2];              ///< Padding
                    };
                } hid_data;                      ///< Pre-9.0.0

                union {
                    u8 rawdata[0x2C8];           ///< Raw data.

                    struct {
                        u32 unk_x0;              ///< Unknown. hid-sysmodule only uses the below report when this field is 0.
                        BtdrvAddress addr;       ///< \ref BtdrvAddress
                        BtdrvHidReport report;   ///< \ref BtdrvHidReport
                    };
                } hid_report;                    ///< [9.0.0+]
            };
        } type9;                                 ///< ::BtdrvHidEventType_Unknown9
    };
} BtdrvHidReportEventInfo;

/// The raw sharedmem data for HidReportEventInfo.
typedef struct {
    struct {
        u8 type;                                 ///< \ref BtdrvHidEventType
        u8 pad[7];
        u64 tick;
        u64 size;
    } hdr;

    union {
        struct {
            struct {
                u8 unused[0x3];                  ///< Unused
                BtdrvAddress addr;               ///< \ref BtdrvAddress
                u8 unused2[0x3];                 ///< Unused
                u16 size;                        ///< Size of the below data.
                u8 data[];                       ///< Data.
            } v1;                                ///< Pre-9.0.0

            struct {
                u8 unused[0x5];                  ///< Unused
                BtdrvAddress addr;               ///< \ref BtdrvAddress
                u8 pad;                          ///< Padding
                u16 size;                        ///< Size of the below data.
                u8 data[];                       ///< Data.
            } v9;                                ///< [9.0.0+]
        } type4;                                 ///< ::BtdrvHidEventType_Unknown4

        struct {
            u8 data[0xC];                        ///< Raw data.
        } type8;                                 ///< ::BtdrvHidEventType_Unknown8

        struct {
            union {
                struct {
                    u8 rawdata[0x290];           ///< Raw data.
                } hid_data;                      ///< Pre-9.0.0

                struct {
                    u8 rawdata[0x2C8];           ///< Raw data.
                } hid_report;                    ///< [9.0.0+]
            };
        } type9;                                 ///< ::BtdrvHidEventType_Unknown9
    } data;
} BtdrvHidReportEventInfoBufferData;

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
    u8 unk_x0[0x148];          ///< Unknown
} BtdrvBleScanResult;

/// BleConnectionInfo
typedef struct {
    u32 id;                    ///< Id, 0xFFFFFFFF ([5.0.0-5.0.2] 0xFFFF) is invalid.
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
    u8 unk_x0[0x18];           ///< Unknown
} BtdrvGattId;

/// CircularBuffer
typedef struct {
    Mutex mutex;
    void* event_type;          ///< Not set with sharedmem.
    u8 data[0x2710];
    s32 write_offset;
    s32 read_offset;
    u64 utilization;
    char name[0x11];
    u8 initialized;
} BtdrvCircularBuffer;

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
 * @brief GetEventInfo
 * @note This is used by btm-sysmodule.
 * @param[out] buffer Output buffer. 0x400-bytes from state is written here.
 * @param[in] size Output buffer size.
 * @oaram[out] type Output EventType.
 */
Result btdrvGetEventInfo(void* buffer, size_t size, u32 *type);

/**
 * @brief This sends a HID DATA transaction packet with report-type Output.
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
 * @brief This sends a HID SET_REPORT transaction packet.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] type \ref BtdrvBluetoothHhReportType
 * @param[in] buffer Input \ref BtdrvHidReport, on pre-9.0.0 this is \ref BtdrvHidData.
 */
Result btdrvSetHidReport(BtdrvAddress addr, BtdrvBluetoothHhReportType type, BtdrvHidReport *buffer);

/**
 * @brief This sends a HID GET_REPORT transaction packet.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] report_id This is sent in the packet for the Report Id, when non-zero.
 * @param[in] type \ref BtdrvBluetoothHhReportType
 */
Result btdrvGetHidReport(BtdrvAddress addr, u8 report_id, BtdrvBluetoothHhReportType type);

/**
 * @brief TriggerConnection
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] unk [9.0.0+] Unknown
 */
Result btdrvTriggerConnection(BtdrvAddress addr, u16 unk);

/**
 * @brief GetHidEventInfo
 * @note This is used by btm-sysmodule.
 * @param[out] buffer Output buffer. 0x480-bytes from state is written here.
 * @param[in] size Output buffer size.
 * @param[out] type \ref BtdrvHidEventType, always ::BtdrvHidEventType_Unknown0 or ::BtdrvHidEventType_Unknown7.
 */
Result btdrvGetHidEventInfo(void* buffer, size_t size, BtdrvHidEventType *type);

/**
 * @brief RegisterHidReportEvent
 * @note This also does sharedmem init/handling if needed, on [7.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btdrvRegisterHidReportEvent(Event* out_event);

/**
 * @brief GetHidReportEventInfo
 * @note \ref btdrvRegisterHidReportEvent must be used before this, on [7.0.0+].
 * @note This is used by hid-sysmodule. When used by other processes, hid/user-process will conflict. No events will be received by that user-process, or it will be corrupted, etc.
 * @note [7.0.0+] When data isn't available, the type is set to ::BtdrvHidEventType_Unknown4, with the buffer cleared to all-zero.
 * @param[out] buffer Output buffer, see \ref BtdrvHidReportEventInfo.
 * @param[in] size Output buffer size.
 * @oaram[out] type \ref BtdrvHidEventType
 */
Result btdrvGetHidReportEventInfo(void* buffer, size_t size, BtdrvHidEventType *type);

/// Gets the SharedMemory addr for HidReportEventInfo (\ref BtdrvCircularBuffer), only valid when \ref btdrvRegisterHidReportEvent was previously used, on [7.0.0+].
void* btdrvGetHidReportEventInfoSharedmemAddr(void);

/**
 * @brief GetLatestPlr
 * @param[out] out Output \ref BtdrvPlrList, on pre-9.0.0 this is \ref BtdrvPlrStatistics.
 */
Result btdrvGetLatestPlr(BtdrvPlrList *out);

/**
 * @brief EnableTxPowerBoostSetting
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result btdrvEnableTxPowerBoostSetting(bool flag);

/**
 * @brief IsTxPowerBoostSettingEnabled
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result btdrvIsTxPowerBoostSettingEnabled(bool *out);

/**
 * @brief EnableAfhSetting
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result btdrvEnableAfhSetting(bool flag);

/**
 * @brief IsAfhSettingEnabled
 * @param[out] out Output flag.
 */
Result btdrvIsAfhSettingEnabled(bool *out);

/**
 * @brief GetChannelMap
 * @note Only available on [3.0.0+].
 * @param[out] out \ref BtdrvChannelMapList
 */
Result btdrvGetChannelMap(BtdrvChannelMapList *out);

/**
 * @brief SetBleVisibility
 * @note Only available on [5.0.0+].
 * @param[in] flag0 Unknown flag.
 * @param[in] flag1 Unknown flag.
 */
Result btdrvSetBleVisibility(bool flag0, bool flag1);

/**
 * @brief SetBleAdvertiseData
 * @note Only available on [5.0.0+].
 * @param[in] data \ref BtdrvBleAdvertisePacketData
 */
Result btdrvSetBleAdvertiseData(const BtdrvBleAdvertisePacketData *data);

/**
 * @brief SetBleAdvertiseParameter
 * @note Only available on [5.0.0+].
 * @param[in] addr \ref BtdrvAddress
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 */
Result btdrvSetBleAdvertiseParameter(BtdrvAddress addr, u16 unk0, u16 unk1);

/**
 * @brief RegisterGattClient
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvRegisterGattClient(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief UnregisterGattClient
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 */
Result btdrvUnregisterGattClient(u8 unk);

/**
 * @brief UnregisterAllGattClients
 * @note Only available on [5.0.0+].
 */
Result btdrvUnregisterAllGattClients(void);

/**
 * @brief GetGattAttribute
 * @note Only available on [5.0.0+].
 * @param[in] addr \ref BtdrvAddress, only used on pre-9.0.0.
 * @param[in] unk Unknown
 */
Result btdrvGetGattAttribute(BtdrvAddress addr, u32 unk);

/**
 * @brief GetGattService
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvGetGattService(u32 unk, const BtdrvGattAttributeUuid *uuid);

/**
 * @brief RegisterGattServer
 * @note Only available on [5.0.0+].
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvRegisterGattServer(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief UnregisterGattServer
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 */
Result btdrvUnregisterGattServer(u8 unk);

/**
 * @brief ConnectGattClient
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] addr \ref BtdrvAddress
 * @param[in] flag Flag
 */
Result btdrvConnectGattClient(u8 unk, BtdrvAddress addr, bool flag);

/**
 * @brief DisconnectGattClient
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] addr \ref BtdrvAddress, only used on pre-9.0.0.
 */
Result btdrvDisconnectGattClient(u8 unk, BtdrvAddress addr);

/**
 * @brief AddGattService
 * @note Only available on [5.0.0+].
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[in] flag Flag
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvAddGattService(u8 unk0, u8 unk1, bool flag, const BtdrvGattAttributeUuid *uuid);

/**
 * @brief EnableGattService
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvEnableGattService(u8 unk, const BtdrvGattAttributeUuid *uuid);

/**
 * @brief AddGattCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[in] unk2 Unknown
 * @param[in] uuid0 \ref BtdrvGattAttributeUuid
 * @param[in] uuid1 \ref BtdrvGattAttributeUuid
 */
Result btdrvAddGattCharacteristic(u8 unk0, u8 unk1, u16 unk2, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1);

/**
 * @brief AddGattDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[in] uuid0 \ref BtdrvGattAttributeUuid
 * @param[in] uuid1 \ref BtdrvGattAttributeUuid
 */
Result btdrvAddGattDescriptor(u8 unk0, u16 unk1, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1);

/**
 * @brief GetBleManagedEventInfo
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[out] buffer Output buffer. 0x400-bytes from state is written here.
 * @param[in] size Output buffer size.
 * @oaram[out] type Output BleEventType.
 */
Result btdrvGetBleManagedEventInfo(void* buffer, size_t size, u32 *type);

/**
 * @brief GetGattFirstCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] id \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] unk_out Unknown
 * @param[out] id_out \ref BtdrvGattId
 */
Result btdrvGetGattFirstCharacteristic(bool flag, u32 unk, const BtdrvGattId *id, const BtdrvGattAttributeUuid *uuid, u8 *unk_out, BtdrvGattId *id_out);

/**
 * @brief GetGattNextCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] unk_out Unknown
 * @param[out] id_out \ref BtdrvGattId
 */
Result btdrvGetGattNextCharacteristic(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattAttributeUuid *uuid, u8 *unk_out, BtdrvGattId *id_out);

/**
 * @brief GetGattFirstDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] id_out \ref BtdrvGattId
 */
Result btdrvGetGattFirstDescriptor(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattAttributeUuid *uuid, BtdrvGattId *id_out);

/**
 * @brief GetGattNextDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] flag Flag
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] id_out \ref BtdrvGattId
 */
Result btdrvGetGattNextDescriptor(bool flag, u32 unk, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, const BtdrvGattAttributeUuid *uuid, BtdrvGattId *id_out);

/**
 * @brief RegisterGattManagedDataPath
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvRegisterGattManagedDataPath(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief UnregisterGattManagedDataPath
 * @note Only available on [5.0.0+].
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvUnregisterGattManagedDataPath(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief RegisterGattHidDataPath
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvRegisterGattHidDataPath(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief UnregisterGattHidDataPath
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvUnregisterGattHidDataPath(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief RegisterGattDataPath
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvRegisterGattDataPath(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief UnregisterGattDataPath
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdrvUnregisterGattDataPath(const BtdrvGattAttributeUuid *uuid);

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
 * @brief GetLeHidEventInfo
 * @note Only available on [5.0.0+].
 * @note The state used by this is reset after writing the data to output.
 * @param[out] buffer Output buffer. 0x400-bytes from state is written here.
 * @param[in] size Output buffer size.
 * @oaram[out] type Output BleEventType.
 */
Result btdrvGetLeHidEventInfo(void* buffer, size_t size, u32 *type);

/**
 * @brief RegisterBleHidEvent
 * @note Only available on [5.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btdrvRegisterBleHidEvent(Event* out_event);

/**
 * @brief MoveToSecondaryPiconet
 * @note Only available on [10.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvMoveToSecondaryPiconet(BtdrvAddress addr);

/**
 * @brief IsManufacturingMode
 * @note Only available on [5.0.0+].
 * @param[out] out Output flag.
 */
Result btdrvIsManufacturingMode(bool *out);

/**
 * @brief EmulateBluetoothCrash
 * @note Only available on [7.0.0+].
 * @param[in] reason \ref BtdrvFatalReason
 */
Result btdrvEmulateBluetoothCrash(BtdrvFatalReason reason);

/**
 * @brief GetBleChannelMap
 * @note Only available on [9.0.0+].
 * @param[out] out \ref BtdrvChannelMapList
 */
Result btdrvGetBleChannelMap(BtdrvChannelMapList *out);

///@name CircularBuffer
///@{

/**
 * @brief Read
 * @note Used by \ref btdrvGetHidReportEventInfo on [7.0.0+].
 * @param c \ref BtdrvCircularBuffer
 */
void* btdrvCircularBufferRead(BtdrvCircularBuffer *c);

/**
 * @brief Free
 * @note Used by \ref btdrvGetHidReportEventInfo on [7.0.0+].
 * @param c \ref BtdrvCircularBuffer
 */
bool btdrvCircularBufferFree(BtdrvCircularBuffer *c);

///@}

