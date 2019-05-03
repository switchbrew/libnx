/**
 * @file nxlink.h
 * @brief Netloader (nxlink) utilities
 * @author WinterMute
 * @copyright libnx Authors
 */
#pragma once

struct in_addr;

/// Address of the host connected through nxlink
extern struct in_addr __nxlink_host;

#define NXLINK_SERVER_PORT 28280 ///< nxlink TCP server port
#define NXLINK_CLIENT_PORT 28771 ///< nxlink TCP client port

/**
 * @brief Sets up stdout/stderr redirection to the nxlink host.
 * @return Socket fd on success, negative number on failure.
 * @note The socket should be closed with close() during application cleanup.
 */
int nxlinkStdio(void);
