/**
 * @file bt.h
 * @brief Bluetooth user (bt) service IPC wrapper.
 * @note See also btdev.
 * @author yellows8, ndeadly
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
 * @param[in] is_primary Is a primary service or not
 * @param[in] serv_id Service GATT ID \ref BtdrvGattId
 * @param[in] char_id Characteristic GATT ID \ref BtdrvGattId
 * @param[in] auth_req \ref BtdrvGattAuthReqType
 */
Result btLeClientReadCharacteristic(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, u8 auth_req);

/**
 * @brief LeClientReadDescriptor
 * @note This is essentially the same as \ref btdrvReadGattDescriptor.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] is_primary Is a primary service or not
 * @param[in] serv_id Service GATT ID \ref BtdrvGattId
 * @param[in] char_id Characteristic GATT ID \ref BtdrvGattId
 * @param[in] desc_id Descriptor GATT ID \ref BtdrvGattId
 * @param[in] auth_req \ref BtdrvGattAuthReqType
 */
Result btLeClientReadDescriptor(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, u8 auth_req);

/**
 * @brief LeClientWriteCharacteristic
 * @note This is essentially the same as \ref btdrvWriteGattCharacteristic.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] is_primary Is a primary service or not
 * @param[in] serv_id Service GATT ID \ref BtdrvGattId
 * @param[in] char_id Characteristic GATT ID \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 * @param[in] auth_req \ref BtdrvGattAuthReqType
 * @param[in] with_response Whether to use Write-With-Response write type or not
 */
Result btLeClientWriteCharacteristic(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const void* buffer, size_t size, u8 auth_req, bool with_response);

/**
 * @brief LeClientWriteDescriptor
 * @note This is essentially the same as \ref btdrvWriteGattDescriptor.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] is_primary Is a primary service or not
 * @param[in] serv_id Service GATT ID \ref BtdrvGattId
 * @param[in] char_id Characteristic GATT ID \ref BtdrvGattId
 * @param[in] desc_id Descriptor GATT ID \ref BtdrvGattId
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 * @param[in] auth_req \ref BtdrvGattAuthReqType
 */
Result btLeClientWriteDescriptor(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id, const BtdrvGattId *desc_id, const void* buffer, size_t size, u8 auth_req);

/**
 * @brief LeClientRegisterNotification
 * @note This is essentially the same as \ref btdrvRegisterGattNotification.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] is_primary Is a primary service or not
 * @param[in] serv_id Service GATT ID \ref BtdrvGattId
 * @param[in] char_id Characteristic GATT ID \ref BtdrvGattId
 */
Result btLeClientRegisterNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id);

/**
 * @brief LeClientDeregisterNotification
 * @note This is essentially the same as \ref btdrvUnregisterGattNotification.
 * @param[in] connection_handle ConnectionHandle
 * @param[in] is_primary Is a primary service or not
 * @param[in] serv_id Service GATT ID \ref BtdrvGattId
 * @param[in] char_id Characteristic GATT ID \ref BtdrvGattId
 */
Result btLeClientDeregisterNotification(u32 connection_handle, bool is_primary, const BtdrvGattId *serv_id, const BtdrvGattId *char_id);

/**
 * @brief SetLeResponse
 * @param[in] server_if Server interface ID
 * @param[in] serv_uuid Service UUID \ref BtdrvGattAttributeUuid
 * @param[in] char_uuid Characteristic UUID \ref BtdrvGattAttributeUuid
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, must be <=0x258.
 */
Result btSetLeResponse(u8 server_if, const BtdrvGattAttributeUuid *serv_uuid, const BtdrvGattAttributeUuid *char_uuid, const void* buffer, size_t size);

/**
 * @brief LeSendIndication
 * @param[in] server_if Server interface ID
 * @param[in] serv_uuid Service UUID \ref BtdrvGattAttributeUuid
 * @param[in] char_uuid Characteristic UUID \ref BtdrvGattAttributeUuid
 * @param[in] buffer Input buffer.
 * @param[in] size Input buffer size, clamped to max size 0x258.
 * @param[in] noconfirm Whether no confirmation is required (notification) or not (indication)
 */
Result btLeSendIndication(u8 server_if, const BtdrvGattAttributeUuid *serv_uuid, const BtdrvGattAttributeUuid *char_uuid, const void* buffer, size_t size, bool noconfirm);

/**
 * @brief GetLeEventInfo
 * @note This is identical to \ref btdrvGetLeHidEventInfo except different state is used.
 * @note The state used by this is reset after writing the data to output.
 * @param[in] buffer Output buffer. 0x400-bytes from state is written here. See \ref BtdrvLeEventInfo.
 * @param[in] size Output buffer size.
 * @param[out] type Output BtdrvBleEventType.
 */
Result btGetLeEventInfo(void* buffer, size_t size, BtdrvBleEventType *type);

/**
 * @brief RegisterBleEvent
 * @note This is identical to \ref btdrvRegisterBleHidEvent except different state is used.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btRegisterBleEvent(Event* out_event);

