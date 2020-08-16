/**
 * @file btdev.h
 * @brief Wrapper around the bt/btmu services for using bluetooth BLE.
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
Result btdevGetBleScanParameter(u16 unk, BtdrvBleAdvertisePacketParameter *out);

/// Wrapper for \ref btmuGetBleScanFilterParameter2.
Result btdevGetBleScanParameter2(u16 unk, BtdrvGattAttributeUuid *out);

/// Wrapper for \ref btdevStartBleScanGeneral.
Result btdevStartBleScanGeneral(BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuStopBleScanForGeneral.
Result btdevStopBleScanGeneral(void);

/// Wrapper for \ref btmuGetBleScanResultsForGeneral and \ref btmuGetBleScanResultsForSmartDevice.
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
Result btdevDisconnectFromGattServer(u32 id);

/// Wrapper for \ref btmuBleGetConnectionState.
Result btdevGetBleConnectionInfoList(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out);

/// Wrapper for \ref btmuAcquireBleServiceDiscoveryEvent.
Result btdevAcquireBleServiceDiscoveryEvent(Event* out_event);

/// Wrapper for \ref btmuGetGattServices.
Result btdevGetGattServices(u32 connection_handle, BtdevGattService *services, u8 count, u8 *total_out);

/// Wrapper for \ref btmuGetGattService.
Result btdevGetGattService(u32 connection_handle, const BtdrvGattAttributeUuid *uuid, BtdevGattService *service, bool *flag);

/// Wrapper for \ref btmuAcquireBlePairingEvent.
Result btdevAcquireBlePairingEvent(Event* out_event);

/// Wrapper for \ref btmuBlePairDevice.
Result btdevPairGattServer(BtdrvBleAdvertisePacketParameter param, u32 id);

/// Wrapper for \ref btmuBleUnPairDevice.
Result btdevUnpairGattServer(BtdrvBleAdvertisePacketParameter param, u32 id);

/// Wrapper for \ref btmuBleUnPairDevice2.
Result btdevUnpairGattServer2(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param);

/// Wrapper for \ref btmuBleGetPairedDevices.
Result btdevGetPairedGattServerAddress(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out);

/// Wrapper for \ref btmuAcquireBleMtuConfigEvent.
Result btdevAcquireBleMtuConfigEvent(Event* out_event);

/// Wrapper for \ref btmuConfigureBleMtu. mtu must be 0x18-0x200.
Result btdevConfigureBleMtu(u32 id, u16 mtu);

/// Wrapper for \ref btmuGetBleMtu.
Result btdevGetBleMtu(u32 id, u16 *out);

/// Wrapper for \ref btRegisterBleEvent.
Result btdevAcquireBleGattOperationEvent(Event* out_event);

/// Wrapper for \ref btdevRegisterGattOperationNotification.
Result btdevRegisterGattOperationNotification(const BtdrvGattAttributeUuid *uuid);

/// Wrapper for \ref btdevUnregisterGattOperationNotification.
Result btdevUnregisterGattOperationNotification(const BtdrvGattAttributeUuid *uuid);

/// Wrapper for \ref btGetLeEventInfo.
Result btdevGetGattOperationResult(BtdrvBleClientGattOperationInfo *out);

/// Wrapper for \ref btLeClientReadCharacteristic.
Result btdevReadGattCharacteristic(BtdevGattCharacteristic *c);

/// Wrapper for \ref btLeClientWriteCharacteristic.
Result btdevWriteGattCharacteristic(BtdevGattCharacteristic *c);

/// Wrapper for \ref btLeClientRegisterNotification / \ref btLeClientDeregisterNotification, flag controls which func to call.
Result btdevEnableGattCharacteristicNotification(BtdevGattCharacteristic *c, bool flag);

/// Wrapper for \ref btLeClientReadDescriptor.
Result btdevReadGattDescriptor(BtdevGattDescriptor *d);

/// Wrapper for \ref btLeClientWriteDescriptor.
Result btdevWriteGattDescriptor(BtdevGattDescriptor *d);

///@name GattAttribute
///@{

/// Creates a \ref BtdevGattAttribute object.
void btdevGattAttributeCreate(BtdevGattAttribute *a, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle);

/// Gets the Type.
NX_CONSTEXPR u8 btdevGattAttributeGetType(BtdevGattAttribute *a) {
    return a->type;
}

/// Gets the Uuid.
NX_CONSTEXPR void btdevGattAttributeGetUuid(BtdevGattAttribute *a, BtdrvGattAttributeUuid *out) {
    *out = a->uuid;
}

/// Gets the Handle.
NX_CONSTEXPR u16 btdevGattAttributeGetHandle(BtdevGattAttribute *a) {
    return a->handle;
}

/// Gets the ConnectionHandle.
NX_CONSTEXPR u32 btdevGattAttributeGetConnectionHandle(BtdevGattAttribute *a) {
    return a->connection_handle;
}

///@}

///@name GattService
///@{

/// Creates a \ref BtdevGattService object.
void btdevGattServiceCreate(BtdevGattService *s, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle, u16 instance_id, u16 end_group_handle, bool primary_service);

/// Gets the InstanceId.
NX_CONSTEXPR u16 btdevGattServiceGetInstanceId(BtdevGattService *s) {
    return s->instance_id;
}

/// Gets the EndGroupHandle.
NX_CONSTEXPR u16 btdevGattServiceGetEndGroupHandle(BtdevGattService *s) {
    return s->end_group_handle;
}

/// Gets whether this is the PrimaryService.
NX_CONSTEXPR u16 btdevGattServiceIsPrimaryService(BtdevGattService *s) {
    return s->primary_service;
}

/// Wrapper for \ref btmuGetGattIncludedServices.
Result btdevGattServiceGetIncludedServices(BtdevGattService *s, BtdevGattService *services, u8 count, u8 *total_out);

/// Wrapper for \ref btmuGetGattCharacteristics.
Result btdevGattServiceGetCharacteristics(BtdevGattService *s, BtdevGattCharacteristic *characteristic, u8 count, u8 *total_out);

/// Wrapper for \ref btmuGetGattCharacteristics.
Result btdevGattServiceGetCharacteristic(BtdevGattService *s, const BtdrvGattAttributeUuid *uuid, BtdevGattCharacteristic *characteristic, bool *flag);

///@}

///@name GattCharacteristic
///@{

/// Creates a \ref BtdevGattCharacteristic object.
void btdevGattCharacteristicCreate(BtdevGattCharacteristic *c, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle, u16 instance_id, u8 properties);

/// Gets the InstanceId.
NX_CONSTEXPR u16 btdevGattCharacteristicGetInstanceId(BtdevGattCharacteristic *c) {
    return c->instance_id;
}

/// Gets the Properties.
NX_CONSTEXPR u8 btdevGattCharacteristicGetProperties(BtdevGattCharacteristic *c) {
    return c->properties;
}

/// Wrapper for \ref btmuGetBelongingGattService.
Result btdevGattCharacteristicGetService(BtdevGattCharacteristic *c, BtdevGattService *service);

/// Wrapper for \ref btmuGetGattDescriptors.
Result btdevGattCharacteristicGetDescriptors(BtdevGattCharacteristic *c, BtdevGattDescriptor *descriptors, u8 count, u8 *total_out);

/// Same as \ref btdevGattCharacteristicGetDescriptors except this only returns a \ref BtdevGattDescriptor which contains a matching \ref BtdrvGattAttributeUuid.
Result btdevGattCharacteristicGetDescriptor(BtdevGattCharacteristic *c, const BtdrvGattAttributeUuid *uuid, BtdevGattDescriptor *descriptor, bool *flag);

/// Sets the Value in the object, max size is 0x200.
void btdevGattCharacteristicSetValue(BtdevGattCharacteristic *c, const void* buffer, size_t size);

/// Gets the Value in the object, returns the copied value size.
u64 btdevGattCharacteristicGetValue(BtdevGattCharacteristic *c, void* buffer, size_t size);

///@}

///@name GattDescriptor
///@{

/// Creates a \ref BtdevGattDescriptor object.
void btdevGattDescriptorCreate(BtdevGattDescriptor *d, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle);

/// Wrapper for \ref btmuGetBelongingGattService.
Result btdevGattDescriptorGetService(BtdevGattDescriptor *d, BtdevGattService *service);

/// Wrapper for \ref btmuGetGattCharacteristics.
Result btdevGattDescriptorGetCharacteristic(BtdevGattDescriptor *d, BtdevGattCharacteristic *characteristic);

/// Sets the Value in the object, max size is 0x200.
void btdevGattDescriptorSetValue(BtdevGattDescriptor *d, const void* buffer, size_t size);

/// Gets the Value in the object, returns the copied value size.
u64 btdevGattDescriptorGetValue(BtdevGattDescriptor *d, void* buffer, size_t size);

///@}

