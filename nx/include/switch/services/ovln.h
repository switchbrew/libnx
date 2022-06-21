/**
 * @file ovln.h
 * @brief Overlay notification (ovln:*) service IPC wrapper.
 * @author ndeadly
 * @copyright libnx Authors
 */
#pragma once
#include "../sf/service.h"
#include "../kernel/event.h"

/// SenderSession
typedef struct {
    Service s;
} OvlnSenderSession;

/// ReceiverSession
typedef struct {
    Service s;
} OvlnReceiverSession;

/// Name
typedef struct {
    char name[0x10];
} OvlnName;

/// Message 
typedef struct {
    u32 id;
    u32 size;
    u8 data[0x78];
} OvlnMessage;

Result ovlnsndInitialize(void);
Result ovlnrcvInitialize(void);

void ovlnsndExit(void);
void ovlnrcvExit(void);

Service* ovlnsndGetServiceSession(void);
Service* ovlnrcvGetServiceSession(void);

/**
 * @brief OpenSender
 * @param[in] sender 
 * @param[in] name 
 * @param[in] queue_length 
 */
Result ovlnsndOpenSender(OvlnSenderSession *sender, const OvlnName *name, u64 queue_length);

/**
 * @brief 
 * @param[in] sender 
 */
void ovlnsndCloseSender(OvlnSenderSession *sender);

/**
 * @brief Send
 * @param[in] sender 
 * @param[in] id 
 * @param[in] message 
 */
Result ovlnsndSend(OvlnSenderSession *sender, u64 id,  const OvlnMessage *message);

/**
 * @brief GetUnreceivedMessageCount
 * @param[in] sender 
 * @param[out] count 
 */
Result ovlnsndGetUnreceivedMessageCount(OvlnSenderSession *sender, u32 *count);

/**
 * @brief OpenReceiver
 * @param[out] receiver 
 */
Result ovlnrcvOpenReceiver(OvlnReceiverSession *receiver);

/**
 * @brief CloseReceiver
 * @param[in] receiver 
 */
 */
void ovlnrcvCloseReceiver(OvlnReceiverSession *receiver);

/**
 * @brief AddSource
 * @param[in] receiver 
 * @param[in] source 
 */
Result ovlnrcvAddSource(OvlnReceiverSession *receiver, const OvlnName *source);

/**
 * @brief RemoveSource
 * @param[in] receiver 
 * @param[in] source 
 */
Result ovlnrcvRemoveSource(OvlnReceiverSession *receiver, const OvlnName *source);

/**
 * @brief GetReceiveEventHandle
 * @param[in] receiver 
 * @param[out] out_event 
 */
Result ovlnrcvGetReceiveEventHandle(OvlnReceiverSession *receiver, Event* out_event);

/**
 * @brief Receive
 * @param[in] receiver 
 * @param[out] message 
 */
Result ovlnrcvReceive(OvlnReceiverSession *receiver, OvlnMessage *message);

/**
 * @brief ReceiveWithTick
 * @param[in] receiver 
 * @param[out] message 
 * @param[out] tick 
 */
Result ovlnrcvReceiveWithTick(OvlnReceiverSession *receiver, OvlnMessage *message, s64 *tick);
