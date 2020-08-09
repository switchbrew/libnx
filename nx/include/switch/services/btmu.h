/**
 * @file btmu.h
 * @brief btm:u (btm user) service IPC wrapper.
 * @note Only available on [5.0.0+].
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/btdrv.h"
#include "../sf/service.h"

/// GattService
typedef struct {
    u8 unk_x0[0x4];               ///< Unknown
    BtdrvGattAttributeUuid uuid;  ///< \ref BtdrvGattAttributeUuid
    u16 unk_x18;                  ///< Unknown
    u8 unk_x1A[0x4];              ///< Unknown
    u16 unk_x1E;                  ///< Unknown
    u8 unk_x20;                   ///< Unknown
    u8 pad[3];                    ///< Padding
} BtmuGattService;

/// GattCharacteristic
typedef struct {
    u8 unk_x0[0x24];              ///< Unknown
} BtmuGattCharacteristic;

/// GattDescriptor
typedef struct {
    u8 unk_x0[0x20];              ///< Unknown
} BtmuGattDescriptor;

/// BleDataPath
typedef struct {
    u8 unk_x0;                    ///< Unknown
    u8 pad[3];                    ///< Padding
    BtdrvGattAttributeUuid uuid;  ///< \ref BtdrvGattAttributeUuid
} BtmuBleDataPath;

/// Initialize btm:u.
Result btmuInitialize(void);

/// Exit btm:u.
void btmuExit(void);

/// Gets the Service object for the actual btm:u service session. This object must be closed by the user once finished using cmds with this.
Result btmuGetServiceSession(Service* srv_out);

/// Gets the Service object for IBtmUserCore.
Service* btmuGetServiceSession_IBtmUserCore(void);

/**
 * @brief AcquireBleScanEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleScanEvent(Event* out_event);

/**
 * @brief GetBleScanFilterParameter
 * @param[in] unk Must be value 0x1 or 0xFFFF.
 * @param[out] out \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuGetBleScanFilterParameter(u16 unk, BtdrvBleAdvertisePacketParameter *out);

/**
 * @brief GetBleScanFilterParameter2
 * @param[in] unk Must be value 0x2.
 * @param[out] out \ref BtdrvGattAttributeUuid. The first 4-bytes is always 0.
 */
Result btmuGetBleScanFilterParameter2(u16 unk, BtdrvGattAttributeUuid *out);

/**
 * @brief StartBleScanForGeneral
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuStartBleScanForGeneral(BtdrvBleAdvertisePacketParameter param);

/**
 * @brief StopBleScanForGeneral
 */
Result btmuStopBleScanForGeneral(void);

/**
 * @brief GetBleScanResultsForGeneral
 * @param[out] results Output array of \ref BtdrvBleScanResult.
 * @param[in] count Size of the results array in entries. The max is 10.
 * @param[out] total_out Total output entries.
 */
Result btmuGetBleScanResultsForGeneral(BtdrvBleScanResult *results, u8 count, u8 *total_out);

/**
 * @brief StartBleScanForPaired
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuStartBleScanForPaired(BtdrvBleAdvertisePacketParameter param);

/**
 * @brief StopBleScanForPaired
 */
Result btmuStopBleScanForPaired(void);

/**
 * @brief StartBleScanForSmartDevice
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btmuStartBleScanForSmartDevice(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief StopBleScanForSmartDevice
 */
Result btmuStopBleScanForSmartDevice(void);

/**
 * @brief GetBleScanResultsForSmartDevice
 * @param[out] results Output array of \ref BtdrvBleScanResult.
 * @param[in] count Size of the results array in entries. The max is 10.
 * @param[out] total_out Total output entries.
 */
Result btmuGetBleScanResultsForSmartDevice(BtdrvBleScanResult *results, u8 count, u8 *total_out);

/**
 * @brief AcquireBleConnectionEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleConnectionEvent(Event* out_event);

/**
 * @brief BleConnect
 * @note The \ref BtdrvAddress must not be already connected. A maximum of 4 devices can be connected.
 * @param[in] addr \ref BtdrvAddress
 */
Result btmuBleConnect(BtdrvAddress addr);

/**
 * @brief BleDisconnect
 * @param[in] id This must match a BtdrvBleConnectionInfo::id from \ref btmuBleGetConnectionState. [5.1.0+] 0xFFFFFFFF is invalid.
 */
Result btmuBleDisconnect(u32 id);

/**
 * @brief BleGetConnectionState
 * @param[out] info Output array of \ref BtdrvBleConnectionInfo.
 * @param[in] count Size of the info array in entries. Other cmds which use this internally use count=4.
 * @param[out] total_out Total output entries.
 */
Result btmuBleGetConnectionState(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out);

/**
 * @brief AcquireBlePairingEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBlePairingEvent(Event* out_event);

/**
 * @brief BlePairDevice
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 * @param[in] id Same as \ref btmuBleDisconnect.
 */
Result btmuBlePairDevice(BtdrvBleAdvertisePacketParameter param, u32 id);

/**
 * @brief BleUnPairDevice
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 * @param[in] id Same as \ref btmuBleDisconnect.
 */
Result btmuBleUnPairDevice(BtdrvBleAdvertisePacketParameter param, u32 id);

/**
 * @brief BleUnPairDevice2
 * @param[in] addr \ref BtdrvAddress
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuBleUnPairDevice2(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param);

/**
 * @brief BleGetPairedDevices
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 * @param[out] addrs Output array of \ref BtdrvAddress.
 * @param[in] count Size of the addrs array in entries.
 * @param[out] total_out Total output entries. The max is 10.
 */
Result btmuBleGetPairedDevices(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out);

/**
 * @brief AcquireBleServiceDiscoveryEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleServiceDiscoveryEvent(Event* out_event);

/**
 * @brief GetGattServices
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[out] services Output array of \ref BtmuGattService.
 * @param[in] count Size of the services array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattServices(u32 id, BtmuGattService *services, u8 count, u8 *total_out);

/**
 * @brief Same as \ref btmuGetGattServices except this only returns the \ref BtmuGattService which matches the input \ref BtdrvGattAttributeUuid.
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] service \ref BtmuGattService
 * @param[out] flag Whether a \ref BtmuGattService was returned.
 */
Result btmuGetGattService(u32 id, const BtdrvGattAttributeUuid *uuid, BtmuGattService *service, bool *flag);

/**
 * @brief Same as \ref btmuGetGattServices except this only returns \ref BtmuGattService entries where various checks pass with u16 fields.
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[in] unk1 Unknown
 * @param[out] services \ref BtmuGattService
 * @param[in] count Size of the services array in entries. The max is 100.
 * @param[out] out Output value.
 */
Result btmuGetGattIncludedServices(u32 id, u16 unk1, BtmuGattService *services, u8 count, u8 *out);

/**
 * @brief This is similar to \ref btmuGetGattIncludedServices except this only returns 1 \ref BtmuGattService.
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[in] unk1 Unknown
 * @param[out] service \ref BtmuGattService
 * @param[out] flag Whether a \ref BtmuGattService was returned.
 */
Result btmuGetBelongingGattService(u32 id, u16 unk1, BtmuGattService *service, bool *flag);

/**
 * @brief GetGattCharacteristics
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[in] unk1 This controls which \ref BtmuGattCharacteristic entries to return.
 * @param[out] characteristics \ref BtmuGattCharacteristic
 * @param[in] count Size of the characteristics array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattCharacteristics(u32 id, u16 unk1, BtmuGattCharacteristic *characteristics, u8 count, u8 *total_out);

/**
 * @brief GetGattDescriptors
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[in] unk1 This controls which \ref BtmuGattDescriptor entries to return.
 * @param[out] descriptors \ref BtmuGattDescriptor
 * @param[in] count Size of the descriptors array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattDescriptors(u32 id, u16 unk1, BtmuGattDescriptor *descriptors, u8 count, u8 *total_out);

/**
 * @brief AcquireBleMtuConfigEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleMtuConfigEvent(Event* out_event);

/**
 * @brief ConfigureBleMtu
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[in] mtu MTU
 */
Result btmuConfigureBleMtu(u32 id, u16 mtu);

/**
 * @brief GetBleMtu
 * @param[in] id Same as \ref btmuBleDisconnect.
 * @param[out] out Output MTU.
 */
Result btmuGetBleMtu(u32 id, u16 *out);

/**
 * @brief RegisterBleGattDataPath
 * @param[in] path \ref BtmuBleDataPath
 */
Result btmuRegisterBleGattDataPath(const BtmuBleDataPath *path);

/**
 * @brief UnregisterBleGattDataPath
 * @param[in] path \ref BtmuBleDataPath
 */
Result btmuUnregisterBleGattDataPath(const BtmuBleDataPath *path);

