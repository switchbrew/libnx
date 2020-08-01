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
    u8 unk_x0[0x24];              ///< Unknown
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
    u8 unk_x0[0x18];              ///< Unknown
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
 * @param[in] unk Unknown
 * @param[out] out \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuGetBleScanFilterParameter(u16 unk, BtdrvBleAdvertisePacketParameter *out);

/**
 * @brief GetBleScanFilterParameter2
 * @param[in] unk Unknown
 * @param[out] out \ref BtdrvGattAttributeUuid
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
 * @param[in] count Size of the results array in entries.
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
 * @param[in] count Size of the results array in entries.
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
 * @param[in] addr \ref BtdrvAddress
 */
Result btmuBleConnect(BtdrvAddress addr);

/**
 * @brief BleDisconnect
 * @param[in] unk Unknown
 */
Result btmuBleDisconnect(u32 unk);

/**
 * @brief BleGetConnectionState
 * @param[out] info Output array of \ref BtdrvBleConnectionInfo.
 * @param[in] count Size of the info array in entries.
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
 * @param[in] unk Unknown
 */
Result btmuBlePairDevice(BtdrvBleAdvertisePacketParameter param, u32 unk);

/**
 * @brief BleUnPairDevice
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 * @param[in] unk Unknown
 */
Result btmuBleUnPairDevice(BtdrvBleAdvertisePacketParameter param, u32 unk);

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
 * @param[out] total_out Total output entries.
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
 * @param[in] unk Unknown
 * @param[out] services Output array of \ref BtmuGattService.
 * @param[in] count Size of the services array in entries.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattServices(u32 unk, BtmuGattService *services, u8 count, u8 *total_out);

/**
 * @brief GetGattService
 * @param[in] unk Unknown
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] service \ref BtmuGattService
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattService(u32 unk, const BtdrvGattAttributeUuid *uuid, BtmuGattService *service, u8 *total_out);

/**
 * @brief GetGattIncludedServices
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[out] services \ref BtmuGattService
 * @param[in] count Size of the services array in entries.
 * @param[out] out Output value.
 */
Result btmuGetGattIncludedServices(u32 unk0, u16 unk1, BtmuGattService *services, u8 count, u8 *out);

/**
 * @brief GetBelongingGattService
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[out] service \ref BtmuGattService
 * @param[out] total_out Total output entries.
 */
Result btmuGetBelongingGattService(u32 unk0, u16 unk1, BtmuGattService *service, u8 *total_out);

/**
 * @brief GetGattCharacteristics
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[out] characteristics \ref BtmuGattCharacteristic
 * @param[in] count Size of the characteristics array in entries.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattCharacteristics(u32 unk0, u16 unk1, BtmuGattCharacteristic *characteristics, u8 count, u8 *total_out);

/**
 * @brief GetGattDescriptors
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[out] descriptors \ref BtmuGattDescriptor
 * @param[in] count Size of the descriptors array in entries.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattDescriptors(u32 unk0, u16 unk1, BtmuGattDescriptor *descriptors, u8 count, u8 *total_out);

/**
 * @brief AcquireBleMtuConfigEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleMtuConfigEvent(Event* out_event);

/**
 * @brief ConfigureBleMtu
 * @param[in] unk Unknown
 * @param[in] mtu MTU
 */
Result btmuConfigureBleMtu(u32 unk, u16 mtu);

/**
 * @brief GetBleMtu
 * @param[in] unk Unknown
 * @param[out] out Output MTU.
 */
Result btmuGetBleMtu(u32 unk, u16 *out);

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

