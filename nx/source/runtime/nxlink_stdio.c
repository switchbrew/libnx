#include "runtime/nxlink.h"

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>

static int sock = -1;

int nxlinkStdio(void)
{
    int ret = -1;
    struct sockaddr_in srv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock) {
        return ret;
    }

    bzero(&srv_addr, sizeof srv_addr);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr = __nxlink_host;
    srv_addr.sin_port = htons(NXLINK_CLIENT_PORT);

    ret = connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    if (ret != 0) {
        close(sock);
        return -1;
    }

    // redirect stdout
    fflush(stdout);
    dup2(sock, STDOUT_FILENO);
    // redirect stderr
    fflush(stderr);
    dup2(sock, STDERR_FILENO);

    return sock;
}
