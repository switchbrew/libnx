/**
 * @file btmu.h
 * @brief btm:u (btm user) service IPC wrapper.
 * @note Only available on [5.0.0+].
 * @note See also btdev.
 * @note See also: https://switchbrew.org/wiki/BTM_services
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/btdrv_types.h"
#include "../services/btm.h"
#include "../sf/service.h"

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
 * @note This is similar to \ref btmAcquireBleScanEvent.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleScanEvent(Event* out_event);

/**
 * @brief GetBleScanFilterParameter
 * @note This is the same as \ref btmGetBleScanParameterGeneral.
 * @param[in] parameter_id Must be value 0x1 or 0xFFFF.
 * @param[out] out \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuGetBleScanFilterParameter(u16 parameter_id, BtdrvBleAdvertisePacketParameter *out);

/**
 * @brief GetBleScanFilterParameter2
 * @note This is the same as \ref btmGetBleScanParameterSmartDevice.
 * @param[in] parameter_id Must be value 0x2.
 * @param[out] out \ref BtdrvGattAttributeUuid. The first 4-bytes is always 0.
 */
Result btmuGetBleScanFilterParameter2(u16 parameter_id, BtdrvGattAttributeUuid *out);

/**
 * @brief StartBleScanForGeneral
 * @note This is similar to \ref btmStartBleScanForGeneral.
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuStartBleScanForGeneral(BtdrvBleAdvertisePacketParameter param);

/**
 * @brief StopBleScanForGeneral
 * @note This is similar to \ref btmStopBleScanForGeneral.
 */
Result btmuStopBleScanForGeneral(void);

/**
 * @brief GetBleScanResultsForGeneral
 * @note This is similar to \ref btmGetBleScanResultsForGeneral.
 * @param[out] results Output array of \ref BtdrvBleScanResult.
 * @param[in] count Size of the results array in entries. The max is 10.
 * @param[out] total_out Total output entries.
 */
Result btmuGetBleScanResultsForGeneral(BtdrvBleScanResult *results, u8 count, u8 *total_out);

/**
 * @brief StartBleScanForPaired
 * @note This is similar to \ref btmStartBleScanForPaired.
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuStartBleScanForPaired(BtdrvBleAdvertisePacketParameter param);

/**
 * @brief StopBleScanForPaired
 * @note This is similar to \ref btmStopBleScanForPaired.
 */
Result btmuStopBleScanForPaired(void);

/**
 * @brief StartBleScanForSmartDevice
 * @note This is similar to \ref btmStartBleScanForSmartDevice.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btmuStartBleScanForSmartDevice(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief StopBleScanForSmartDevice
 * @note This is similar to \ref btmStopBleScanForSmartDevice.
 */
Result btmuStopBleScanForSmartDevice(void);

/**
 * @brief GetBleScanResultsForSmartDevice
 * @note This is similar to \ref btmGetBleScanResultsForSmartDevice.
 * @param[out] results Output array of \ref BtdrvBleScanResult.
 * @param[in] count Size of the results array in entries. The max is 10.
 * @param[out] total_out Total output entries.
 */
Result btmuGetBleScanResultsForSmartDevice(BtdrvBleScanResult *results, u8 count, u8 *total_out);

/**
 * @brief AcquireBleConnectionEvent
 * @note This is similar to \ref btmAcquireBleConnectionEvent.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleConnectionEvent(Event* out_event);

/**
 * @brief BleConnect
 * @note This is similar to \ref btmBleConnect.
 * @param[in] addr \ref BtdrvAddress
 */
Result btmuBleConnect(BtdrvAddress addr);

/**
 * @brief BleDisconnect
 * @note This is similar to \ref btmBleDisconnect.
 * @param[in] connection_handle This must match a BtdrvBleConnectionInfo::connection_handle from \ref btmuBleGetConnectionState. [5.1.0+] 0xFFFFFFFF is invalid.
 */
Result btmuBleDisconnect(u32 connection_handle);

/**
 * @brief BleGetConnectionState
 * @note This is similar to \ref btmBleGetConnectionState.
 * @param[out] info Output array of \ref BtdrvBleConnectionInfo.
 * @param[in] count Size of the info array in entries. Other cmds which use this internally use count=4.
 * @param[out] total_out Total output entries.
 */
Result btmuBleGetConnectionState(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out);

/**
 * @brief AcquireBlePairingEvent
 * @note This is similar to \ref btmAcquireBlePairingEvent.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBlePairingEvent(Event* out_event);

/**
 * @brief BlePairDevice
 * @note This is similar to \ref btmBlePairDevice.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuBlePairDevice(u32 connection_handle, BtdrvBleAdvertisePacketParameter param);

/**
 * @brief BleUnPairDevice
 * @note This is similar to \ref btmBleUnpairDeviceOnBoth.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuBleUnPairDevice(u32 connection_handle, BtdrvBleAdvertisePacketParameter param);

/**
 * @brief BleUnPairDevice2
 * @note This is similar to \ref btmBleUnPairDevice.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 */
Result btmuBleUnPairDevice2(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param);

/**
 * @brief BleGetPairedDevices
 * @note This is similar to \ref btmBleGetPairedAddresses.
 * @param[in] param \ref BtdrvBleAdvertisePacketParameter
 * @param[out] addrs Output array of \ref BtdrvAddress.
 * @param[in] count Size of the addrs array in entries.
 * @param[out] total_out Total output entries. The max is 10.
 */
Result btmuBleGetPairedDevices(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out);

/**
 * @brief AcquireBleServiceDiscoveryEvent
 * @note This is similar to \ref btmAcquireBleServiceDiscoveryEvent.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleServiceDiscoveryEvent(Event* out_event);

/**
 * @brief GetGattServices
 * @note This is similar to \ref btmGetGattServices.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[out] services Output array of \ref BtmGattService.
 * @param[in] count Size of the services array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattServices(u32 connection_handle, BtmGattService *services, u8 count, u8 *total_out);

/**
 * @brief Same as \ref btmuGetGattServices except this only returns the \ref BtmGattService which matches the input \ref BtdrvGattAttributeUuid.
 * @note This is similar to \ref btmGetGattService.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] service \ref BtmGattService
 * @param[out] flag Whether a \ref BtmGattService was returned.
 */
Result btmuGetGattService(u32 connection_handle, const BtdrvGattAttributeUuid *uuid, BtmGattService *service, bool *flag);

/**
 * @brief Same as \ref btmuGetGattServices except this only returns \ref BtmGattService entries where various checks pass with u16 fields.
 * @note This is similar to \ref btmGetGattIncludedServices.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] service_handle ServiceHandle
 * @param[out] services \ref BtmGattService
 * @param[in] count Size of the services array in entries. The max is 100.
 * @param[out] out Output value.
 */
Result btmuGetGattIncludedServices(u32 connection_handle, u16 service_handle, BtmGattService *services, u8 count, u8 *out);

/**
 * @brief This is similar to \ref btmuGetGattIncludedServices except this only returns 1 \ref BtmGattService.
 * @note This is similar to \ref btmGetBelongingService.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] attribute_handle AttributeHandle
 * @param[out] service \ref BtmGattService
 * @param[out] flag Whether a \ref BtmGattService was returned.
 */
Result btmuGetBelongingGattService(u32 connection_handle, u16 attribute_handle, BtmGattService *service, bool *flag);

/**
 * @brief GetGattCharacteristics
 * @note This is similar to \ref btmGetGattCharacteristics.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] service_handle This controls which \ref BtmGattCharacteristic entries to return.
 * @param[out] characteristics \ref BtmGattCharacteristic
 * @param[in] count Size of the characteristics array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattCharacteristics(u32 connection_handle, u16 service_handle, BtmGattCharacteristic *characteristics, u8 count, u8 *total_out);

/**
 * @brief GetGattDescriptors
 * @note This is similar to \ref btmGetGattDescriptors.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] char_handle Characteristic handle. This controls which \ref BtmGattDescriptor entries to return.
 * @param[out] descriptors \ref BtmGattDescriptor
 * @param[in] count Size of the descriptors array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btmuGetGattDescriptors(u32 connection_handle, u16 char_handle, BtmGattDescriptor *descriptors, u8 count, u8 *total_out);

/**
 * @brief AcquireBleMtuConfigEvent
 * @note This is similar to \ref btmAcquireBleMtuConfigEvent.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmuAcquireBleMtuConfigEvent(Event* out_event);

/**
 * @brief ConfigureBleMtu
 * @note This is similar to \ref btmConfigureBleMtu.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] mtu MTU
 */
Result btmuConfigureBleMtu(u32 connection_handle, u16 mtu);

/**
 * @brief GetBleMtu
 * @note This is similar to \ref btmGetBleMtu.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[out] out Output MTU.
 */
Result btmuGetBleMtu(u32 connection_handle, u16 *out);

/**
 * @brief RegisterBleGattDataPath
 * @note This is similar to \ref btmRegisterBleGattDataPath.
 * @param[in] path \ref BtmBleDataPath
 */
Result btmuRegisterBleGattDataPath(const BtmBleDataPath *path);

/**
 * @brief UnregisterBleGattDataPath
 * @note This is similar to \ref btmUnregisterBleGattDataPath.
 * @param[in] path \ref BtmBleDataPath
 */
Result btmuUnregisterBleGattDataPath(const BtmBleDataPath *path);

