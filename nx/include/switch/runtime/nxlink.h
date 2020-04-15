/**
 * @file nxlink.h
 * @brief Netloader (nxlink) utilities
 * @author WinterMute
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

struct in_addr;

/// Address of the host connected through nxlink
extern struct in_addr __nxlink_host;

#define NXLINK_SERVER_PORT 28280 ///< nxlink TCP server port
#define NXLINK_CLIENT_PORT 28771 ///< nxlink TCP client port

/**
 * @brief Connects to the nxlink host, setting up an output stream.
 * @param[in] redirStdout Whether to redirect stdout to nxlink output.
 * @param[in] redirStderr Whether to redirect stderr to nxlink output.
 * @return Socket fd on success, negative number on failure.
 * @note The socket should be closed with close() during application cleanup.
 */
int nxlinkConnectToHost(bool redirStdout, bool redirStderr);

/// Same as \ref nxlinkConnectToHost but redirecting both stdout/stderr.
NX_INLINE int nxlinkStdio(void) {
    return nxlinkConnectToHost(true, true);
}

/// Same as \ref nxlinkConnectToHost but redirecting only stderr.
NX_INLINE int nxlinkStdioForDebug(void) {
    return nxlinkConnectToHost(false, true);
}
