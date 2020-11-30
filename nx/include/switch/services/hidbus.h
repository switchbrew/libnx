/**
 * @file hidbus.h
 * @brief hidbus service IPC wrapper, for using external devices attached to HID controllers. See also: https://switchbrew.org/wiki/HID_services#hidbus
 * @note Only available on [5.0.0+].
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/hid.h"
#include "../sf/service.h"

/// BusType
typedef enum {
    HidbusBusType_LeftJoyRail   = 0,          ///< LeftJoyRail
    HidbusBusType_RightJoyRail  = 1,          ///< RightJoyRail
    HidbusBusType_RightLarkRail = 2,          ///< [6.0.0+] RightLarkRail (for microphone).
} HidbusBusType;

/// JoyPollingMode
typedef enum {
    HidbusJoyPollingMode_SixAxisSensorDisable = 0,    ///< SixAxisSensorDisable
    HidbusJoyPollingMode_SixAxisSensorEnable  = 1,    ///< JoyEnableSixAxisPollingData
    HidbusJoyPollingMode_ButtonOnly           = 2,    ///< [6.0.0+] ButtonOnly
} HidbusJoyPollingMode;

/// BusHandle
typedef struct {
    u32 abstracted_pad_id;         ///< AbstractedPadId
    u8 internal_index;             ///< InternalIndex
    u8 player_number;              ///< PlayerNumber
    u8 bus_type_id;                ///< BusTypeId
    u8 is_valid;                   ///< IsValid
} HidbusBusHandle;

/// JoyPollingReceivedData
typedef struct {
    u8 data[0x30];                 ///< Data.
    u64 out_size;                  ///< Size of data.
    u64 sampling_number;           ///< SamplingNumber
} HidbusJoyPollingReceivedData;

/// HidbusDataAccessorHeader
typedef struct {
    Result res;                                ///< Result.
    u32 pad;                                   ///< Padding.
    u8 unused[0x18];                           ///< Initialized sysmodule-side, not used by sdknso.
    u64 latest_entry;                          ///< Latest entry.
    u64 total_entries;                         ///< Total entries.
} HidbusDataAccessorHeader;

/// HidbusJoyDisableSixAxisPollingDataAccessorEntryData
typedef struct {
    u8 data[0x26];                             ///< Data.
    u8 out_size;                               ///< Size of data.
    u8 pad;                                    ///< Padding.
    u64 sampling_number;                       ///< SamplingNumber
} HidbusJoyDisableSixAxisPollingDataAccessorEntryData;

/// HidbusJoyDisableSixAxisPollingDataAccessorEntry
typedef struct {
    u64 sampling_number;                                         ///< SamplingNumber
    HidbusJoyDisableSixAxisPollingDataAccessorEntryData data;    ///< \ref HidbusJoyDisableSixAxisPollingDataAccessorEntryData
} HidbusJoyDisableSixAxisPollingDataAccessorEntry;

/// HidbusJoyEnableSixAxisPollingDataAccessorEntryData
typedef struct {
    u8 data[0x8];                              ///< Data.
    u8 out_size;                               ///< Size of data.
    u8 pad[7];                                 ///< Padding.
    u64 sampling_number;                       ///< SamplingNumber
} HidbusJoyEnableSixAxisPollingDataAccessorEntryData;

/// HidbusJoyEnableSixAxisPollingDataAccessorEntry
typedef struct {
    u64 sampling_number;                                         ///< SamplingNumber
    HidbusJoyEnableSixAxisPollingDataAccessorEntryData data;     ///< \ref HidbusJoyEnableSixAxisPollingDataAccessorEntryData
} HidbusJoyEnableSixAxisPollingDataAccessorEntry;

/// HidbusJoyButtonOnlyPollingDataAccessorEntryData
typedef struct {
    u8 data[0x2c];                             ///< Data.
    u8 out_size;                               ///< Size of data.
    u8 pad[3];                                 ///< Padding.
    u64 sampling_number;                       ///< SamplingNumber
} HidbusJoyButtonOnlyPollingDataAccessorEntryData;

/// HidbusJoyButtonOnlyPollingDataAccessorEntry
typedef struct {
    u64 sampling_number;                                         ///< SamplingNumber
    HidbusJoyButtonOnlyPollingDataAccessorEntryData data;        ///< \ref HidbusJoyEnableSixAxisPollingDataAccessorEntryData
} HidbusJoyButtonOnlyPollingDataAccessorEntry;

/// HidbusJoyDisableSixAxisPollingDataAccessor
typedef struct {
    HidbusDataAccessorHeader hdr;                                    ///< \ref HidbusDataAccessorHeader
    HidbusJoyDisableSixAxisPollingDataAccessorEntry entries[0xb];    ///< \ref HidbusJoyDisableSixAxisPollingDataAccessorEntry
} HidbusJoyDisableSixAxisPollingDataAccessor;

/// HidbusJoyEnableSixAxisPollingDataAccessor
typedef struct {
    HidbusDataAccessorHeader hdr;                                    ///< \ref HidbusDataAccessorHeader
    HidbusJoyEnableSixAxisPollingDataAccessorEntry entries[0xb];     ///< \ref HidbusJoyEnableSixAxisPollingDataAccessorEntry
} HidbusJoyEnableSixAxisPollingDataAccessor;

/// HidbusJoyButtonOnlyPollingDataAccessor
typedef struct {
    HidbusDataAccessorHeader hdr;                                    ///< \ref HidbusDataAccessorHeader
    HidbusJoyButtonOnlyPollingDataAccessorEntry entries[0xb];        ///< \ref HidbusJoyButtonOnlyPollingDataAccessorEntry
} HidbusJoyButtonOnlyPollingDataAccessor;

/// Common data for HidbusStatusManagerEntry*.
typedef struct {
    u8 is_connected;               ///< IsConnected
    u8 pad[3];                     ///< Padding.
    Result is_connected_result;    ///< IsConnectedResult
    u8 is_enabled;                 ///< Flag indicating whether a device is enabled (\ref hidbusEnableExternalDevice).
    u8 is_in_focus;                ///< Flag indicating whether this entry is valid.
    u8 is_polling_mode;            ///< Flag indicating whether polling is enabled (\ref hidbusEnableJoyPollingReceiveMode).
    u8 reserved;                   ///< Reserved
    u32 polling_mode;              ///< \ref HidbusJoyPollingMode
} HidbusStatusManagerEntryCommon;

/// HidbusStatusManagerEntry on 5.x.
typedef struct {
    HidbusStatusManagerEntryCommon common;       ///< \ref HidbusStatusManagerEntryCommon
    u8 unk_x10[0xf0];                            ///< Ignored by official sw.
} HidbusStatusManagerEntryV5;

/// HidbusStatusManagerEntry
typedef struct {
    HidbusStatusManagerEntryCommon common;       ///< \ref HidbusStatusManagerEntryCommon
    u8 unk_x10[0x70];                            ///< Ignored by official sw.
} HidbusStatusManagerEntry;

/// StatusManager on 5.x.
typedef struct {
    HidbusStatusManagerEntryV5 entries[0x10];    ///< \ref HidbusStatusManagerEntryV5
} HidbusStatusManagerV5;

/// StatusManager
typedef struct {
    HidbusStatusManagerEntry entries[0x13];      ///< \ref HidbusStatusManagerEntry
    u8 unused[0x680];                            ///< Unused.
} HidbusStatusManager;

/// Gets the Service object for the actual hidbus service session. This object must be closed by the user once finished using cmds with this.
Result hidbusGetServiceSession(Service* srv_out);

/// Gets the SharedMemory addr (\ref HidbusStatusManagerV5 on 5.x, otherwise \ref HidbusStatusManager). Only valid when at least one BusHandle is currently initialized (\ref hidbusInitialize).
void* hidbusGetSharedmemAddr(void);

/**
 * @brief GetBusHandle
 * @param[out] handle \ref HidbusBusHandle
 * @param[out] flag Output flag indicating whether the handle is valid.
 * @param[in] id \ref HidNpadIdType
 * @param[in] bus_type \ref HidbusBusType
 */
Result hidbusGetBusHandle(HidbusBusHandle *handle, bool *flag, HidNpadIdType id, HidbusBusType bus_type);

/**
 * @brief Initialize
 * @param[in] handle \ref HidbusBusHandle
 */
Result hidbusInitialize(HidbusBusHandle handle);

/**
 * @brief Finalize
 * @param[in] handle \ref HidbusBusHandle
 */
Result hidbusFinalize(HidbusBusHandle handle);

/**
 * @brief EnableExternalDevice
 * @note This uses \ref hidLaShowControllerFirmwareUpdate if needed.
 * @param[in] handle \ref HidbusBusHandle
 * @param[in] flag Whether to enable the device (true = enable, false = disable). When false, this will internally use \ref hidbusDisableJoyPollingReceiveMode if needed.
 * @param[in] device_id ExternalDeviceId which must match the connected device. Only used when flag is set.
 */
Result hidbusEnableExternalDevice(HidbusBusHandle handle, bool flag, u32 device_id);

/**
 * @brief SendAndReceive
 * @param[in] handle \ref HidbusBusHandle
 * @param[in] inbuf Input buffer, containing the command data.
 * @param[in] inbuf_size Input buffer size, must be <0x26.
 * @param[out] outbuf Output buffer, containing the command reply data.
 * @param[in] outbuf_size Output buffer max size.
 * @param[out] out_size Actual output size.
 */
Result hidbusSendAndReceive(HidbusBusHandle handle, const void* inbuf, size_t inbuf_size, void* outbuf, size_t outbuf_size, u64 *out_size);

/**
 * @brief EnableJoyPollingReceiveMode
 * @param[in] handle \ref HidbusBusHandle
 * @param[in] inbuf Input buffer, containing the command data.
 * @param[in] inbuf_size Input buffer size, must be <0x26.
 * @param[out] workbuf TransferMemory buffer, must be 0x1000-byte aligned. This buffer must not be written to until after \ref hidbusDisableJoyPollingReceiveMode is used.
 * @param[in] workbuf_size TransferMemory buffer size, must be 0x1000-byte aligned.
 * @param[in] polling_mode \ref HidbusJoyPollingMode
 */
Result hidbusEnableJoyPollingReceiveMode(HidbusBusHandle handle, const void* inbuf, size_t inbuf_size, void* workbuf, size_t workbuf_size, HidbusJoyPollingMode polling_mode);

/**
 * @brief DisableJoyPollingReceiveMode
 * @note This can also be used via \ref hidbusEnableExternalDevice with flag=false.
 * @param[in] handle \ref HidbusBusHandle
 */
Result hidbusDisableJoyPollingReceiveMode(HidbusBusHandle handle);

/**
 * @brief GetJoyPollingReceivedData
 * @param[in] handle \ref HidbusBusHandle
 * @param[out] recv_data Output array of \ref HidbusJoyPollingReceivedData.
 * @param[in] count Total entries for the recv_data array. The maximum is 0xa. Official apps use range 0x1-0x9.
 */
Result hidbusGetJoyPollingReceivedData(HidbusBusHandle handle, HidbusJoyPollingReceivedData *recv_data, s32 count);

