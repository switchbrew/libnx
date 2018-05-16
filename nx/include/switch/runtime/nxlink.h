#pragma once

struct in_addr;

extern struct in_addr __nxlink_host;

#define NXLINK_SERVER_PORT 28280
#define NXLINK_CLIENT_PORT 28771

int nxlinkStdio(void);
