#pragma once
#include "../../types.h"

/// Configuration structure for socketInitalize
typedef struct  {
    u32 bsdsockets_version;     ///< Observed 1 on 2.0 LibAppletWeb, 2 on 3.0.

    u32 tcp_tx_buf_size;        ///< Size of the TCP transfer (send) buffer (initial or fixed).
    u32 tcp_rx_buf_size;        ///< Size of the TCP recieve buffer (initial or fixed).
    u32 tcp_tx_buf_max_size;    ///< Maximum size of the TCP transfer (send) buffer. If it is 0, the size of the buffer is fixed to its initial value.
    u32 tcp_rx_buf_max_size;    ///< Maximum size of the TCP receive buffer. If it is 0, the size of the buffer is fixed to its initial value.

    u32 udp_tx_buf_size;        ///< Size of the UDP transfer (send) buffer (typically 0x2400 bytes).
    u32 udp_rx_buf_size;        ///< Size of the UDP receive buffer (typically 0xA500 bytes).

    u32 sb_efficiency;          ///< Number of buffers for each socket (standard values range from 1 to 8).
} SocketInitConfig;

const SocketInitConfig *socketGetDefaultInitConfig(void);
Result socketInitialize(const SocketInitConfig *config);
Result socketGetLastResult(void);

void socketExit(void);

static inline Result socketInitializeDefault(void)
{
    return socketInitialize(socketGetDefaultInitConfig());
}

