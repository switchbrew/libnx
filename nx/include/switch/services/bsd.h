/**
 * @file bsd.h
 * @brief BSD sockets (bsd:u/s) service IPC wrapper.
 * @author plutoo
 * @author TuxSH
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/tmem.h"

#include <sys/types.h>  // for ssize_t, etc.
#include <sys/socket.h> // for socklen_t
#include <sys/select.h> // for fd_set
#include <poll.h>       // for struct pollfd, ndfs_t

/// Configuration structure for bsdInitalize
typedef struct  {
    u32 version;                ///< Observed 1 on 2.0 LibAppletWeb, 2 on 3.0.

    u32 tcp_tx_buf_size;        ///< Size of the TCP transfer (send) buffer (initial or fixed).
    u32 tcp_rx_buf_size;        ///< Size of the TCP recieve buffer (initial or fixed).
    u32 tcp_tx_buf_max_size;    ///< Maximum size of the TCP transfer (send) buffer. If it is 0, the size of the buffer is fixed to its initial value.
    u32 tcp_rx_buf_max_size;    ///< Maximum size of the TCP receive buffer. If it is 0, the size of the buffer is fixed to its initial value.

    u32 udp_tx_buf_size;        ///< Size of the UDP transfer (send) buffer (typically 0x2400 bytes).
    u32 udp_rx_buf_size;        ///< Size of the UDP receive buffer (typically 0xA500 bytes).

    u32 sb_efficiency;          ///< Number of buffers for each socket (standard values range from 1 to 8).
} BsdBufferConfig;

__thread Result t_bsdResult;    ///< Last Switch "result", per-thread
__thread int t_bsdErrno;        ///< Last errno, per-thread

const BsdBufferConfig *bsdGetDefaultBufferConfig(void);
Result bsdInitialize(const BsdBufferConfig *config);
void bsdExit(void);

int bsdSocket(int domain, int type, int protocol);
int bsdSocketExempt(int domain, int type, int protocol);
int bsdOpen(const char *pathname, int flags);
int bsdSelect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int bsdPoll(struct pollfd *fds, nfds_t nfds, int timeout);
int bsdSysctl(const int *name, unsigned int namelen, void *oldp, size_t *oldlenp, const void *newp, size_t newlen);
ssize_t bsdRecv(int sockfd, void *buf, size_t len, int flags);
ssize_t bsdRecvFrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t bsdSend(int sockfd, const void* buf, size_t len, int flags);
ssize_t bsdSendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int bsdAccept(int sockfd, struct sockaddr *address, socklen_t *addrlen);
int bsdBind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int bsdConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int bsdGetPeerName(int sockfd, struct sockaddr *address, socklen_t *addrlen);
int bsdGetSockName(int sockfd, struct sockaddr *address, socklen_t *addrlen);
int bsdGetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int bsdListen(int sockfd, int backlog);
// The following two functions are supposed to be variadic
int bsdIoctl(int fd, int request, void *data);
int bsdFnctl(int fd, int cmd, int flags);
int bsdSetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int bsdShutdown(int sockfd, int how);
int bsdShutdownAllSockets(int how);
ssize_t bsdWrite(int fd, const void *buf, size_t count);
ssize_t bsdRead(int fd, void *buf, size_t count);
int bsdClose(int fd);
int bsdDuplicateSocket(int sockfd);

//TODO: Reverse-engineer GetResourceStatistics, sendmmsg/recvmmsg (custom (un)serialization)

static inline Result bsdInitializeDefault(void)
{
    return bsdInitialize(bsdGetDefaultBufferConfig());
}
