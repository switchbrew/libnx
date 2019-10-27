#pragma once
#include "../../types.h"

/// BSD service type used by the socket driver.
typedef enum {
    BsdServiceType_User   = BIT(0), ///< Uses bsd:u (default).
    BsdServiceType_System = BIT(1), ///< Uses bsd:s.
    BsdServiceType_Auto   = BsdServiceType_User | BsdServiceType_System, ///< Tries to use bsd:s first, and if that fails uses bsd:u (official software behavior).
} BsdServiceType;

/// Configuration structure for socketInitalize
typedef struct  {
    u32 bsdsockets_version;                     ///< Observed 1 on 2.0 LibAppletWeb, 2 on 3.0.

    u32 tcp_tx_buf_size;                        ///< Size of the TCP transfer (send) buffer (initial or fixed).
    u32 tcp_rx_buf_size;                        ///< Size of the TCP recieve buffer (initial or fixed).
    u32 tcp_tx_buf_max_size;                    ///< Maximum size of the TCP transfer (send) buffer. If it is 0, the size of the buffer is fixed to its initial value.
    u32 tcp_rx_buf_max_size;                    ///< Maximum size of the TCP receive buffer. If it is 0, the size of the buffer is fixed to its initial value.

    u32 udp_tx_buf_size;                        ///< Size of the UDP transfer (send) buffer (typically 0x2400 bytes).
    u32 udp_rx_buf_size;                        ///< Size of the UDP receive buffer (typically 0xA500 bytes).

    u32 sb_efficiency;                          ///< Number of buffers for each socket (standard values range from 1 to 8).

    u32 num_bsd_sessions;                       ///< Number of BSD service sessions (typically 3).
    BsdServiceType bsd_service_type;            ///< BSD service type (typically \ref BsdServiceType_User).
} SocketInitConfig;

/// Fetch the default configuration for the socket driver.
const SocketInitConfig *socketGetDefaultInitConfig(void);
/// Initalize the socket driver.
Result socketInitialize(const SocketInitConfig *config);
/// Fetch the last bsd:u/s Switch result code (thread-local).
Result socketGetLastResult(void);
/// Deinitialize the socket driver.
void socketExit(void);

/// Initalize the socket driver using the default configuration.
NX_INLINE Result socketInitializeDefault(void) {
    return socketInitialize(NULL);
}
