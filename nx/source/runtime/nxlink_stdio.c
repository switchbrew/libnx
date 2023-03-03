#include "runtime/nxlink.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

static int sock = -1;

int nxlinkConnectToHost(bool redirStdout, bool redirStderr)
{
    if (!__nxlink_host.s_addr) {
        errno = ENETUNREACH;
        return -1;
    }

    struct sockaddr_in srv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    // set to non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        close(sock);
        return -1;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != 0) {
        close(sock);
        return -1;
    }

    bzero(&srv_addr, sizeof srv_addr);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr = __nxlink_host;
    srv_addr.sin_port = htons(NXLINK_CLIENT_PORT);

    int ret = connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    if (ret != 0 && errno != EINPROGRESS) {
        int err = errno;
        close(sock);
        close(err);
        return -1;
    }

    if (ret != 0) { // EINPROGRESS
        struct pollfd pfd;

        pfd.fd      = sock;
        pfd.events  = POLLOUT;
        pfd.revents = 0;

        int n = poll(&pfd, 1, 1000); // only wait up to 1s to connect
        if (n < 0) {
            close(sock);
            return -1;
        }

        if (n == 0 || !(pfd.revents & POLLOUT)) {
            close(sock);
            errno = ETIMEDOUT;
            return -1;
        }
    }

    // reset back to blocking
    if (fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) != 0) {
        close(sock);
        return -1;
    }

    if (redirStdout) {
        // redirect stdout
        fflush(stdout);
        dup2(sock, STDOUT_FILENO);
    }

    if (redirStderr) {
        // redirect stderr
        fflush(stderr);
        dup2(sock, STDERR_FILENO);
    }

    return sock;
}
