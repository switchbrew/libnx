Result bsdInitialize(TransferMemory* tmem);
int bsdGetErrno();
int bsdConnect(int sockfd, void* addr, u32 addrlen);
int bsdSocket(int domain, int type, int protocol);
int bsdBind(int sockfd, void* addr, u32 addrlen);
int bsdListen(int sockfd, int backlog);
int bsdSend(int sockfd, void* buffer, size_t length, int flags);
int bsdRecv(int sockfd, void* buffer, size_t length, int flags);
int bsdWrite(int sockfd, void* buffer, size_t length);

#define BSD_AF_INET 2
#define BSD_AF_INET6 10

#define BSD_SOCK_STREAM 1
#define BSD_SOCK_DGRAM 2

#define BSD_MSG_RECV_ALL 0x40

struct bsd_sockaddr_in {
    u8  sin_len;
    u8  sin_family;
    u16 sin_port;
    u32 sin_addr;
    u8  sin_zero[8];
};
