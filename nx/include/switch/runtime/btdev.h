/**
 * @file btdev.h
 * @brief Wrapper around the bt/btmu services for using bluetooth BLE.
 * @note Only available on [5.0.0+].
 * @note See also: https://switchbrew.org/wiki/BTM_services
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/btdrv_types.h"

/// GattAttribute
typedef struct {
    u8 type;                                        ///< Type
    BtdrvGattAttributeUuid uuid;                    ///< \ref BtdrvGattAttributeUuid
    u16 handle;                                     ///< Handle
    u32 connection_handle;                          ///< ConnectionHandle
} BtdevGattAttribute;

/// GattService
typedef struct {
    BtdevGattAttribute attr;                        ///< \ref BtdevGattAttribute
    u16 instance_id;                                ///< InstanceId
    u16 end_group_handle;                           ///< EndGroupHandle
    bool primary_service;                           ///< PrimaryService
} BtdevGattService;

/// GattCharacteristic
typedef struct {
    BtdevGattAttribute attr;                        ///< \ref BtdevGattAttribute
    u16 instance_id;                                ///< InstanceId
    u8 properties;                                  ///< Properties
    u64 value_size;                                 ///< Size of value.
    u8 value[0x200];                                ///< Value
} BtdevGattCharacteristic;

/// GattDescriptor
typedef struct {
    BtdevGattAttribute attr;                        ///< \ref BtdevGattAttribute
    u64 value_size;                                 ///< Size of value.
    u8 value[0x200];                                ///< Value
} BtdevGattDescriptor;

/// Initialize bt/btmu.
Result btdevInitialize(void);

/// Exit bt/btmu.
void btdevExit(void);

/// Compares two \ref BtdrvGattAttributeUuid, returning whether these match.
bool btdevGattAttributeUuidIsSame(const BtdrvGattAttributeUuid *a, const BtdrvGattAttributeUuid *b);

/// Wrapper for \ref btmuAcquireBleScanEvent.
Result btdevAcquireBleScanEvent(Event* out_event);

/// Wrapper for \ref btmuGetBleScanFilterParameter.
Result btdevGetBleScanParameter(u16 parameter_id, BtdrvBleAdvertisePacketParameter *out);

/// Wrapper for \ref btmuGetBleScanFilterParameter2.
Result btdevGetBleScanParameter2(u16 parameter_id, BtdrvGattAttributeUuid *out);

/// Wrapper for \ref btdevStartBleScanGeneral.
Result btdevStartBleScanGeneral(BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuStopBleScanForGeneral.
Result btdevStopBleScanGeneral(void);

/**
 * @brief Wrapper for \ref btmuGetBleScanResultsForGeneral and \ref btmuGetBleScanResultsForSmartDevice.
 * @param[out] results Output array of \ref BtdrvBleScanResult.
 * @param[in] count Size of the results array in entries.
 * @param[out] total_out Total output entries.
 */
Result btdevGetBleScanResult(BtdrvBleScanResult *results, u8 count, u8 *total_out);

/// Wrapper for \ref btmuStartBleScanForPaired.
Result btdevEnableBleAutoConnection(BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuStopBleScanForPaired.
Result btdevDisableBleAutoConnection(void);

/// Wrapper for \ref btmuStartBleScanForSmartDevice.
Result btdevStartBleScanSmartDevice(const BtdrvGattAttributeUuid *uuid);

/// Wrapper for \ref btmuStopBleScanForSmartDevice.
Result btdevStopBleScanSmartDevice(void);

/// Wrapper for \ref btmuAcquireBleConnectionEvent.
Result btdevAcquireBleConnectionStateChangedEvent(Event* out_event);

/// Wrapper for \ref btmuBleConnect.
Result btdevConnectToGattServer(BtdrvAddress addr);

/// Wrapper for \ref btmuBleDisconnect.
Result btdevDisconnectFromGattServer(u32 connection_handle);

/// Wrapper for \ref btmuBleGetConnectionState.
Result btdevGetBleConnectionInfoList(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out);

/// Wrapper for \ref btmuAcquireBleServiceDiscoveryEvent.
Result btdevAcquireBleServiceDiscoveryEvent(Event* out_event);

/**
 * @brief Wrapper for \ref btmuGetGattServices.
 * @param[in] connection_handle ConnectionHandle
 * @param[out] services Output array of \ref BtdevGattService.
 * @param[in] count Size of the services array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btdevGetGattServices(u32 connection_handle, BtdevGattService *services, u8 count, u8 *total_out);

/**
 * @brief Wrapper for \ref btmuGetGattService.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] service \ref BtdevGattService
 * @param[out] flag Whether a \ref BtdevGattService was returned.
 */
Result btdevGetGattService(u32 connection_handle, const BtdrvGattAttributeUuid *uuid, BtdevGattService *service, bool *flag);

/// Wrapper for \ref btmuAcquireBlePairingEvent.
Result btdevAcquireBlePairingEvent(Event* out_event);

/// Wrapper for \ref btmuBlePairDevice.
Result btdevPairGattServer(u32 connection_handle, BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuBleUnPairDevice.
Result btdevUnpairGattServer(u32 connection_handle, BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuBleUnPairDevice2.
Result btdevUnpairGattServer2(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuBleGetPairedDevices.
Result btdevGetPairedGattServerAddress(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out);

/// Wrapper for \ref btmuAcquireBleMtuConfigEvent.
Result btdevAcquireBleMtuConfigEvent(Event* out_event);

/**
 * @brief Wrapper for \ref btmuConfigureBleMtu.
 * @param[in] connection_handle Same as \ref btmuBleDisconnect.
 * @param[in] mtu MTU, must be 0x18-0x200.
 */
Result btdevConfigureBleMtu(u32 connection_handle, u16 mtu);

/// Wrapper for \ref btmuGetBleMtu.
Result btdevGetBleMtu(u32 connection_handle, u16 *out);

/// Wrapper for \ref btRegisterBleEvent.
Result btdevAcquireBleGattOperationEvent(Event* out_event);

/**
 * @brief Wrapper for \ref btmuRegisterBleGattDataPath.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdevRegisterGattOperationNotification(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief Wrapper for \ref btmuUnregisterBleGattDataPath.
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 */
Result btdevUnregisterGattOperationNotification(const BtdrvGattAttributeUuid *uuid);

/**
 * @brief Wrapper for \ref btGetLeEventInfo.
 * @param[out] out \ref BtdrvBleClientGattOperationInfo
 */
Result btdevGetGattOperationResult(BtdrvBleClientGattOperationInfo *out);

/**
 * @brief Wrapper for \ref btLeClientReadCharacteristic.
 * @note An error is thrown if the properties from \ref btdevGattCharacteristicGetProperties don't allow using this.
 * @param c \ref BtdevGattCharacteristic
 */
Result btdevReadGattCharacteristic(BtdevGattCharacteristic *c);

/**
 * @brief Wrapper for \ref btLeClientWriteCharacteristic.
 * @note An error is thrown if the properties from \ref btdevGattCharacteristicGetProperties don't allow using this.
 * @note This uses the Value from \ref btdevGattCharacteristicSetValue.
 * @param c \ref BtdevGattCharacteristic
 */
Result btdevWriteGattCharacteristic(BtdevGattCharacteristic *c);

/**
 * @brief Wrapper for \ref btLeClientRegisterNotification / \ref btLeClientDeregisterNotification.
 * @note An error is thrown if the properties from \ref btdevGattCharacteristicGetProperties don't allow using this.
 * @param c \ref BtdevGattCharacteristic
 * @param[in] flag Whether to enable/disable, controls which func to call.
 */
Result btdevEnableGattCharacteristicNotification(BtdevGattCharacteristic *c, bool flag);

/**
 * @brief Wrapper for \ref btLeClientReadDescriptor.
 * @param d \ref BtdevGattDescriptor
 */
Result btdevReadGattDescriptor(BtdevGattDescriptor *d);

/**
 * @brief Wrapper for \ref btLeClientWriteDescriptor.
 * @note This uses the Value from \ref btdevGattDescriptorSetValue.
 * @param d \ref BtdevGattDescriptor
 */
Result btdevWriteGattDescriptor(BtdevGattDescriptor *d);

///@name GattAttribute
///@{

/**
 * @brief Creates a \ref BtdevGattAttribute object. This is intended for internal use.
 * @param a \ref BtdevGattAttribute
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[in] handle Handle
 * @param[in] connection_handle ConnectionHandle
 */
void btdevGattAttributeCreate(BtdevGattAttribute *a, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle);

/**
 * @brief Gets the Type.
 * @param a \ref BtdevGattAttribute
 */
NX_CONSTEXPR u8 btdevGattAttributeGetType(BtdevGattAttribute *a) {
    return a->type;
}

/**
 * @brief Gets the Uuid.
 * @param a \ref BtdevGattAttribute
 * @param[out] out \ref BtdrvGattAttributeUuid
 */
NX_CONSTEXPR void btdevGattAttributeGetUuid(BtdevGattAttribute *a, BtdrvGattAttributeUuid *out) {
    *out = a->uuid;
}

/**
 * @brief Gets the Handle.
 * @param a \ref BtdevGattAttribute
 */
NX_CONSTEXPR u16 btdevGattAttributeGetHandle(BtdevGattAttribute *a) {
    return a->handle;
}

/**
 * @brief Gets the ConnectionHandle.
 * @param a \ref BtdevGattAttribute
 */
NX_CONSTEXPR u32 btdevGattAttributeGetConnectionHandle(BtdevGattAttribute *a) {
    return a->connection_handle;
}

///@}

///@name GattService
///@{

/**
 * @brief Creates a \ref BtdevGattService object. This is intended for internal use.
 * @param s \ref BtdevGattService
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[in] handle Handle
 * @param[in] connection_handle ConnectionHandle
 * @param[in] instance_id InstanceId
 * @param[in] end_group_handle EndGroupHandle
 * @param[in] primary_service PrimaryService
 */
void btdevGattServiceCreate(BtdevGattService *s, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle, u16 instance_id, u16 end_group_handle, bool primary_service);

/**
 * @brief Gets the InstanceId.
 * @param s \ref BtdevGattService
 */
NX_CONSTEXPR u16 btdevGattServiceGetInstanceId(BtdevGattService *s) {
    return s->instance_id;
}

/**
 * @brief Gets the EndGroupHandle.
 * @param s \ref BtdevGattService
 */
NX_CONSTEXPR u16 btdevGattServiceGetEndGroupHandle(BtdevGattService *s) {
    return s->end_group_handle;
}

/**
 * @brief Gets whether this is the PrimaryService.
 * @param s \ref BtdevGattService
 */
NX_CONSTEXPR u16 btdevGattServiceIsPrimaryService(BtdevGattService *s) {
    return s->primary_service;
}

/**
 * @brief Wrapper for \ref btmuGetGattIncludedServices.
 * @param s \ref BtdevGattService
 * @param[out] services Output array of \ref BtdevGattService.
 * @param[in] count Size of the services array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btdevGattServiceGetIncludedServices(BtdevGattService *s, BtdevGattService *services, u8 count, u8 *total_out);

/**
 * @brief Wrapper for \ref btmuGetGattCharacteristics.
 * @param s \ref BtdevGattService
 * @param[out] characteristics Output array of \ref BtdevGattCharacteristic.
 * @param[in] count Size of the characteristics array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btdevGattServiceGetCharacteristics(BtdevGattService *s, BtdevGattCharacteristic *characteristics, u8 count, u8 *total_out);

/**
 * @brief Same as \ref btdevGattServiceGetCharacteristics except this only returns the \ref BtdevGattCharacteristic which contains a matching \ref BtdrvGattAttributeUuid.
 * @param s \ref BtdevGattService
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] characteristic \ref BtdevGattCharacteristic
 * @param[out] flag Whether a \ref BtdevGattService was returned.
 */
Result btdevGattServiceGetCharacteristic(BtdevGattService *s, const BtdrvGattAttributeUuid *uuid, BtdevGattCharacteristic *characteristic, bool *flag);

///@}

///@name GattCharacteristic
///@{

/**
 * @brief Creates a \ref BtdevGattCharacteristic object. This is intended for internal use.
 * @param c \ref BtdevGattCharacteristic
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[in] handle Handle
 * @param[in] connection_handle ConnectionHandle
 * @param[in] instance_id InstanceId
 * @param[in] properties Properties
 */
void btdevGattCharacteristicCreate(BtdevGattCharacteristic *c, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle, u16 instance_id, u8 properties);

/**
 * @brief Gets the InstanceId.
 * @param c \ref BtdevGattCharacteristic
 */
NX_CONSTEXPR u16 btdevGattCharacteristicGetInstanceId(BtdevGattCharacteristic *c) {
    return c->instance_id;
}

/**
 * @brief Gets the Properties.
 * @param c \ref BtdevGattCharacteristic
 */
NX_CONSTEXPR u8 btdevGattCharacteristicGetProperties(BtdevGattCharacteristic *c) {
    return c->properties;
}

/**
 * @brief Wrapper for \ref btmuGetBelongingGattService.
 * @note Gets the \ref BtdevGattService which belongs to this object.
 * @param c \ref BtdevGattCharacteristic.
 * @param[out] service \ref BtdevGattService
 */
Result btdevGattCharacteristicGetService(BtdevGattCharacteristic *c, BtdevGattService *service);

/**
 * @brief Wrapper for \ref btmuGetGattDescriptors.
 * @note Gets the descriptors which belongs to this object.
 * @param c \ref BtdevGattCharacteristic
 * @param[out] descriptors Output array of \ref BtdevGattDescriptor.
 * @param[in] count Size of the descriptors array in entries. The max is 100.
 * @param[out] total_out Total output entries.
 */
Result btdevGattCharacteristicGetDescriptors(BtdevGattCharacteristic *c, BtdevGattDescriptor *descriptors, u8 count, u8 *total_out);

/**
 * @brief Same as \ref btdevGattCharacteristicGetDescriptors except this only returns a \ref BtdevGattDescriptor which contains a matching \ref BtdrvGattAttributeUuid.
 * @param c \ref BtdevGattCharacteristic
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[out] descriptor \ref BtdevGattDescriptor
 * @param[out] flag Whether a \ref BtdevGattDescriptor was returned.
 */
Result btdevGattCharacteristicGetDescriptor(BtdevGattCharacteristic *c, const BtdrvGattAttributeUuid *uuid, BtdevGattDescriptor *descriptor, bool *flag);

/**
 * @brief Sets the Value in the object.
 * @note See also \ref btdevWriteGattCharacteristic.
 * @param c \ref BtdevGattCharacteristic
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, max is 0x200.
 */
void btdevGattCharacteristicSetValue(BtdevGattCharacteristic *c, const void* buffer, size_t size);

/**
 * @brief Gets the Value in the object, returns the copied value size.
 * @param c \ref BtdevGattCharacteristic
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size, max is 0x200.
 */
u64 btdevGattCharacteristicGetValue(BtdevGattCharacteristic *c, void* buffer, size_t size);

///@}

///@name GattDescriptor
///@{

/**
 * @brief Creates a \ref BtdevGattDescriptor object. This is intended for internal use.
 * @param d \ref BtdevGattDescriptor
 * @param[in] uuid \ref BtdrvGattAttributeUuid
 * @param[in] handle Handle
 * @param[in] connection_handle ConnectionHandle
 */
void btdevGattDescriptorCreate(BtdevGattDescriptor *d, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle);

/**
 * @brief Wrapper for \ref btmuGetBelongingGattService.
 * @note Gets the \ref BtdevGattService which belongs to this object.
 * @param d \ref BtdevGattDescriptor
 * @param[out] service \ref BtdevGattService
 */
Result btdevGattDescriptorGetService(BtdevGattDescriptor *d, BtdevGattService *service);

/**
 * @brief Wrapper for \ref btmuGetGattCharacteristics.
 * @note Gets the \ref BtdevGattCharacteristic which belongs to this object.
 * @param d \ref BtdevGattDescriptor
 * @param[out] characteristic \ref BtdevGattCharacteristic
 */
Result btdevGattDescriptorGetCharacteristic(BtdevGattDescriptor *d, BtdevGattCharacteristic *characteristic);

/**
 * @brief Sets the Value in the object.
 * @note See also \ref btdevWriteGattDescriptor.
 * @param d \ref BtdevGattDescriptor
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, max is 0x200.
 */
void btdevGattDescriptorSetValue(BtdevGattDescriptor *d, const void* buffer, size_t size);

/**
 * @brief Gets the Value in the object, returns the copied value size.
 * @param d \ref BtdevGattDescriptor
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size, max is 0x200.
 */
u64 btdevGattDescriptorGetValue(BtdevGattDescriptor *d, void* buffer, size_t size);

///@}

