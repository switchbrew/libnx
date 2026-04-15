/**
 * @file ovln.h
 * @brief Overlay notification (ovln:*) service IPC wrapper.
 * @author ndeadly
 * @copyright libnx Authors
 */
#pragma once
#include "../sf/service.h"
#include "../kernel/event.h"

/// EnqueuePosition
typedef enum {
    OvlnEnqueuePosition_Front = 0,
    OvlnEnqueuePosition_Back  = 1,
} OvlnEnqueuePosition;

/// OverflowOption
typedef enum {
    OvlnOverflowOption_Error       = 0,
    OvlnOverflowOption_RemoveFront = 1,
    OvlnOverflowOption_RemoveBack  = 2,
    OvlnOverflowOption_Block       = 3,
} OvlnOverflowOption;

/// SourceName
typedef struct {
    char name[0x10];        ///< Source name. Official software always uses the name "overlay".
} OvlnSourceName;

/// RawMessage 
typedef struct {
    u32 tag;                ///< Unique tag specifying the message type.
    u32 data_size;          ///< Size of the below data.
    u8 data[0x78];          ///< Message data.
} OvlnRawMessage;

/// QueueAttribute
typedef struct {
    u32 queue_length;       ///< Length of send queue.
    u8 reserved[4];         ///< Reserved.
} OvlnQueueAttribute;

/// SendOption
typedef struct {
    u8 enqueue_position;    ///< \ref OvlnEnqueuePosition
    u8 overflow_option;     ///< \ref OvlnOverflowOption
    u8 reserved[6];         ///< Reserved.
} OvlnSendOption;

/// Receiver
typedef struct {
    Service s;              ///< IReceiver
} OvlnReceiver;

/// Sender
typedef struct {
    Service s;              ///< ISender
} OvlnSender;

/// Initialize ovln:rcv.
Result ovlnrcvInitialize(void);

/// Exit ovln:rcv.
void ovlnrcvExit(void);

/// Initialize ovln:snd.
Result ovlnsndInitialize(void);

/// Exit ovln:snd.
void ovlnsndExit(void);

/// Gets the Service object for the actual ovln:rcv service session.
Service* ovlnrcvGetServiceSession(void);

/// Gets the Service object for the actual ovln:snd service session.
Service* ovlnsndGetServiceSession(void);

/**
 * @brief OpenReceiver
 * @param[out] r \ref OvlnReceiver
 */
Result ovlnrcvOpenReceiver(OvlnReceiver *r);

/**
 * @brief CloseReceiver
 * @param[in] r \ref OvlnReceiver
 */
void ovlnrcvCloseReceiver(OvlnReceiver *r);

/**
 * @brief AddSource
 * @param[in] r \ref OvlnReceiver
 * @param[in] name \ref OvlnSourceName
 */
Result ovlnrcvAddSource(OvlnReceiver *r, const OvlnSourceName *name);

/**
 * @brief RemoveSource
 * @param[in] r \ref OvlnReceiver
 * @param[in] name \ref OvlnSourceName
 */
Result ovlnrcvRemoveSource(OvlnReceiver *r, const OvlnSourceName *name);

/**
 * @brief GetReceiveEventHandle
 * @param[in] r \ref OvlnReceiver
 * @param[out] out_event Output Event with autoclear=true.
 */
Result ovlnrcvGetReceiveEventHandle(OvlnReceiver *r, Event* out_event);

/**
 * @brief Receive
 * @param[in] r \ref OvlnReceiver
 * @param[out] message \ref OvlnRawMessage
 */
Result ovlnrcvReceive(OvlnReceiver *r, OvlnRawMessage *message);

/**
 * @brief ReceiveWithTick
 * @param[in] r \ref OvlnReceiver
 * @param[out] message \ref OvlnRawMessage
 * @param[out] tick System tick.
 */
Result ovlnrcvReceiveWithTick(OvlnReceiver *r, OvlnRawMessage *message, s64 *tick);

/**
 * @brief OpenSender
 * @param[in] s \ref OvlnSender
 * @param[in] name \ref OvlnSourceName
 * @param[in] attribute \ref OvlnQueueAttribute
 */
Result ovlnsndOpenSender(OvlnSender *s, const OvlnSourceName *name, OvlnQueueAttribute attribute);

/** 
 * @brief CloseSender
 * @param[in] s \ref OvlnSender
 */
void ovlnsndCloseSender(OvlnSender *s);

/**
 * @brief Send
 * @param[in] s \ref OvlnSender
 * @param[in] option \ref OvlnSendOption
 * @param[in] message \ref OvlnRawMessage
 */
Result ovlnsndSend(OvlnSender *s, OvlnSendOption option, const OvlnRawMessage *message);

/**
 * @brief GetUnreceivedMessageCount
 * @param[in] s \ref OvlnSender
 * @param[out] count number of unreceived messages.
 */
Result ovlnsndGetUnreceivedMessageCount(OvlnSender *s, u32 *count);
