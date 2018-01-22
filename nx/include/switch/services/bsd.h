#pragma once
#include "types.h"
#include "kernel/tmem.h"

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
} BsdConfig;

struct bsd_sockaddr_in {
    u8  sin_len;
    u8  sin_family;
    u16 sin_port;
    u32 sin_addr;
    u8  sin_zero[8];
};

const BsdConfig *bsdGetDefaultConfig(void);
Result bsdInitialize(TransferMemory* tmem, const BsdConfig *config);
int bsdGetErrno(void);
int bsdConnect(int sockfd, const void* addr, u32 addrlen);
int bsdSocket(int domain, int type, int protocol);
int bsdBind(int sockfd, const void* addr, u32 addrlen);
int bsdListen(int sockfd, int backlog);
int bsdSend(int sockfd, const void* buffer, size_t length, int flags);
int bsdSendTo(int sockfd, const void* buffer, size_t length, int flags, const struct bsd_sockaddr_in *dest_addr, size_t dest_len);
int bsdRecv(int sockfd, void* buffer, size_t length, int flags);
int bsdSetSockOpt(int sockfd, int level, int option_name, const void *option_value, size_t option_size);
int bsdWrite(int sockfd, const void* buffer, size_t length);

/**
 * @brief Computes the minimal size of the transfer memory to be passed to @ref bsdInitalize.
 *        Should the transfer memory be smaller than that, the BSD sockets service would only send
 *        ZeroWindow packets (for TCP), resulting in a transfer rate not exceeding 1 byte/s.
 * @param config Pointer to the BSD sockets service configuration.
 */
static inline size_t bsdGetTransferMemSizeForConfig(const BsdConfig *config)
{
    u32 tcp_tx_buf_max_size = config->tcp_tx_buf_max_size != 0 ? config->tcp_tx_buf_max_size : config->tcp_tx_buf_size;
    u32 tcp_rx_buf_max_size = config->tcp_rx_buf_max_size != 0 ? config->tcp_rx_buf_max_size : config->tcp_rx_buf_size;
    u32 sum = tcp_tx_buf_max_size + tcp_rx_buf_max_size + config->udp_tx_buf_size + config->udp_rx_buf_size;

    sum = ((sum + 0xFFF) >> 12) << 12; // page round-up
    return (size_t)(config->sb_efficiency * sum);
}

static inline size_t bsdGetTransferMemSizeForDefaultConfig(void)
{
    return bsdGetTransferMemSizeForConfig(bsdGetDefaultConfig());
}

static inline Result bsdInitializeDefault(TransferMemory* tmem)
{
    return bsdInitialize(tmem, bsdGetDefaultConfig());
}

#define BSD_AF_INET 2
#define BSD_AF_INET6 10
#define BSD_IPPROTO_IP 0
#define BSD_IPPROTO_TCP 6
#define BSD_IPPROTO_UDP 17

#define BSD_SOCK_STREAM 1
#define BSD_SOCK_DGRAM 2

#define BSD_MSG_RECV_ALL 0x40
