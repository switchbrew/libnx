// FreeBSD's header was so full of s*it, I just picked libctru's version of <arpa/inet.h>

#pragma once

#include <netinet/in.h>
#include <stdint.h>

static inline uint32_t htonl(uint32_t hostlong)
{
    return __builtin_bswap32(hostlong);
}

static inline uint16_t htons(uint16_t hostshort)
{
    return __builtin_bswap16(hostshort);
}

static inline uint32_t ntohl(uint32_t netlong)
{
    return __builtin_bswap32(netlong);
}

static inline uint16_t ntohs(uint16_t netshort)
{
    return __builtin_bswap16(netshort);
}

#ifdef __cplusplus
extern "C" {
#endif

    in_addr_t inet_addr(const char *cp);
    int       inet_aton(const char *cp, struct in_addr *inp);
    char*     inet_ntoa(struct in_addr in);

    const char *inet_ntop(int af, const void * src, char * dst, socklen_t size);
    int        inet_pton(int af, const char * src, void * dst);

#ifdef __cplusplus
}
#endif
