/**
 * @file bt.h
 * @brief Bluetooth user (bt) service IPC wrapper.
 * @note See also btdev.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/btdrv.h"
#include "../sf/service.h"

/// Initialize bt. Only available on [5.0.0+].
Result btInitialize(void);

/// Exit bt.
void btExit(void);

/// Gets the Service object for the actual bt service session.
Service* btGetServiceSession(void);

/**
 * @brief LeClientReadCharacteristic
 * @note This is essentially the same as \ref btdrvReadGattCharacteristic.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] unk Unknown
 */
Result btLeClientReadCharacteristic(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, u8 unk);

/**
 * @brief LeClientReadDescriptor
 * @note This is essentially the same as \ref btdrvReadGattDescriptor.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] unk Unknown
 */
Result btLeClientReadDescriptor(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, u8 unk);

/**
 * @brief LeClientWriteCharacteristic
 * @note This is essentially the same as \ref btdrvWriteGattCharacteristic.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 * @param[in] unk Unknown
 * @param[in] flag Flag
 */
Result btLeClientWriteCharacteristic(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const void* buffer, size_t size, u8 unk, bool flag);

/**
 * @brief LeClientWriteDescriptor
 * @note This is essentially the same as \ref btdrvWriteGattDescriptor.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 * @param[in] id2 \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 * @param[in] unk Unknown
 */
Result btLeClientWriteDescriptor(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1, const BtdrvGattId *id2, const void* buffer, size_t size, u8 unk);

/**
 * @brief LeClientRegisterNotification
 * @note This is essentially the same as \ref btdrvRegisterGattNotification.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btLeClientRegisterNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief LeClientDeregisterNotification
 * @note This is essentially the same as \ref btdrvUnregisterGattNotification.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] primary_service PrimaryService
 * @param[in] id0 \ref BtdrvGattId
 * @param[in] id1 \ref BtdrvGattId
 */
Result btLeClientDeregisterNotification(u32 connection_handle, bool primary_service, const BtdrvGattId *id0, const BtdrvGattId *id1);

/**
 * @brief SetLeResponse
 * @param[in] unk Unknown
 * @param[in] uuid0 \ref BtdrvGattAttributeUuid
 * @param[in] uuid1 \ref BtdrvGattAttributeUuid
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 */
Result btSetLeResponse(u8 unk, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1, const void* buffer, size_t size);

/**
 * @brief LeSendIndication
 * @param[in] unk Unknown
 * @param[in] uuid0 \ref BtdrvGattAttributeUuid
 * @param[in] uuid1 \ref BtdrvGattAttributeUuid
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, clamped to max size 0x258.
 * @param[in] flag Flag
 */
Result btLeSendIndication(u8 unk, const BtdrvGattAttributeUuid *uuid0, const BtdrvGattAttributeUuid *uuid1, const void* buffer, size_t size, bool flag);

/**
 * @brief GetLeEventInfo
 * @note This is identical to \ref btdrvGetLeHidEventInfo except different state is used.
 * @note The state used by this is reset after writing the data to output.
 * @param[in] buffer Output buffer. 0x400-bytes from state is written here. See \ref BtdrvLeEventInfo.
 * @param[in] size Output buffer size.
 * @param[out] type Output BleEventType.
 */
Result btGetLeEventInfo(void* buffer, size_t size, u32 *type);

/**
 * @brief RegisterBleEvent
 * @note This is identical to \ref btdrvRegisterBleHidEvent except different state is used.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btRegisterBleEvent(Event* out_event);

