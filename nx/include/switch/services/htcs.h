/**
 * @file htcs.h
 * @brief HTC sockets (htcs) service IPC wrapper. Please use <<TODO>> instead.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../sf/service.h"

#define HTCS_PEER_NAME_MAX 32
#define HTCS_PORT_NAME_MAX 32

#define HTCS_SESSION_COUNT_MAX 0x10
#define HTCS_SOCKET_COUNT_MAX  40
#define HTCS_FD_SET_SIZE       HTCS_SOCKET_COUNT_MAX

typedef uint16_t HtcsAddressFamilyType;

typedef struct {
    char name[HTCS_PEER_NAME_MAX];
} HtcsPeerName;

typedef struct {
    char name[HTCS_PORT_NAME_MAX];
} HtcsPortName;

typedef struct {
    HtcsAddressFamilyType family;
    HtcsPeerName peer_name;
    HtcsPortName port_name;
} HtcsSockAddr;

typedef struct {
    s64 tv_sec;
    s64 tv_usec;
} HtcsTimeVal;

typedef struct {
    int fds[HTCS_FD_SET_SIZE];
} HtcsFdSet;

typedef enum {
    HTCS_ENONE         =  0,
    HTCS_EACCES        =  2,
    HTCS_EADDRINUSE    =  3,
    HTCS_EADDRNOTAVAIL =  4,
    HTCS_EAGAIN        =  6,
    HTCS_EALREADY      =  7,
    HTCS_EBADF         =  8,
    HTCS_EBUSY         = 10,
    HTCS_ECONNABORTED  = 13,
    HTCS_ECONNREFUSED  = 14,
    HTCS_ECONNRESET    = 15,
    HTCS_EDESTADDRREQ  = 17,
    HTCS_EFAULT        = 21,
    HTCS_EINPROGRESS   = 26,
    HTCS_EINTR         = 27,
    HTCS_EINVAL        = 28,
    HTCS_EIO           = 29,
    HTCS_EISCONN       = 30,
    HTCS_EMFILE        = 33,
    HTCS_EMSGSIZE      = 35,
    HTCS_ENETDOWN      = 38,
    HTCS_ENETRESET     = 39,
    HTCS_ENOBUFS       = 42,
    HTCS_ENOMEM        = 49,
    HTCS_ENOTCONN      = 56,
    HTCS_ETIMEDOUT     = 76,
    HTCS_EUNKNOWN      = 79,

    HTCS_EWOULDBLOCK   = HTCS_EAGAIN,
} HtcsSocketError;

typedef enum {
    HTCS_MSG_PEEK    = 1,
    HTCS_MSG_WAITALL = 2,
} HtcsMessageFlag;

typedef enum {
    HTCS_SHUT_RD   = 0,
    HTCS_SHUT_WR   = 1,
    HTCS_SHUT_RDWR = 2,
} HtcsShutdownType;

typedef enum {
    HTCS_F_GETFL = 3,
    HTCS_F_SETFL = 4,
} HtcsFcntlOperation;

typedef enum {
    HTCS_O_NONBLOCK = 4,
} HtcsFcntlFlag;

typedef enum {
    HTCS_AF_HTCS = 0,
} HtcsAddressFamily;

typedef struct {
    Service s;
} HtcsSocket;


/// Initialize the HTCS service.
Result htcsInitialize(u32 num_sessions);

/// Exit the HTCS service.
void htcsExit(void);

/// Gets the Service object for the actual HTCS manager service session.
Service* htcsGetManagerServiceSession(void);

/// Gets the Service object for the actual HTCS monitor service session.
Service* htcsGetMonitorServiceSession(void);

/// Manager functionality.
Result htcsGetPeerNameAny(HtcsPeerName *out);
Result htcsGetDefaultHostName(HtcsPeerName *out);
Result htcsCreateSocket(s32 *out_err, HtcsSocket *out, bool enable_disconnection_emulation);
Result htcsStartSelect(u32 *out_task_id, Handle *out_event_handle, const s32 *read, size_t num_read, const s32 *write, size_t num_write, const s32 *except, size_t num_except, s64 tv_sec, s64 tv_usec);
Result htcsEndSelect(s32 *out_err, s32 *out_count, s32 *read, size_t num_read, s32 *write, size_t num_write, s32 *except, size_t num_except, u32 task_id);

/// Socket functionality.
Result htcsSocketClose(HtcsSocket *s, s32 *out_err, s32 *out_res);
Result htcsSocketConnect(HtcsSocket *s, s32 *out_err, s32 *out_res, const HtcsSockAddr *address);
Result htcsSocketBind(HtcsSocket *s, s32 *out_err, s32 *out_res, const HtcsSockAddr *address);
Result htcsSocketListen(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 backlog_count);
Result htcsSocketShutdown(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 how);
Result htcsSocketFcntl(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 command, s32 value);

Result htcsSocketAcceptStart(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle);
Result htcsSocketAcceptResults(HtcsSocket *s, s32 *out_err, HtcsSocket *out_socket, HtcsSockAddr *out_address, u32 task_id);

Result htcsSocketRecvStart(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, s32 mem_size, s32 flags);
Result htcsSocketRecvResults(HtcsSocket *s, s32 *out_err, s64 *out_size, void *buffer, size_t buffer_size, u32 task_id);

Result htcsSocketSendStart(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, const void *buffer, size_t buffer_size, s32 flags);
Result htcsSocketSendResults(HtcsSocket *s, s32 *out_err, s64 *out_size, u32 task_id);

Result htcsSocketStartSend(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, s64 *out_max_size, s64 size, s32 flags);
Result htcsSocketContinueSend(HtcsSocket *s, s64 *out_size, bool *out_wait, const void *buffer, size_t buffer_size, u32 task_id);
Result htcsSocketEndSend(HtcsSocket *s, s32 *out_err, s64 *out_size, u32 task_id);

Result htcsSocketStartRecv(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, s64 size, s32 flags);
Result htcsSocketEndRecv(HtcsSocket *s, s32 *out_err, s64 *out_size, void *buffer, size_t buffer_size, u32 task_id);

Result htcsSocketGetPrimitive(HtcsSocket *s, s32 *out);

void htcsCloseSocket(HtcsSocket *s);
