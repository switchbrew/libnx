/**
 * @file btdrv.h
 * @brief Bluetooth driver (btdrv) service IPC wrapper.
 * @author yellows8, ndeadly
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/btdrv_types.h"
#include "../services/set.h"
#include "../sf/service.h"

/// Data for \ref btdrvGetEventInfo. The data stored here depends on the \ref BtdrvEventType.
typedef struct {
    union {
        u8 data[0x400];                                     ///< Raw data.

        struct {
            u32 val;                                        ///< Value
        } type0;                                            ///< ::BtdrvEventType_Unknown0

        struct {
            union {
                struct {
                    char name[0xF9];                        ///< Device name, NUL-terminated string.
                    BtdrvAddress addr;                      ///< Device address.
                    u8 reserved_xFF[0x10];                  ///< Reserved
                    BtdrvClassOfDevice class_of_device;     ///< Class of Device.
                    u8 unk_x112[0x4];                       ///< Set to fixed value u32 0x1.
                    u8 reserved_x116[0xFA];                 ///< Reserved
                    u8 reserved_x210[0x5C];                 ///< Reserved
                    char name2[0xF9];                       ///< Device name, NUL-terminated string. Same as name above, except starting at index 1.
                    u8 rssi[0x4];                           ///< s32 RSSI
                    u8 name3[0x4];                          ///< Two bytes which are the same as name[11-12].
                    u8 reserved_x36D[0x10];                 ///< Reserved
                } v1;                                       ///< [1.0.0-11.0.1]

                struct {
                    BtdrvAddress addr;                      ///< Device address.
                    char name[0xF9];                        ///< Device name, NUL-terminated string.
                    BtdrvClassOfDevice class_of_device;     ///< Class of Device.
                    u8 reserved[0x6];                       ///< Reserved
                } v12;                                      ///< [12.0.0+]
            };
        } inquiry_device;                                   ///< ::BtdrvEventType_InquiryDevice

        struct {
            union {
                struct {
                    BtdrvInquiryStatus status;              ///< \ref BtdrvInquiryStatus
                } v1;                                       ///< [1.0.0-11.0.1]

                struct {
                    u8 status;                              ///< \ref BtdrvInquiryStatus
                    u8 pad[3];                              ///< Padding
                    u32 service_mask;                       ///< Services value from \ref btdrvStartInquiry when starting, otherwise this is value 0
                } v12;                                      ///< [12.0.0+]
            };
        } inquiry_status;                                   ///< ::BtdrvEventType_InquiryStatus

        struct {
            BtdrvAddress addr;                              ///< Device address.
            char name[0xF9];                                ///< Device name, NUL-terminated string.
            BtdrvClassOfDevice class_of_device;             ///< Class of Device.
        } pairing_pin_code_request;                         ///< ::BtdrvEventType_PairingPinCodeRequest

        struct {
            union {
                struct {
                    BtdrvAddress addr;                      ///< Device address.
                    char name[0xF9];                        ///< Device name, NUL-terminated string.
                    BtdrvClassOfDevice class_of_device;     ///< Class of Device.
                    u8 pad[2];                              ///< Padding
                    u32 type;                               ///< 0 = SSP confirm request, 3 = SSP passkey notification.
                    s32 passkey;                            ///< Passkey, only set when the above field is value 3.
                } v1;                                       ///< [1.0.0-11.0.1]

                struct {
                    BtdrvAddress addr;                      ///< Device address.
                    char name[0xF9];                        ///< Device name, NUL-terminated string.
                    BtdrvClassOfDevice class_of_device;     ///< Class of Device.
                    u8 flag;                                ///< bool flag for Just Works. With SSP passkey notification this is always 0
                    u8 pad;                                 ///< Padding
                    s32 passkey;                            ///< Passkey
                } v12;                                      ///< [12.0.0+]
            };  
        } ssp_request;                                      ///< ::BtdrvEventType_SspRequest

        struct {
            union {
                struct {
                    u32 status;                             ///< Status, always 0 except with ::BtdrvConnectionEventType_Status: 2 = ACL Link is now Resumed, 9 = connection failed (pairing/authentication failed, or opening the hid connection failed).
                    BtdrvAddress addr;                      ///< Device address.
                    u8 pad[2];                              ///< Padding
                    u32 type;                               ///< \ref BtdrvConnectionEventType
                } v1;                                       ///< [1.0.0-11.0.1]

                struct {
                    u32 type;                               ///< \ref BtdrvConnectionEventType
                    BtdrvAddress addr;                      ///< Device address.
                    u8 reserved[0xfe];                      ///< Reserved
                } v12;                                      ///< [12.0.0+]
            };
        } connection;                                       ///< ::BtdrvEventType_Connection

        struct {
            BtdrvAddress addr;                              ///< Device address.
            u8 status;                                      ///< Status flag: 1 = success, 0 = failure.
            u8 value;                                       ///< Tsi value, when the above indicates success.
        } tsi;                                              ///< ::BtdrvEventType_Tsi

        struct {
            BtdrvAddress addr;                              ///< Device address.
            u8 status;                                      ///< Status flag: 1 = success, 0 = failure.
            u8 value;                                       ///< Input bool value from \ref btdrvEnableBurstMode, when the above indicates success.
        } burst_mode;                                       ///< ::BtdrvEventType_BurstMode

        struct {
            BtdrvAddress addr;                              ///< Device address.
            u8 status;                                      ///< Status flag: 1 = success, 0 = failure.
            u8 flag;                                        ///< Bool flag, when the above indicates success.
        } set_zero_retransmission;                          ///< ::BtdrvEventType_SetZeroRetransmission

        struct {
            u8 status;                                      ///< Status flag: 1 = success, 0 = failure.
            u8 pad[0x3];                                    ///< Padding
            u32 count;                                      ///< Count value.
        } pending_connections;                              ///< ::BtdrvEventType_PendingConnections

        struct {
            BtdrvAddress addr;                              ///< Device address.
            u8 status;                                      ///< Status flag: 1 = success, 0 = failure.
        } move_to_secondary_piconet;                        ///< ::BtdrvEventType_MoveToSecondaryPiconet

        struct {
            u16 reason;                                     ///< \ref BtdrvFatalReason
        } bluetooth_crash;                                  ///< ::BtdrvEventType_BluetoothCrash
    };
} BtdrvEventInfo;

/// Data for \ref btdrvGetHidEventInfo. The data stored here depends on the \ref BtdrvHidEventType.
typedef struct {
    union {
        u8 data[0x480];                                 ///< Raw data.

        struct {
            union {
                struct {
                    BtdrvAddress addr;                  ///< Device address.
                    u8 pad[2];                          ///< Padding
                    BtdrvHidConnectionStatus status;    ///< \ref BtdrvHidConnectionStatus
                } v1;                                   ///< [1.0.0-11.0.1]

                struct {
                    BtdrvHidConnectionStatus status;    ///< \ref BtdrvHidConnectionStatus
                    BtdrvAddress addr;                  ///< Device address.
                } v12;                                  ///< [12.0.0+]
            };
        } connection;                                   ///< ::BtdrvHidEventType_Connection

        struct {
            u32 type;                                   ///< \ref BtdrvExtEventType, controls which data is stored below.

            union {
                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Device address.
                } set_tsi;                              ///< ::BtdrvExtEventType_SetTsi

                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Device address.
                } exit_tsi;                             ///< ::BtdrvExtEventType_ExitTsi

                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Device address.
                } set_burst_mode;                       ///< ::BtdrvExtEventType_SetBurstMode

                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Device address.
                } exit_burst_mode;                      ///< ::BtdrvExtEventType_ExitBurstMode

                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Device address.
                    u8 pad[2];                          ///< Padding
                    u8 flag;                            ///< Flag
                } set_zero_retransmission;              ///< ::BtdrvExtEventType_SetZeroRetransmission

                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Unused
                    u8 pad[2];                          ///< Padding
                    u32 count;                          ///< Count value.
                } pending_connections;                  ///< ::BtdrvExtEventType_PendingConnections

                struct {
                    u32 status;                         ///< 0 for success, non-zero for error.
                    BtdrvAddress addr;                  ///< Device address.
                } move_to_secondary_piconet;            ///< ::BtdrvExtEventType_MoveToSecondaryPiconet
            };
        } ext;                                          ///< ::BtdrvHidEventType_Ext [1.0.0-11.0.1]
    };
} BtdrvHidEventInfo;

/// Data for \ref btdrvGetHidReportEventInfo. The data stored here depends on the \ref BtdrvHidEventType.
typedef struct {
    union {
        u8 data[0x480];                         ///< Raw data.

        struct {
            union {
                struct {
                    struct {
                        BtdrvAddress addr;
                        u8 pad[2];
                        u32 res;
                        u32 size;
                    } hdr;
                    u8 unused[0x3];             ///< Unused
                    BtdrvAddress addr;          ///< \ref BtdrvAddress
                    u8 unused2[0x3];            ///< Unused
                    BtdrvHidData report;
                } v1;                           ///< [1.0.0-6.2.0]

                struct {
                    u8 unused[0x3];             ///< Unused
                    BtdrvAddress addr;          ///< \ref BtdrvAddress
                    u8 unused2[0x3];            ///< Unused
                    BtdrvHidData report;
                } v7;                           ///< [7.0.0-8.1.1]

                struct {
                    u32 res;                    ///< Always 0.
                    u8 unk_x4;                  ///< Always 0.
                    BtdrvAddress addr;          ///< \ref BtdrvAddress
                    u8 pad;                     ///< Padding
                    BtdrvHidReport report;
                } v9;                           ///< [9.0.0+]
            };
        } data_report;                          ///< ::BtdrvHidEventType_DataReport

        struct {
            union {
                u8 rawdata[0xC];                ///< Raw data.

                struct {
                    u32 res;                    ///< 0 = success, non-zero = error.
                    BtdrvAddress addr;          ///< \ref BtdrvAddress
                    u8 pad[2];                  ///< Padding
                };
            };
        } set_report;                           ///< ::BtdrvHidEventType_SetReport

        struct {
            union {
                union {
                    u8 rawdata[0x290];          ///< Raw data.

                    struct {
                        BtdrvAddress addr;      ///< \ref BtdrvAddress
                        u8 pad[2];              ///< Padding
                        u32 res;                ///< Unknown. hid-sysmodule only uses the below data when this field is 0.
                        BtdrvHidData report;    ///< \ref BtdrvHidData
                        u8 pad2[2];             ///< Padding
                    };
                } v1;                           ///< [1.0.0-8.1.1]

                union {
                    u8 rawdata[0x2C8];          ///< Raw data.

                    struct {
                        u32 res;                ///< Unknown. hid-sysmodule only uses the below report when this field is 0.
                        BtdrvAddress addr;      ///< \ref BtdrvAddress
                        BtdrvHidReport report;  ///< \ref BtdrvHidReport
                    };
                } v9;                           ///< [9.0.0+]
            };
        } get_report;                           ///< ::BtdrvHidEventType_GetReport
    };
} BtdrvHidReportEventInfo;

/// The raw sharedmem data for HidReportEventInfo.
typedef struct {
    struct {
        u8 type;        ///< \ref BtdrvHidEventType
        u8 pad[7];
        u64 tick;
        u64 size;
    } hdr;

    BtdrvHidReportEventInfo data;
} BtdrvHidReportEventInfoBufferData;

/// Data for \ref btdrvGetAudioEventInfo. The data stored here depends on the \ref BtdrvAudioEventType.
typedef union {
    struct {
        u32 status;                  ///< Status: 0 = AV connection closed, 1 = AV connection opened, 2 = failed to open AV connection.
        BtdrvAddress addr;           ///< Device address.
        u8 pad[2];                   ///< Padding
    } connection;                    ///< ::BtdrvAudioEventType_Connection
} BtdrvAudioEventInfo;

/// CircularBuffer
typedef struct {
    Mutex mutex;
    void* event_type;   ///< Not set with sharedmem.
    u8 data[0x2710];
    s32 write_offset;
    s32 read_offset;
    u64 utilization;
    char name[0x11];
    u8 initialized;
} BtdrvCircularBuffer;

/// Data for \ref btdrvGetBleManagedEventInfo. The data stored here depends on the \ref BtdrvBleEventType.
typedef struct {
    union {
        u8 data[0x400];

        struct {
            u32 status;
            u8 handle;
            u8 registered;
        } type0;

        struct {
            u32 status;
            u32 conn_id;
            u32 unk_x8;
            u32 unk_xC;
        } type2;

        struct {
            u32 conn_id;
            u16 min_interval;
            u16 max_interval;
            u16 slave_latency;
            u16 timeout_multiplier;
        } type3;                                ///< Connection params?

        struct {
            u32 status;
            u8 unk_x4;
            u8 unk_x5;
            u8 unk_x6;
            u8 unk_x7;
            u32 conn_id;
            BtdrvAddress address;
            u16 unk_x12;
        } type4;                                ///< Connection status?

        struct {
            u32 status;
            u8 unk_x4;
            u8 unk_x5;
            u8 unk_x6;
            BtdrvAddress address;
            BtdrvBleAdvertisementData adv[10];
            u8 count;
            u32 unk_x144;
        } type6;                                ///< Scan result?

        struct {
            u32 status;
            u32 conn_id;
        } type7;

        struct {
            u32 status;
            u8 interface;
            u8 unk_x5;
            u16 unk_x6;
            u32 unk_x8;
            BtdrvGattAttributeUuid svc_uuid;
            BtdrvGattAttributeUuid char_uuid;
            BtdrvGattAttributeUuid descr_uuid;
            u16 size;
            u8 data[0x202];
        } type8;                                ///< Notification?

        struct {
            u32 status;
            u32 conn_id;
            u32 unk_x8;
            u8 unk_xC[0x140];
        } type9;

        struct {
            u32 status;
            u32 conn_id;
            u8 unk_x8[0x24];
            u32 unk_x2C;
            u8 unk_x30[0x11c];
        } type10;

        struct {
            u32 status;
            u32 conn_id;
            u16 unk_x8;
        } type11;

        struct {
            u8 unk_x0[0x218];
        } type13;
    };
} BtdrvBleEventInfo;

/// Initialize btdrv.
Result btdrvInitialize(void);

/// Exit btdrv.
void btdrvExit(void);

/// Gets the Service object for the actual btdrv service session.
Service* btdrvGetServiceSession(void);

/**
 * @brief InitializeBluetooth
 * @note This is used by btm-sysmodule, this should not be used by other processes.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btdrvInitializeBluetooth(Event* out_event);

/**
 * @brief EnableBluetooth
 * @note This is used by btm-sysmodule.
 */
Result btdrvEnableBluetooth(void);

/**
 * @brief DisableBluetooth
 * @note This is used by btm-sysmodule.
 */
Result btdrvDisableBluetooth(void);

/**
 * @brief FinalizeBluetooth
 * @note This is not used by btm-sysmodule, this should not be used by other processes.
 */
Result btdrvFinalizeBluetooth(void);

/**
 * @brief GetAdapterProperties [1.0.0-11.0.1]
 * @param[out] properties \ref BtdrvAdapterProperty
 */
Result btdrvLegacyGetAdapterProperties(BtdrvAdapterPropertyOld *properties);

/**
 * @brief GetAdapterProperties [12.0.0+]
 * @param[out] properties \ref BtdrvAdapterPropertySet
 */
Result btdrvGetAdapterProperties(BtdrvAdapterPropertySet *properties);

/**
 * @brief GetAdapterProperty [1.0.0-11.0.1]
 * @param[in] type \ref BtdrvBluetoothPropertyType
 * @param[out] buffer Output buffer, see \ref BtdrvBluetoothPropertyType for the contents.
 * @param[in] size Output buffer size.
 */
Result btdrvLegacyGetAdapterProperty(BtdrvBluetoothPropertyType type, void* buffer, size_t size);

/**
 * @brief GetAdapterProperty [12.0.0+]
 * @param[in] type \ref BtdrvAdapterPropertyType
 * @param[in] property \ref BtdrvAdapterProperty
 */
Result btdrvGetAdapterProperty(BtdrvAdapterPropertyType type, BtdrvAdapterProperty *property);

/**
 * @brief SetAdapterProperty [1.0.0-11.0.1]
 * @param[in] type \ref BtdrvBluetoothPropertyType
 * @param[in] buffer Input buffer, see \ref BtdrvBluetoothPropertyType for the contents.
 * @param[in] size Input buffer size.
 */
Result btdrvLegacySetAdapterProperty(BtdrvBluetoothPropertyType type, const void* buffer, size_t size);

/**
 * @brief SetAdapterProperty [12.0.0+]
 * @param[in] type \ref BtdrvAdapterPropertyType
 * @param[in] property \ref BtdrvAdapterProperty
 */
Result btdrvSetAdapterProperty(BtdrvAdapterPropertyType type, const BtdrvAdapterProperty *property);

/**
 * @brief StartInquiry [1.0.0-11.0.1] This starts Inquiry, the output data will be available via \ref btdrvGetEventInfo. Inquiry will automatically stop in 10.24 seconds.
 * @note This is used by btm-sysmodule.
 */
Result btdrvLegacyStartInquiry(void);

/**
 * @brief StartInquiry [12.0.0+] This starts Inquiry, the output data will be available via \ref btdrvGetEventInfo.
 * @param[in] services Bitmask of allowed services. When -1 the original defaults from pre-12.0.0 are used.
 * @param[in] duration Inquiry duration in nanoseconds.
 * @note This is used by btm-sysmodule.
 */
Result btdrvStartInquiry(u32 services, s64 duration);

/**
 * @brief This stops Inquiry which was started by \ref btdrvStartInquiry, if it's still active.
 * @note This is used by btm-sysmodule.
 */
Result btdrvStopInquiry(void);

/**
 * @brief CreateBond
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] type TransportType
 */
Result btdrvCreateBond(BtdrvAddress addr, u32 type);

/**
 * @brief RemoveBond
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvRemoveBond(BtdrvAddress addr);

/**
 * @brief CancelBond
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvCancelBond(BtdrvAddress addr);

/**
 * @brief RespondToPinRequest [1.0.0-11.0.1]
 * @note The official sysmodule only uses the input \ref BtdrvAddress.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] flag Flag
 * @param[in] pin_code \ref BtdrvBluetoothPinCode
 * @param[in] length Length of pin_code
 */
Result btdrvLegacyRespondToPinRequest(BtdrvAddress addr, bool flag, const BtdrvBluetoothPinCode *pin_code, u8 length);

/**
 * @brief RespondToPinRequest [12.0.0+]
 * @param[in] addr \ref BtdrvAddress
 * @param[in] pin_code \ref BtdrvPinCode
 */
Result btdrvRespondToPinRequest(BtdrvAddress addr, const BtdrvPinCode *pin_code);

/**
 * @brief RespondToSspRequest
 * @note The official sysmodule only uses the input \ref BtdrvAddress and the flag.
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] variant BluetoothSspVariant
 * @param[in] accept Whether the request is accepted.
 * @param[in] passkey Passkey.
 */
Result btdrvRespondToSspRequest(BtdrvAddress addr, u8 variant, bool accept, u32 passkey);

/**
 * @brief GetEventInfo
 * @note This is used by btm-sysmodule.
 * @param[out] buffer Output buffer, see \ref BtdrvEventInfo.
 * @param[in] size Output buffer size.
 * @param[out] type Output BtdrvEventType.
 */
Result btdrvGetEventInfo(void* buffer, size_t size, BtdrvEventType *type);

/**
 * @brief InitializeHid
 * @note This is used by btm-sysmodule, this should not be used by other processes.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btdrvInitializeHid(Event* out_event);

/**
 * @brief OpenHidConnection
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvOpenHidConnection(BtdrvAddress addr);

/**
 * @brief CloseHidConnection
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvCloseHidConnection(BtdrvAddress addr);

/**
 * @brief This sends a HID DATA transaction packet with report-type Output.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] buffer Input \ref BtdrvHidReport, on pre-9.0.0 this is \ref BtdrvHidData.
 */
Result btdrvWriteHidData(BtdrvAddress addr, const BtdrvHidReport *buffer);

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
Result btdrvSetHidReport(BtdrvAddress addr, BtdrvBluetoothHhReportType type, const BtdrvHidReport *buffer);

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
 * @brief AddPairedDeviceInfo
 * @note This is used by btm-sysmodule.
 * @param[in] settings \ref SetSysBluetoothDevicesSettings
 */
Result btdrvAddPairedDeviceInfo(const SetSysBluetoothDevicesSettings *settings);

/**
 * @brief GetPairedDeviceInfo
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[out] settings \ref SetSysBluetoothDevicesSettings
 */
Result btdrvGetPairedDeviceInfo(BtdrvAddress addr, SetSysBluetoothDevicesSettings *settings);

/**
 * @brief FinalizeHid
 * @note This is not used by btm-sysmodule, this should not be used by other processes.
 */
Result btdrvFinalizeHid(void);

/**
 * @brief GetHidEventInfo
 * @note This is used by btm-sysmodule.
 * @param[out] buffer Output buffer, see \ref BtdrvHidEventInfo.
 * @param[in] size Output buffer size.
 * @param[out] type \ref BtdrvHidEventType, always ::BtdrvHidEventType_Connection or ::BtdrvHidEventType_Ext.
 */
Result btdrvGetHidEventInfo(void* buffer, size_t size, BtdrvHidEventType *type);

/**
 * @brief SetTsi
 * @note The response will be available via \ref btdrvGetHidEventInfo.
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] tsi Tsi: non-value-0xFF to Set, value 0xFF to Exit. See also \ref BtmTsiMode.
 */
Result btdrvSetTsi(BtdrvAddress addr, u8 tsi);

/**
 * @brief EnableBurstMode
 * @note The response will be available via \ref btdrvGetHidEventInfo.
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] flag Flag: true = Set, false = Exit.
 */
Result btdrvEnableBurstMode(BtdrvAddress addr, bool flag);

/**
 * @brief SetZeroRetransmission
 * @note The response will be available via \ref btdrvGetHidEventInfo.
 * @note This is used by btm-sysmodule.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] report_ids Input buffer containing an array of u8s.
 * @param[in] count Total u8s in the input buffer. This can be 0, the max is 5.
 */
Result btdrvSetZeroRetransmission(BtdrvAddress addr, u8 *report_ids, u8 count);

/**
 * @brief EnableMcMode
 * @note This is used by btm-sysmodule.
 * @param[in] flag Flag
 */
Result btdrvEnableMcMode(bool flag);

/**
 * @brief EnableLlrScan
 * @note This is used by btm-sysmodule.
 */
Result btdrvEnableLlrScan(void);

/**
 * @brief DisableLlrScan
 * @note This is used by btm-sysmodule.
 */
Result btdrvDisableLlrScan(void);

/**
 * @brief EnableRadio
 * @note This is used by btm-sysmodule.
 * @param[in] flag Flag
 */
Result btdrvEnableRadio(bool flag);

/**
 * @brief SetVisibility
 * @note This is used by btm-sysmodule.
 * @param[in] inquiry_scan Controls Inquiry Scan, whether the device can be discovered during Inquiry.
 * @param[in] page_scan Controls Page Scan, whether the device accepts connections.
 */
Result btdrvSetVisibility(bool inquiry_scan, bool page_scan);

/**
 * @brief EnableTbfcScan
 * @note Only available on [4.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] flag Flag
 */
Result btdrvEnableTbfcScan(bool flag);

/**
 * @brief RegisterHidReportEvent
 * @note This also does sharedmem init/handling if needed, on [7.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true. This is signaled when data is available with \ref btdrvGetHidReportEventInfo.
 */
Result btdrvRegisterHidReportEvent(Event* out_event);

/**
 * @brief GetHidReportEventInfo
 * @note \ref btdrvRegisterHidReportEvent must be used before this, on [7.0.0+].
 * @note This is used by hid-sysmodule. When used by other processes, hid/user-process will conflict. No events will be received by that user-process, or it will be corrupted, etc.
 * @note [7.0.0+] When data isn't available, the type is set to ::BtdrvHidEventType_Data, with the buffer cleared to all-zero.
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
 * @brief GetPendingConnections
 * @note The output data will be available via \ref btdrvGetHidEventInfo.
 * @note This is used by btm-sysmodule.
 * @note Only available on [3.0.0+].
 */
Result btdrvGetPendingConnections(void);

/**
 * @brief GetChannelMap
 * @note Only available on [3.0.0+].
 * @param[out] out \ref BtdrvChannelMapList
 */
Result btdrvGetChannelMap(BtdrvChannelMapList *out);

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
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result btdrvIsAfhSettingEnabled(bool *out);

/**
 * @brief InitializeBle
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btdrvInitializeBle(Event* out_event);

/**
 * @brief EnableBle
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 */
Result btdrvEnableBle(void);

/**
 * @brief DisableBle
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 */
Result btdrvDisableBle(void);

/**
 * @brief FinalizeBle
 * @note Only available on [5.0.0+].
 */
Result btdrvFinalizeBle(void);

/**
 * @brief SetBleVisibility
 * @note Only available on [5.0.0+].
 * @param[in] discoverable Whether the BLE device is discoverable.
 * @param[in] connectable Whether the BLE device is connectable.
 */
Result btdrvSetBleVisibility(bool discoverable, bool connectable);

/**
 * @brief SetLeConnectionParameter
 * @note Only available on [5.0.0-8.1.1]. This is the older version of \ref btdrvSetBleConnectionParameter.
 * @param[in] param \ref BtdrvLeConnectionParams
 */
Result btdrvSetLeConnectionParameter(const BtdrvLeConnectionParams *param);

/**
 * @brief SetBleConnectionParameter
 * @note Only available on [9.0.0+]. This is the newer version of \ref btdrvSetLeConnectionParameter.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] param \ref BtdrvBleConnectionParameter
 * @param[in] flag Flag
 */
Result btdrvSetBleConnectionParameter(BtdrvAddress addr, const BtdrvBleConnectionParameter *param, bool flag);

/**
 * @brief SetLeDefaultConnectionParameter
 * @note Only available on [5.0.0-8.1.1]. This is the older version of \ref btdrvSetBleDefaultConnectionParameter.
 * @param[in] param \ref BtdrvLeConnectionParams
 */
Result btdrvSetLeDefaultConnectionParameter(const BtdrvLeConnectionParams *param);

/**
 * @brief SetBleDefaultConnectionParameter
 * @note Only available on [9.0.0+]. This is the newer version of \ref btdrvSetLeDefaultConnectionParameter.
 * @param[in] param \ref BtdrvBleConnectionParameter
 */
Result btdrvSetBleDefaultConnectionParameter(const BtdrvBleConnectionParameter *param);

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
 * @brief StartBleScan
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 */
Result btdrvStartBleScan(void);

/**
 * @brief StopBleScan
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 */
Result btdrvStopBleScan(void);

/**
 * @brief AddBleScanFilterCondition
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] filter \ref BtdrvBleAdvertiseFilter
 */
Result btdrvAddBleScanFilterCondition(const BtdrvBleAdvertiseFilter *filter);

/**
 * @brief DeleteBleScanFilterCondition
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] filter \ref BtdrvBleAdvertiseFilter
 */
Result btdrvDeleteBleScanFilterCondition(const BtdrvBleAdvertiseFilter *filter);

/**
 * @brief DeleteBleScanFilter
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 */
Result btdrvDeleteBleScanFilter(u8 unk);

/**
 * @brief ClearBleScanFilters
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 */
Result btdrvClearBleScanFilters(void);

/**
 * @brief EnableBleScanFilter
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] flag Flag
 */
Result btdrvEnableBleScanFilter(bool flag);

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
 * @brief ConnectGattServer
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] unk Unknown
 * @param[in] addr \ref BtdrvAddress
 * @param[in] flag Flag
 * @param[in] AppletResourceUserId AppletResourceUserId
 */
Result btdrvConnectGattServer(u8 unk, BtdrvAddress addr, bool flag, u64 AppletResourceUserId);

/**
 * @brief CancelConnectGattServer
 * @note Only available on [5.1.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] unk Unknown
 * @param[in] addr \ref BtdrvAddress
 * @param[in] flag Flag
 */
Result btdrvCancelConnectGattServer(u8 unk, BtdrvAddress addr, bool flag);

/**
 * @brief DisconnectGattServer
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] unk Unknown
 */
Result btdrvDisconnectGattServer(u32 unk);

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
 * @brief ConfigureAttMtu
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] unk Unknown
 * @param[in] mtu MTU
 */
Result btdrvConfigureAttMtu(u32 unk, u16 mtu);

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
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[in] unk1 Unknown
 * @param[in] flag Flag
 */
Result btdrvAddGattService(u8 unk0, const BtdrvGattAttributeUuid *uuid, u8 unk1, bool flag);

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
 * @param[in] uuid0 \ref BtdrvGattAttributeUuid
 * @param[in] uuid1 \ref BtdrvGattAttributeUuid
 * @param[in] unk1 Unknown
 * @param[in] unk2 Unknown
 */
Result btdrvAddGattCharacteristic(u8 unk0, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1, u8 unk1, u16 unk2);

/**
 * @brief AddGattDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] unk0 Unknown
 * @param[in] uuid0 \ref BtdrvGattAttributeUuid
 * @param[in] uuid1 \ref BtdrvGattAttributeUuid
 * @param[in] unk1 Unknown
 */
Result btdrvAddGattDescriptor(u8 unk0, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1, u16 unk1);

/**
 * @brief GetBleManagedEventInfo
 * @note Only available on [5.0.0+].
 * @note This is used by btm-sysmodule.
 * @param[out] buffer Output buffer. 0x400-bytes from state is written here.
 * @param[in] size Output buffer size.
 * @oaram[out] type Output BtdrvBleEventType.
 */
Result btdrvGetBleManagedEventInfo(void* buffer, size_t size, BtdrvBleEventType *type);

/**
 * @brief GetGattFirstCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] id \ref BtdrvGattId
 * @param[in] flag Flag
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] out_property Output Property.
 * @param[out] out_char_id Output CharacteristicId \ref BtdrvGattId
 */
Result btdrvGetGattFirstCharacteristic(u32 unk, const BtdrvGattId *id, bool flag, const BtdrvGattAttributeUuid *uuid, u8 *out_property, BtdrvGattId *out_char_id);

/**
 * @brief GetGattNextCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] flag Flag
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] out_property Output Property.
 * @param[out] out_char_id Output CharacteristicId \ref BtdrvGattId
 */
Result btdrvGetGattNextCharacteristic(u32 unk, const BtdrvGattId *id0, bool flag, const BtdrvGattId *id1, const BtdrvGattAttributeUuid *uuid, u8 *out_property, BtdrvGattId *out_char_id);

/**
 * @brief GetGattFirstDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] flag Flag
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] out_desc_id Output DescriptorId \ref BtdrvGattId
 */
Result btdrvGetGattFirstDescriptor(u32 unk, const BtdrvGattId *id0, bool flag, const BtdrvGattId *id1, const BtdrvGattAttributeUuid *uuid, BtdrvGattId *out_desc_id);

/**
 * @brief GetGattNextDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] unk Unknown
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] flag Flag
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] out_desc_id Output DescriptorId \ref BtdrvGattId
 */
Result btdrvGetGattNextDescriptor(u32 unk, const BtdrvGattId *id0, bool flag, const BtdrvGattId *id1, const BtdrvGattId *id2, const BtdrvGattAttributeUuid *uuid, BtdrvGattId *out_desc_id);

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
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] unk Unknown
 */
Result btdrvReadGattCharacteristic(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, u8 unk);

/**
 * @brief ReadGattDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] unk Unknown
 */
Result btdrvReadGattDescriptor(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, u8 unk);

/**
 * @brief WriteGattCharacteristic
 * @note Only available on [5.0.0+].
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 * @param[in] unk Unknown
 * @param[in] flag Flag
 */
Result btdrvWriteGattCharacteristic(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const void* buffer, size_t size, u8 unk, bool flag);

/**
 * @brief WriteGattDescriptor
 * @note Only available on [5.0.0+].
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 * @param[in] unk Unknown
 */
Result btdrvWriteGattDescriptor(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, const void* buffer, size_t size, u8 unk);

/**
 * @brief RegisterGattNotification
 * @note Only available on [5.0.0+].
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btdrvRegisterGattNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief UnregisterGattNotification
 * @note Only available on [5.0.0+].
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btdrvUnregisterGattNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief GetLeHidEventInfo
 * @note Only available on [5.0.0+].
 * @note The state used by this is reset after writing the data to output.
 * @param[out] buffer Output buffer. 0x400-bytes from state is written here. See \ref BtdrvLeEventInfo.
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
 * @brief SetBleScanParameter
 * @note Only available on [5.1.0+].
 * @note This is used by btm-sysmodule.
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 */
Result btdrvSetBleScanParameter(u16 unk0, u16 unk1);

/**
 * @brief MoveToSecondaryPiconet
 * @note The response will be available via \ref btdrvGetHidEventInfo.
 * @note Only available on [10.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvMoveToSecondaryPiconet(BtdrvAddress addr);

/**
 * @brief IsBluetoothEnabled
 * @note Only available on [12.0.0+].
 * @param[out] out Output flag.
 */
Result btdrvIsBluetoothEnabled(bool *out);

/**
 * @brief AcquireAudioEvent
 * @note Only available on [12.0.0+].
 * @param[out] out_event Output Event.
 * @param[in] autoclear Event autoclear.
 */
Result btdrvAcquireAudioEvent(Event* out_event, bool autoclear);

/**
 * @brief GetAudioEventInfo
 * @note Only available on [12.0.0+].
 * @param[out] buffer Output buffer, see \ref BtdrvAudioEventInfo.
 * @param[in] size Output buffer size.
 * @param[out] type \ref BtdrvAudioEventType.
 */
Result btdrvGetAudioEventInfo(void* buffer, size_t size, BtdrvAudioEventType *type);

/**
 * @brief OpenAudioConnection
 * @note Only available on [12.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvOpenAudioConnection(BtdrvAddress addr);

/**
 * @brief CloseAudioConnection
 * @note Only available on [12.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btdrvCloseAudioConnection(BtdrvAddress addr);

/**
 * @brief OpenAudioOut
 * @note Only available on [12.0.0+].
 * @param[in] addr \ref BtdrvAddress
 * @param[out] audio_handle Audio handle.
 */
Result btdrvOpenAudioOut(BtdrvAddress addr, u32 *audio_handle);

/**
 * @brief CloseAudioOut
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 */
Result btdrvCloseAudioOut(u32 audio_handle);

/**
 * @brief StartAudioOut
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[in] pcm_param \ref BtdrvPcmParameter
 * @param[in] in_latency Input latency in nanoseconds.
 * @param[out] out_latency Output latency in nanoseconds.
 * @param[out] out1 Unknown output.
 */
Result btdrvStartAudioOut(u32 audio_handle, const BtdrvPcmParameter *pcm_param, s64 in_latency, s64 *out_latency, u64 *out1);

/**
 * @brief StopAudioOut
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 */
Result btdrvStopAudioOut(u32 audio_handle);

/**
 * @brief GetAudioOutState
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[out] out \ref BtdrvAudioOutState
 */
Result btdrvGetAudioOutState(u32 audio_handle, BtdrvAudioOutState *out);

/**
 * @brief GetAudioOutFeedingCodec
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[out] out \ref BtdrvAudioCodec
 */
Result btdrvGetAudioOutFeedingCodec(u32 audio_handle, BtdrvAudioCodec *out);

/**
 * @brief GetAudioOutFeedingParameter
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[out] out \ref BtdrvPcmParameter
 */
Result btdrvGetAudioOutFeedingParameter(u32 audio_handle, BtdrvPcmParameter *out);

/**
 * @brief AcquireAudioOutStateChangedEvent
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[out] out_event Output Event.
 * @param[in] autoclear Event autoclear.
 */
Result btdrvAcquireAudioOutStateChangedEvent(u32 audio_handle, Event* out_event, bool autoclear);

/**
 * @brief AcquireAudioOutBufferAvailableEvent
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[out] out_event Output Event.
 * @param[in] autoclear Event autoclear.
 */
Result btdrvAcquireAudioOutBufferAvailableEvent(u32 audio_handle, Event* out_event, bool autoclear);

/**
 * @brief SendAudioData
 * @note Only available on [12.0.0+].
 * @param[in] audio_handle Audio handle from \ref btdrvOpenAudioOut.
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size.
 * @param[out] Output transferred size. This is always either 0 (error occured) or the buffer size.
 */
Result btdrvSendAudioData(u32 audio_handle, const void* buffer, size_t size, u64 *transferred_size);

/**
 * @brief AcquireAudioControlInputStateChangedEvent
 * @note Only available on [12.0.0+].
 * @param[out] out_event Output Event.
 * @param[in] autoclear Event autoclear.
 */
Result btdrvAcquireAudioControlInputStateChangedEvent(Event* out_event, bool autoclear);

/**
 * @brief GetAudioControlInputState
 * @note Only available on [12.0.0+].
 * @param[out] states Output array of \ref BtdrvAudioControlButtonState.
 * @param[in] count Size of the states array in entries, the maximum is 0xF.
 * @param[out] total_out Total output entries.
 */
Result btdrvGetAudioControlInputState(BtdrvAudioControlButtonState *states, s32 count, s32 *total_out);

/**
 * @brief AcquireAudioConnectionStateChangedEvent
 * @note Only available on [12.0.0+].
 * @param[out] out_event Output Event.
 * @param[in] autoclear Event autoclear.
 */
Result btdrvAcquireAudioConnectionStateChangedEvent(Event* out_event, bool autoclear);

/**
 * @brief GetConnectedAudioDevice
 * @note Only available on [12.0.0+].
 * @param[out] addrs Output array of \ref BtdrvAddress.
 * @param[in] count Size of the addrs array in entries, the maximum is 0x8.
 * @param[out] total_out Total output entries.
 */
Result btdrvGetConnectedAudioDevice(BtdrvAddress *addrs, s32 count, s32 *total_out);

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

