Result bsdInitialize(TransferMemory* tmem);
int bsdGetErrno();
int bsdConnect(int sockfd, void* addr, u32 addrlen);
int bsdSocket(int domain, int type, int protocol);
int bsdBind(int sockfd, void* addr, u32 addrlen);
int bsdListen(int sockfd, int backlog);

#define BSD_AF_INET 2
#define BSD_AF_INET6 10

#define BSD_SOCK_STREAM 1
#define BSD_SOCK_DGRAM 2

struct bsd_sockaddr_in {
    u8  sin_len;
    u8  sin_family;
    u16 sin_port;
    u32 sin_addr;
    u8  sin_zero[8];
};
