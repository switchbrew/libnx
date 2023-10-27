#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <alloca.h>
#include <sys/iosupport.h>

#include <fcntl.h>
#include <poll.h>
#include <net/bpf.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include "result.h"
#include "services/bsd.h"
#include "services/ssl.h"
#include "services/nifm.h"
#include "runtime/devices/socket.h"
#include "runtime/hosversion.h"
#include "../alloc.h"

__attribute__((weak)) size_t __nx_pollfd_sb_max_fds = 64;

int _convert_errno(int bsdErrno);

static int _socketOpen(struct _reent *r, void *fdptr, const char *path, int flags, int mode);
static int _socketClose(struct _reent *r, void *fdptr);
static ssize_t _socketWrite(struct _reent *r, void *fdptr, const char *buf, size_t count);
static ssize_t _socketRead(struct _reent *r, void *fdptr, char *buf, size_t count);

static int _socketGetFd(int fd);

static const devoptab_t g_socketDevoptab = {
    .name = "soc",
    .structSize   = sizeof(int),
    .open_r       = _socketOpen,
    .close_r      = _socketClose,
    .write_r      = _socketWrite,
    .read_r       = _socketRead,
    .seek_r       = NULL,
    .fstat_r      = NULL,
    .stat_r       = NULL,
    .link_r       = NULL,
    .unlink_r     = NULL,
    .chdir_r      = NULL,
    .rename_r     = NULL,
    .mkdir_r      = NULL,
    .dirStateSize = 0,
    .diropen_r    = NULL,
    .dirreset_r   = NULL,
    .dirnext_r    = NULL,
    .dirclose_r   = NULL,
    .statvfs_r    = NULL,
    .ftruncate_r  = NULL,
    .fsync_r      = NULL,
    .deviceData   = 0,
    .chmod_r      = NULL,
    .fchmod_r     = NULL,
    .rmdir_r      = NULL,
};

static const SocketInitConfig g_defaultSocketInitConfig = {
    .tcp_tx_buf_size        = 0x8000,
    .tcp_rx_buf_size        = 0x10000,
    .tcp_tx_buf_max_size    = 0x40000,
    .tcp_rx_buf_max_size    = 0x40000,

    .udp_tx_buf_size = 0x2400,
    .udp_rx_buf_size = 0xA500,

    .sb_efficiency = 4,

    .num_bsd_sessions = 3,
    .bsd_service_type = BsdServiceType_User,
};

const SocketInitConfig *socketGetDefaultInitConfig(void) {
    return &g_defaultSocketInitConfig;
}

static u32 socketSelectVersion(void) {
    if (hosversionBefore(3,0,0)) {
        return 1;
    } else if (hosversionBefore(4,0,0)) {
        return 2;
    } else if (hosversionBefore(5,0,0)) {
        return 3;
    } else if (hosversionBefore(6,0,0)) {
        return 4;
    } else if (hosversionBefore(8,0,0)) {
        return 5;
    } else if (hosversionBefore(9,0,0)) {
        return 6;
    } else if (hosversionBefore(13,0,0)) {
        return 7;
    } else if (hosversionBefore(16,0,0)) {
        return 8;
    } else /* latest known version */ {
        return 9;
    }
}

Result socketInitialize(const SocketInitConfig *config) {
    Result ret = 0;
    if (!config)
        config = &g_defaultSocketInitConfig;

    BsdInitConfig bcfg = {
        .version = socketSelectVersion(),

        .tcp_tx_buf_size        = config->tcp_tx_buf_size,
        .tcp_rx_buf_size        = config->tcp_rx_buf_size,
        .tcp_tx_buf_max_size    = config->tcp_tx_buf_max_size,
        .tcp_rx_buf_max_size    = config->tcp_rx_buf_max_size,

        .udp_tx_buf_size = config->udp_tx_buf_size,
        .udp_rx_buf_size = config->udp_rx_buf_size,

        .sb_efficiency = config->sb_efficiency,
    };

    u32 num_bsd_sessions = config->num_bsd_sessions;
    u32 bsd_service_type = config->bsd_service_type;

    if (!num_bsd_sessions)
        num_bsd_sessions = g_defaultSocketInitConfig.num_bsd_sessions;
    if (!bsd_service_type)
        bsd_service_type = g_defaultSocketInitConfig.bsd_service_type;

    int dev = FindDevice("soc:");
    if(dev != -1)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    ret = bsdInitialize(&bcfg, num_bsd_sessions, bsd_service_type);
    if(R_SUCCEEDED(ret))
        dev = AddDevice(&g_socketDevoptab);
    else {
        socketExit();
        return ret;
    }

    if(dev == -1) {
        socketExit();
        return MAKERESULT(Module_Libnx, LibnxError_TooManyDevOpTabs);
    }
    else {
        g_bsdResult = 0;
        g_bsdErrno = 0;
    }

    return ret;
}

void socketExit(void) {
    RemoveDevice("soc:");
    bsdExit();
}

Result socketGetLastResult(void) {
    return g_bsdResult;
}

int socketSslConnectionSetSocketDescriptor(SslConnection *c, int sockfd) {
    int dev;
    int fd = _socketGetFd(sockfd);

    if (fd==-1)
        return -1;

    int tmpfd=0;
    Result rc = sslConnectionSetSocketDescriptor(c, fd, &tmpfd);
    if (R_FAILED(rc)) {
        g_bsdResult = rc;
        errno = EIO;
        return -1;
    }

    if (tmpfd==-1) { // The cmd didn't return a sockfd. This error must be ignored.
        errno = ENOENT;
        return -1;
    }

    dev = FindDevice("soc:");
    if(dev == -1)
        return -1;

    fd = __alloc_handle(dev);
    if(fd == -1)
        return -1;

    *(int *)__get_handle(fd)->fileStruct = tmpfd;

    return fd;
}

int socketSslConnectionGetSocketDescriptor(SslConnection *c) {
    int fd, dev;
    int tmpfd=0;

    Result rc = sslConnectionGetSocketDescriptor(c, &tmpfd);
    if (R_FAILED(rc)) {
        g_bsdResult = rc;
        errno = EIO;
        return -1;
    }

    dev = FindDevice("soc:");
    if(dev == -1)
        return -1;

    fd = __alloc_handle(dev);
    if(fd == -1)
        return -1;

    *(int *)__get_handle(fd)->fileStruct = tmpfd;

    return fd;
}

int socketSslConnectionSetDtlsSocketDescriptor(SslConnection *c, int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int dev;
    int fd = _socketGetFd(sockfd);

    if (fd==-1)
        return -1;

    int tmpfd=0;
    Result rc = sslConnectionSetDtlsSocketDescriptor(c, fd, addr, addrlen, &tmpfd);
    if (R_FAILED(rc)) {
        g_bsdResult = rc;
        errno = EIO;
        return -1;
    }

    if (tmpfd==-1) { // The cmd didn't return a sockfd. This error must be ignored.
        errno = ENOENT;
        return -1;
    }

    dev = FindDevice("soc:");
    if(dev == -1)
        return -1;

    fd = __alloc_handle(dev);
    if(fd == -1)
        return -1;

    *(int *)__get_handle(fd)->fileStruct = tmpfd;

    return fd;
}

int socketNifmRequestRegisterSocketDescriptor(NifmRequest* r, int sockfd) {
    int fd = _socketGetFd(sockfd);

    if (fd==-1)
        return -1;

    Result rc = nifmRequestRegisterSocketDescriptor(r, fd);
    if (R_FAILED(rc)) {
        g_bsdResult = rc;
        errno = EIO;
        return -1;
    }

    return 0;
}

int socketNifmRequestUnregisterSocketDescriptor(NifmRequest* r, int sockfd) {
    int fd = _socketGetFd(sockfd);

    if (fd==-1)
        return -1;

    Result rc = nifmRequestUnregisterSocketDescriptor(r, fd);
    if (R_FAILED(rc)) {
        g_bsdResult = rc;
        errno = EIO;
        return -1;
    }

    return 0;
}

/***********************************************************************************************************************/

static int _socketGetFd(int fd) {
    __handle *handle = __get_handle(fd);
    if(handle == NULL) {
        errno = EBADF;
        return -1;
    }
    if(strcmp(devoptab_list[handle->device]->name, "soc") != 0) {
        errno = ENOTSOCK;
        return -1;
    }
    return *(int *)handle->fileStruct;
}

static int _socketParseBsdResult(struct _reent *r, int ret) {
    int errno_;
    if(ret != -1)
        return ret; // Nothing to do
    else {
        if(g_bsdErrno == -1) {
            // We're using -1 to signal Switch error codes.
            // Note: all of the bsd:u/s handlers return 0.
            switch(g_bsdResult) {
                case 0xD201:
                    errno_ = ENFILE;
                    break;
                case 0xD401:
                    errno_ = EFAULT;
                    break;
                case 0x10601:
                    errno_ = EBUSY;
                    break;
                default:
                    errno_ = EPIPE;
                    break;
            }
        }
        else
            errno_ = _convert_errno(g_bsdErrno); /* Nintendo actually used the Linux errno definitions for their FreeBSD build :)
                                                    but we still need to convert to newlib errno */
    }

    if(r == NULL)
        errno = errno_;
    else
        r->_errno = errno_;

    return -1;
}

static int _socketOpen(struct _reent *r, void *fdptr, const char *path, int flags, int mode) {
    (void)mode;
    if(strncmp(path, "soc:", 4)==0) path+= 4;
    int ret = _socketParseBsdResult(r, bsdOpen(path, flags));
    if(ret == -1)
        return ret;

    *(int *)fdptr = ret;
    return 0;
}

static int _socketClose(struct _reent *r, void *fdptr) {
    int fd = *(int *)fdptr;
    return _socketParseBsdResult(r, bsdClose(fd));
}

static ssize_t _socketWrite(struct _reent *r, void *fdptr, const char *buf, size_t count) {
    int fd = *(int *)fdptr;
    ssize_t ret = bsdWrite(fd, buf, count);

    _socketParseBsdResult(r, (int)ret);
    return ret;
}

static ssize_t _socketRead(struct _reent *r, void *fdptr, char *buf, size_t count) {
    int fd = *(int *)fdptr;
    ssize_t ret = bsdRead(fd, buf, count);

    _socketParseBsdResult(r, (int)ret);
    return ret;
}

/*
    It is way too complicated and inefficient to use devoptab with bsdSelect.
    We're therefore implementing select with poll.

    Code copied from libctru.
*/
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    struct pollfd *pollinfo;
    nfds_t numfds = 0;
    size_t i, j;
    int rc, found;

    if(nfds >= FD_SETSIZE || nfds < 0) {
        errno = EINVAL;
        return -1;
    }

    for(i = 0; i < nfds; ++i) {
        if((readfds && FD_ISSET(i, readfds))
        || (writefds && FD_ISSET(i, writefds))
        || (exceptfds && FD_ISSET(i, exceptfds)))
            ++numfds;
    }

    if(numfds <= __nx_pollfd_sb_max_fds)
        pollinfo = (struct pollfd *)alloca(numfds * sizeof(struct pollfd));
    else
        pollinfo = (struct pollfd *)__libnx_alloc(numfds * sizeof(struct pollfd));
    if(pollinfo == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for(i = 0, j = 0; i < nfds; ++i) {
        if((readfds && FD_ISSET(i, readfds))
        || (writefds && FD_ISSET(i, writefds))
        || (exceptfds && FD_ISSET(i, exceptfds))) {
            pollinfo[j].fd      = _socketGetFd(i);
            if(pollinfo[j].fd == -1) {
                rc = -1;
                goto cleanup;
            }
            pollinfo[j].events  = 0;
            pollinfo[j].revents = 0;

            if(readfds && FD_ISSET(i, readfds))
                pollinfo[j].events |= POLLIN;
            if(writefds && FD_ISSET(i, writefds))
                pollinfo[j].events |= POLLOUT;

            ++j;
        }
    }

    if(timeout)
        rc = _socketParseBsdResult(NULL, bsdPoll(pollinfo, numfds, timeout->tv_sec*1000 + timeout->tv_usec/1000));
    else
        rc = _socketParseBsdResult(NULL, bsdPoll(pollinfo, numfds, -1));

    if(rc < 0)
        goto cleanup;

    for(i = 0, j = 0, rc = 0; i < nfds; ++i) {
        found = 0;

        if((readfds && FD_ISSET(i, readfds))
        || (writefds && FD_ISSET(i, writefds))
        || (exceptfds && FD_ISSET(i, exceptfds))) {

            if(readfds && FD_ISSET(i, readfds)) {
                if(pollinfo[j].revents & (POLLIN|POLLHUP))
                    found = 1;
                else
                    FD_CLR(i, readfds);
            }

            if(writefds && FD_ISSET(i, writefds)) {
                if(pollinfo[j].revents & (POLLOUT|POLLHUP))
                    found = 1;
                else
                    FD_CLR(i, writefds);
            }

            if(exceptfds && FD_ISSET(i, exceptfds)) {
                if(pollinfo[j].revents & POLLERR)
                    found = 1;
                else
                    FD_CLR(i, exceptfds);
            }

            if(found)
                ++rc;
            ++j;
        }
    }

cleanup:
    if(numfds > __nx_pollfd_sb_max_fds)
        __libnx_free(pollinfo);
    return rc;
}

// This is much saner than select.
int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    struct pollfd *fds2;
    int ret = 0;

    if(fds == NULL) {
        errno = EFAULT;
        return -1;
    }

    if(nfds <= __nx_pollfd_sb_max_fds)
        fds2 = (struct pollfd *)alloca(nfds * sizeof(struct pollfd));
    else
        fds2 = (struct pollfd *)__libnx_alloc(nfds * sizeof(struct pollfd));
    if(fds2 == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for(nfds_t i = 0; i < nfds; i++) {
        fds2[i].events = fds[i].events;
        fds2[i].revents = fds[i].revents;
        if(fds[i].fd < 0) {
            fds2[i].fd = -1;
        } else {
            fds2[i].fd = _socketGetFd(fds[i].fd);
            if(fds2[i].fd == -1) {
                ret = -1;
                break;
            }
        }
    }

    if(ret != -1)
        ret = _socketParseBsdResult(NULL, bsdPoll(fds2, nfds, timeout));
    if(ret != -1) {
        for(nfds_t i = 0; i < nfds; i++) {
            fds[i].events = fds2[i].events;
            fds[i].revents = fds2[i].revents;
        }
    }

    if(nfds > __nx_pollfd_sb_max_fds)
        __libnx_free(fds2);
    return ret;
}

int sysctl(const int *name, unsigned int namelen, void *oldp, size_t *oldlenp, const void *newp, size_t newlen) {
    return _socketParseBsdResult(NULL, bsdSysctl(name, namelen, oldp, oldlenp, newp, newlen));
}

int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, const void *newp, size_t newlen) {
    int mib[CTL_MAXNAME + 2]; // +2 because most of the relevant code I've seen uses +2 as well
    size_t miblen = CTL_MAXNAME + 2;

    if(sysctlnametomib(name, mib, &miblen) == -1)
        return -1;

    return sysctl(mib, miblen, oldp, oldlenp, newp, newlen);
}

int sysctlnametomib(const char *name, int *mibp, size_t *sizep) {
    int oid[2] = {0, 3}; // sysctl.name2oid
    int ret;
    size_t oldlenp;

    if(name == NULL || mibp == NULL || sizep == NULL) {
        errno = EFAULT;
        return -1;
    }

    oldlenp = 4 * (*sizep);
    ret = sysctl(oid, 2, mibp, &oldlenp, name, strlen(name));
    *sizep = oldlenp / 4;

    return ret;
}

int socket(int domain, int type, int protocol) {
    int ret, fd, dev;

    dev = FindDevice("soc:");
    if(dev == -1)
        return -1;

    fd = __alloc_handle(dev);
    if(fd == -1)
        return -1;

    ret = _socketParseBsdResult(NULL, bsdSocket(domain, type, protocol));
    if(ret == -1) {
        __release_handle(fd);
        return -1;
    }
    else {
        *(int *)__get_handle(fd)->fileStruct = ret;
        return fd;
    }
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    ssize_t ret;
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    ret = bsdRecv(sockfd, buf, len, flags);
    return _socketParseBsdResult(NULL, (int)ret);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    ssize_t ret;
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    ret = bsdRecvFrom(sockfd, buf, len, flags, src_addr, addrlen);
    return _socketParseBsdResult(NULL, (int)ret);
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    ssize_t ret;
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    ret = bsdSend(sockfd, buf, len, flags);
    return _socketParseBsdResult(NULL, (int)ret);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    ssize_t ret;
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    ret = bsdSendTo(sockfd, buf, len, flags, dest_addr, addrlen);
    return _socketParseBsdResult(NULL, (int)ret);
}

int accept(int sockfd, struct sockaddr *address, socklen_t *addrlen) {
    int ret, fd, dev;

    fd = _socketGetFd(sockfd);
    if(fd == -1)
        return -1;

    dev = __get_handle(sockfd)->device;
    sockfd = fd;

    fd = __alloc_handle(dev);
    if(fd == -1)
        return -1;

    ret = _socketParseBsdResult(NULL, bsdAccept(sockfd, address, addrlen));
    if(ret == -1) {
        __release_handle(fd);
        return -1;
    }
    else {
        *(int *)__get_handle(fd)->fileStruct = ret;
        return fd;
    }
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdBind(sockfd, addr, addrlen));
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdConnect(sockfd, addr, addrlen));
}

int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdGetPeerName(sockfd, addr, addrlen));
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdGetSockName(sockfd, addr, addrlen));
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdGetSockOpt(sockfd, level, optname, optval, optlen));
}

int listen(int sockfd, int backlog) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdListen(sockfd, backlog));
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdSetSockOpt(sockfd, level, optname, optval, optlen));
}

int ioctl(int fd, int request, ...) {
    void *data;
    va_list args;

    va_start(args, request);
    data = (request & IOC_INOUT) ? va_arg(args, void *) : NULL;
    va_end(args);

    if(data == NULL && (request & IOC_INOUT) && IOCPARM_LEN(request) != 0) {
        errno = EFAULT;
        return -1;
    }

    fd = request != FIONBIO ? _socketGetFd(fd) : fd;
    if(fd == -1)
        return -1;

    switch(request) {
        case FIONBIO: {
            // See note in fcntl (below)
            int flags = fcntl(fd, F_GETFL, 0);
            if(flags == -1)
                return -1;
            flags = *(int *)data != 0 ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
            return fcntl(fd, F_SETFL, flags);
        }
        case BIOCSETF:
        case BIOCSETWF:
        case BIOCSETFNR: {
            int ret;
            struct bpf_program *prog = (struct bpf_program *)data;
            if(prog->bf_len > BPF_MAXBUFSIZE) {
                errno = EINVAL;
                return -1;
            }

            struct bpf_program_serialized *prog_ser = (struct bpf_program_serialized *)__libnx_alloc(sizeof(struct bpf_program_serialized));
            if(prog_ser == NULL) {
                errno = ENOMEM;
                return -1;
            }

            prog_ser->bf_len = prog->bf_len;
            memcpy(prog_ser->bf_insns, prog->bf_insns, prog->bf_len);

            request = _IOC(request & IOC_DIRMASK, IOCGROUP(request), IOCBASECMD(request), sizeof(struct bpf_program_serialized));
            ret = bsdIoctl(fd, request, prog_ser);
            __libnx_free(prog_ser);
            return _socketParseBsdResult(NULL, ret);
        }
        default:
            return _socketParseBsdResult(NULL, bsdIoctl(fd, request, data));
    }
}

#define O_NONBLOCK_NX 0x800

#define ALL_NX   (O_NONBLOCK_NX)
#define ALL_FLAGS (O_NONBLOCK)

static int from_nx(int flags) {
    int newflags = 0;

    if(flags & O_NONBLOCK_NX)
        newflags |= O_NONBLOCK;
        /* add other flag translations here */

    return newflags;
}

static int to_nx(int flags) {
    int newflags = 0;

    if(flags & O_NONBLOCK)
        newflags |= O_NONBLOCK_NX;
    /* add other flag translations here */

    return newflags;
}


int fcntl(int fd, int cmd, ...) {
    va_list args;
    int flags=0;

    /*
        bsd:u/s only supports F_GETFL and F_SETFL with the O_NONBLOCK flag (or 0).
        F_GETFL is implemented using a custom, non-whitelisted IOCTL, whereas
        F_SETFL is implemented using FIONBIO.
    */
    if(cmd != F_GETFL && cmd != F_SETFL)
        return EOPNOTSUPP;

    if (cmd == F_SETFL) {

        va_start(args, cmd);
        flags = va_arg(args, int);
        va_end(args);

        if (flags & ~ALL_FLAGS) {
            errno = EINVAL;
            return -1;
        }
        flags = to_nx(flags);
    }

    fd = _socketGetFd(fd);
    if(fd == -1)
        return -1;

    flags = _socketParseBsdResult(NULL, bsdFcntl(fd, cmd, flags));

    if (flags & ~ALL_NX) {
        /* report unknown flags here */
    }
    return from_nx(flags);
}

int shutdown(int sockfd, int how) {
    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;
    return _socketParseBsdResult(NULL, bsdShutdown(sockfd, how));
}

int sockatmark(int sockfd) {
    int atmark;
    return ioctl(sockfd, SIOCATMARK, &atmark) == -1 ? -1 : atmark;
}

int socketpair(int domain, int type, int protocol, int sv[2]) {
    // Unimplementable, function definition written for compliance
    errno = ENOSYS;
    return -1;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    if(msg == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct mmsghdr msgvec = {
        .msg_hdr = *msg,
        .msg_len = 0,
    };

    ssize_t ret = recvmmsg(sockfd, &msgvec, 1, flags, NULL);
    if (ret >= 0) {
        *msg = msgvec.msg_hdr;
        ret = msgvec.msg_len;
    }

    return ret;
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    if(msg == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct mmsghdr msgvec = {
        .msg_hdr = *msg,
        .msg_len = 0,
    };

    ssize_t ret = sendmmsg(sockfd, &msgvec, 1, flags);
    if (ret > 0) ret = msgvec.msg_len;

    return ret;
}

static int _serializeMmsg(u8 *outbuf, size_t outbuf_size, struct mmsghdr *msgvec, unsigned int vlen, bool is_send) {
    void* tmp_ptr = NULL;
    u8 *dataptr = outbuf;
    *dataptr++ = 0x8;

    for (unsigned int i=0; i<vlen; i++) {
        socklen_t msg_controllen = msgvec[i].msg_hdr.msg_controllen;

        if (msg_controllen >= sizeof(struct cmsghdr)) {
            struct cmsghdr *cmsg = msgvec[i].msg_hdr.msg_control;
            if (cmsg) {
                if (cmsg->cmsg_level == 0xffff && cmsg->cmsg_type == 1) {
                    errno = EOPNOTSUPP;
                    return -1;
                }
            }
        }

        socklen_t msg_namelen = msgvec[i].msg_hdr.msg_namelen;
        *((socklen_t*)dataptr) = msg_namelen;
        dataptr+= sizeof(socklen_t);

        if (is_send && (tmp_ptr = msgvec[i].msg_hdr.msg_name)) memcpy(dataptr, tmp_ptr, msg_namelen);
        dataptr+= msg_namelen;

        int msg_iovlen = msgvec[i].msg_hdr.msg_iovlen;
        *((int*)dataptr) = msg_iovlen;
        dataptr+= sizeof(int);

        for (int veci=0; veci<msg_iovlen; veci++) {
            struct iovec *vec = &msgvec[i].msg_hdr.msg_iov[veci];

            u64 iov_len = vec->iov_len;
            *((u64*)dataptr) = iov_len;
            dataptr+= sizeof(u64);

            if (is_send) memcpy(dataptr, vec->iov_base, iov_len);
            dataptr+= iov_len;
        }

        *((socklen_t*)dataptr) = msg_controllen;
        dataptr+= sizeof(socklen_t);

        if (is_send && (tmp_ptr = msgvec[i].msg_hdr.msg_control)) memcpy(dataptr, tmp_ptr, msg_controllen);
        dataptr+= msg_controllen;

        *((int*)dataptr) = msgvec[i].msg_hdr.msg_flags;
        dataptr+= sizeof(int);

        *((int*)dataptr) = msgvec[i].msg_len;
        dataptr+= sizeof(int);
    }

    // sdknso would verify that dataptr isn't larger than outbuf+outbuf_size (Abort otherwise), but that can't happen anyway since the caller allocates enough space.
    return (uintptr_t)dataptr-(uintptr_t)outbuf;
}

static int _deserializeMmsg(struct mmsghdr *msgvec, unsigned int vlen, u8 *inbuf, size_t inbuf_size, bool is_recv) {
    bool bounds_flag=0;
    void* tmp_ptr = NULL;
    u8 *dataptr = &inbuf[0x1];
    uintptr_t inbuf_end = (uintptr_t)&inbuf[inbuf_size];

    // sdknso verifies that dataptr isn't larger than outbuf+outbuf_size at the end prior to returning (Abort otherwise). We'll also verify it during the loop, and also verify that sizes from the buffer are not larger than the original msgvec values.

    for (unsigned int i=0; i<vlen; i++) {
        if ((uintptr_t)dataptr > (uintptr_t)inbuf_end || (uintptr_t)dataptr+sizeof(socklen_t) > inbuf_end) {
            bounds_flag = 1;
            break;
        }

        socklen_t msg_namelen = *((socklen_t*)dataptr);
        dataptr+= sizeof(socklen_t);

        if ((uintptr_t)dataptr+msg_namelen > inbuf_end || msg_namelen > msgvec[i].msg_hdr.msg_namelen) {
            bounds_flag = 1;
            break;
        }
        msgvec[i].msg_hdr.msg_namelen = msg_namelen;

        if (is_recv && (tmp_ptr = msgvec[i].msg_hdr.msg_name)) {
            memcpy(tmp_ptr, dataptr, msg_namelen);
        }
        dataptr+= msg_namelen;

        if ((uintptr_t)dataptr+sizeof(int) > inbuf_end) {
            bounds_flag = 1;
            break;
        }

        int msg_iovlen = *((int*)dataptr);
        dataptr+= sizeof(int);

        if (msg_iovlen > msgvec[i].msg_hdr.msg_iovlen) {
            bounds_flag = 1;
            break;
        }

        msgvec[i].msg_hdr.msg_iovlen = msg_iovlen;

        for (int veci=0; veci<msg_iovlen; veci++) {
            struct iovec *vec = &msgvec[i].msg_hdr.msg_iov[veci];

            if ((uintptr_t)dataptr+sizeof(u64) > inbuf_end) {
                bounds_flag = 1;
                break;
            }

            u64 iov_len = *((u64*)dataptr);
            dataptr+= sizeof(u64);
            if (iov_len > inbuf_size || (uintptr_t)dataptr+iov_len > inbuf_end || iov_len > vec->iov_len) {
                bounds_flag = 1;
                break;
            }

            vec->iov_len = iov_len;
            if (is_recv) memcpy(vec->iov_base, dataptr, iov_len);
            dataptr+= iov_len;
        }

        if (bounds_flag) break;

        if ((uintptr_t)dataptr+sizeof(socklen_t) > inbuf_end) {
            bounds_flag = 1;
            break;
        }

        socklen_t msg_controllen = *((socklen_t*)dataptr);
        dataptr+= sizeof(socklen_t);

        if ((uintptr_t)dataptr+msg_controllen > inbuf_end || msg_controllen > msgvec[i].msg_hdr.msg_controllen) {
            bounds_flag = 1;
            break;
        }

        msgvec[i].msg_hdr.msg_controllen = msg_controllen;

        if (is_recv && (tmp_ptr = msgvec[i].msg_hdr.msg_control)) {
            memcpy(tmp_ptr, dataptr, msg_controllen);
        }
        dataptr+= msg_controllen;

        if ((uintptr_t)dataptr+sizeof(int) > inbuf_end) {
            bounds_flag = 1;
            break;
        }

        msgvec[i].msg_hdr.msg_flags = *((int*)dataptr);
        dataptr+= sizeof(int);

        if ((uintptr_t)dataptr+sizeof(int) > inbuf_end) {
            bounds_flag = 1;
            break;
        }

        msgvec[i].msg_len = *((int*)dataptr);
        dataptr+= sizeof(int);

        if (msg_controllen >= sizeof(struct cmsghdr)) {
            struct cmsghdr *cmsg = msgvec[i].msg_hdr.msg_control;
            if (cmsg) {
                if (cmsg->cmsg_level == 0xffff && cmsg->cmsg_type == 1) {
                    errno = EOPNOTSUPP;
                    return -1;
                }
            }
        }
    }

    if (bounds_flag || (uintptr_t)dataptr > inbuf_end) {
        errno = EFAULT;
        return -1;
    }

    return (uintptr_t)dataptr-(uintptr_t)inbuf;
}

static int _mmsgInitCommon(u8 **buf, size_t *alignsize, struct mmsghdr *msgvec, unsigned int vlen) {
    size_t msgdatasize_total = 0;
    size_t bufsize = 1;

    for (unsigned int i=0; i<vlen; i++) {
        bufsize+= sizeof(socklen_t) + msgvec[i].msg_hdr.msg_namelen + sizeof(int);

        int msg_iovlen = msgvec[i].msg_hdr.msg_iovlen;

        for (int veci=0; veci<msg_iovlen; veci++) {
            size_t iov_len = msgvec[i].msg_hdr.msg_iov[veci].iov_len;
            bufsize += sizeof(u64) + iov_len;
            msgdatasize_total += iov_len;
        }

        bufsize+= sizeof(socklen_t) + msgvec[i].msg_hdr.msg_controllen + sizeof(int) + sizeof(int);
    }

    if (msgdatasize_total > 0x80000) {
        errno = EMSGSIZE;
        return -1;
    }

    *alignsize = (bufsize+0xfff) & ~0xfff;
    *buf = (u8*)__libnx_aligned_alloc(0x1000, *alignsize);
    if (*buf == NULL) {
        errno = ENOMEM;
        return -1;
    }
    memset(*buf, 0, *alignsize);

    return 0;
}

int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags) {
    if (hosversionBefore(7,0,0)) { // This cmd was added with [3.0.0+], but we'll only support the updated [7.0.0+] version of it.
        errno = ENOSYS;
        return -1;
    }

    if(msgvec == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(vlen < 1 || vlen > 0x20) {
        errno = EMSGSIZE;
        return -1;
    }

    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;

    int ret=0, ret2=0;
    u8 *buf = NULL;
    size_t alignsize=0;

    ret = _mmsgInitCommon(&buf, &alignsize, msgvec, vlen);
    if (ret==-1) return ret;

    ret = _serializeMmsg(buf, alignsize, msgvec, vlen, 1);

    if (ret>=0) ret = _socketParseBsdResult(NULL, bsdSendMMsg(sockfd, buf, alignsize, vlen, flags));

    if (ret>=0 && ret>vlen) { // sdknso doesn't check this, but we will.
        errno = EFAULT;
        ret = -1;
    }

    if (ret>=0) {
        ret2 = _deserializeMmsg(msgvec, ret, buf, alignsize, 0);
        if (ret2==-1) ret = ret2;
    }

    __libnx_free(buf);

    return ret;
}

int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout) {
    if (hosversionBefore(7,0,0)) { // This cmd was added with [3.0.0+], but we'll only support the updated [7.0.0+] version of it.
        errno = ENOSYS;
        return -1;
    }

    if(msgvec == NULL || (vlen < 1 || vlen > 0x20)) {
        errno = EINVAL;
        return -1;
    }

    sockfd = _socketGetFd(sockfd);
    if(sockfd == -1)
        return -1;

    int ret=0, ret2=0;
    u8 *buf = NULL;
    size_t alignsize=0;

    ret = _mmsgInitCommon(&buf, &alignsize, msgvec, vlen);
    if (ret==-1) return ret;

    struct timespec tmp_timeout={0};
    if (timeout) tmp_timeout = *timeout;

    ret = _serializeMmsg(buf, alignsize, msgvec, vlen, 0);

    if (ret>=0) ret = _socketParseBsdResult(NULL, bsdRecvMMsg(sockfd, buf, alignsize, vlen, flags, &tmp_timeout));

    if (ret>=0 && ret>vlen) { // sdknso doesn't check this, but we will.
        errno = EFAULT;
        ret = -1;
    }

    if (ret>=0) {
        ret2 = _deserializeMmsg(msgvec, ret, buf, alignsize, 1);
        if (ret2==-1) ret = ret2;
    }

    __libnx_free(buf);

    return ret;
}
