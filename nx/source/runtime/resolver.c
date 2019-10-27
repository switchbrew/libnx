#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "result.h"
//#include "kernel/random.h"
#include "services/sfdnsres.h"
#include "services/nifm.h"
#include "runtime/hosversion.h"
#include "runtime/resolver.h"

__thread int h_errno;

static __thread Result g_resolverResult;
static __thread u32 g_resolverCancelHandle;             // ResolverOptionKey::RequestCancelHandleInteger
static __thread bool g_resolverDisableServiceDiscovery; // ResolverOptionKey::RequestEnableServiceDiscoveryBoolean (inverted)
static __thread bool g_resolverDisableDnsCache;         // ResolverOptionKey::RequestEnableDnsCacheBoolean (inverted)

static size_t g_resolverHostByNameBufferSize    = 0x200;  // ResolverOptionLocalKey::GetHostByNameBufferSizeUnsigned64
static size_t g_resolverHostByAddrBufferSize    = 0x200;  // ResolverOptionLocalKey::GetHostByAddrBufferSizeUnsigned64
static size_t g_resolverAddrInfoBufferSize      = 0x1000; // ResolverOptionLocalKey::GetAddrInfoBufferSizeUnsigned64
static size_t g_resolverAddrInfoHintsBufferSize = 0x400;  // ResolverOptionLocalKey::GetAddrInfoHintsBufferSizeUnsigned64

Result resolverGetLastResult(void) {
    return g_resolverResult;
}

u32 resolverGetCancelHandle(void) {
    // ResolverOptionKey::GetCancelHandleInteger on 5.0.0+
    /* Below code should be used instead of invoking sfdnsresResolverGetOptionRequest
    while (g_resolverCancelHandle == 0)
        randomGet(&g_resolverCancelHandle, sizeof(u32));
    */
    if (g_resolverCancelHandle == 0)
        sfdnsresGetCancelHandleRequest(&g_resolverCancelHandle);
    return g_resolverCancelHandle;
}

bool resolverGetEnableServiceDiscovery(void) {
    return !g_resolverDisableServiceDiscovery;
}

bool resolverGetEnableDnsCache(void) {
    return !g_resolverDisableDnsCache;
}

void resolverSetEnableServiceDiscovery(bool enable) {
    g_resolverDisableServiceDiscovery = !enable;
}

void resolverSetEnableDnsCache(bool enable) {
    g_resolverDisableDnsCache = !enable;
}

Result resolverCancel(u32 handle) {
    // ResolverOptionKey::SetCancelHandleInteger on [5.0.0+]
    return sfdnsresCancelRequest(handle);
}

Result resolverRemoveHostnameFromCache(const char* hostname) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return MAKERESULT(Module_Libnx, LibnxError_NotInitialized); // not implemented
}

Result resolverRemoveIpAddressFromCache(u32 ip) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return MAKERESULT(Module_Libnx, LibnxError_NotInitialized); // not implemented
}

static struct hostent *_resolverDeserializeHostent(int *err, const void *out_he_serialized) {
    const char *buf = (const char *)out_he_serialized;
    const char *pos, *pos_aliases, *pos_addresses;
    size_t name_size, total_aliases_size = 0;
    size_t nb_addresses;
    size_t nb_aliases = 0;
    size_t nb_pos;
    size_t len;

    int addrtype, addrlen;
    struct hostent *he;

    // Calculate the size of the buffer to allocate
    pos = buf;
    name_size = strlen(pos) + 1;
    pos += name_size;

    nb_aliases = ntohl(*(const u32 *)pos);
    pos += 4;

    pos_aliases = pos;

    if(nb_aliases>0) {
        for(nb_pos=0, len = 1; nb_pos<nb_aliases; nb_pos++, pos += len + 1) {
            len = strlen(pos);
        }
    }

    total_aliases_size = pos - pos_aliases;

    // Nintendo uses unsigned short here...
    addrtype = htons(*(const u16 *)pos);
    pos += 2;
    addrlen = htons(*(const u16 *)pos);
    pos += 2;

    // sfdnsres will only return IPv4 addresses for the "host" commands
    if(addrtype != AF_INET || addrlen != sizeof(struct in_addr)) {
        *err = NO_ADDRESS;
        return NULL;
    }

    // The official hostent (de)serializer doesn't support IPv6, at least not currently.
    nb_addresses = ntohl(*(const u32 *)pos);
    pos += 4;

    pos_addresses = pos;
    pos += addrlen * nb_addresses;

    he = (struct hostent *)malloc(
        sizeof(struct hostent)
        + name_size
        + 8 * (nb_aliases + 1 + nb_addresses + 1)
        + total_aliases_size
        + addrlen * nb_addresses
    );

    if(he == NULL) {
        *err = NO_RECOVERY;
        return NULL;
    }

    if (name_size == 1) {
        he->h_name = NULL;
        he->h_aliases = (char**)((char*)he + sizeof(struct hostent));
    }
    else {
        he->h_name = (char*)he + sizeof(struct hostent);
        memcpy(he->h_name, buf, name_size);
        he->h_aliases = (char **)(he->h_name + name_size);
    }

    he->h_addrtype = addrtype;
    he->h_length = addrlen;
    he->h_addr_list = he->h_aliases + nb_aliases + 1;

    if(nb_aliases>0) {
        char *alias = (char *)(he->h_addr_list + nb_addresses + 1);
        memcpy(alias, pos_aliases, total_aliases_size);
        for(size_t i = 0; i < nb_aliases; i++) {
            he->h_aliases[i] = alias;
            alias += strlen(alias) + 1;
        }
    }
    he->h_aliases[nb_aliases] = NULL;

    if(nb_addresses>0) {
        struct in_addr *addresses = (struct in_addr *)(he->h_addr_list + nb_addresses + 1 + total_aliases_size);
        memcpy(addresses, pos_addresses, addrlen * nb_addresses);
        for(size_t i = 0; i < nb_addresses; i++) {
            he->h_addr_list[i] = (char *)&addresses[i];
            addresses[i].s_addr = ntohl(addresses[i].s_addr); // lol Nintendo
        }
    }
    he->h_addr_list[nb_addresses] = NULL;

    return he;
}

struct addrinfo_serialized_hdr {
    u32 magic;
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    u32 ai_addrlen;
};

static size_t _resolverSerializeAddrInfo(struct addrinfo_serialized_hdr *hdr, const struct addrinfo *ai) {
    size_t subsize1 = (ai->ai_addr == NULL || ai->ai_addrlen == 0) ? 4 : ai->ai_addrlen; // not posix-compliant ?
    size_t subsize2 = ai->ai_canonname == NULL ? 1 : (strlen(ai->ai_canonname) + 1);

    hdr->magic = htonl(0xBEEFCAFE); // Seriously.
    hdr->ai_flags = htonl(ai->ai_flags);
    hdr->ai_family = htonl(ai->ai_family);
    hdr->ai_socktype = htonl(ai->ai_socktype);
    hdr->ai_protocol = htonl(ai->ai_protocol);
    hdr->ai_addrlen = ai->ai_addr == NULL ? 0 : htonl((u32)ai->ai_addrlen);

    if(hdr->ai_addrlen == 0)
        *(u32 *)((u8 *)hdr + sizeof(struct addrinfo_serialized_hdr)) = 0;
    else {
        // Nintendo just byteswaps everything recursively... even fields that are already byteswapped.
        switch(ai->ai_family) {
            case AF_INET: {
                struct sockaddr_in sa = {0};
                memcpy(&sa, ai->ai_addr, subsize1 <= sizeof(struct sockaddr_in) ? subsize1 : sizeof(struct sockaddr_in));
                sa.sin_port = htons(sa.sin_port);
                sa.sin_addr.s_addr = htonl(sa.sin_addr.s_addr);
                memcpy((u8 *)hdr + sizeof(struct addrinfo_serialized_hdr), &sa, sizeof(struct sockaddr_in));
                break;
            }
            case AF_INET6: {
                struct sockaddr_in6 sa6 = {0};
                memcpy(&sa6, ai->ai_addr, subsize1 <= sizeof(struct sockaddr_in6) ? subsize1 : sizeof(struct sockaddr_in6));
                sa6.sin6_port = htons(sa6.sin6_port);
                sa6.sin6_flowinfo = htonl(sa6.sin6_flowinfo);
                sa6.sin6_scope_id = htonl(sa6.sin6_scope_id);
                memcpy((u8 *)hdr + sizeof(struct addrinfo_serialized_hdr), &sa6, sizeof(struct sockaddr_in6));
                break;
            }
            default:
                memcpy((u8 *)hdr + sizeof(struct addrinfo_serialized_hdr), ai->ai_addr, subsize1);
        }
    }

    if(ai->ai_canonname == NULL)
        *((u8 *)hdr + sizeof(struct addrinfo_serialized_hdr) + subsize1) = 0;
    else
        memcpy((u8 *)hdr + sizeof(struct addrinfo_serialized_hdr) + subsize1, ai->ai_canonname, subsize2);

    return sizeof(struct addrinfo_serialized_hdr) + subsize1 + subsize2;
}

static struct addrinfo_serialized_hdr *_resolverSerializeAddrInfoList(size_t *out_size, const struct addrinfo *ai) {
    size_t total_addrlen = 0, total_namelen = 0, n = 0;
    struct addrinfo_serialized_hdr *out, *pos;

    for(const struct addrinfo *node = ai; node != NULL; node = node->ai_next) {
        total_addrlen += node->ai_addrlen == 0 ? 4 : node->ai_addrlen;
        total_namelen += node->ai_canonname == NULL ? 1 : (strlen(node->ai_canonname) + 1);
        n++;
    }

    out = (struct addrinfo_serialized_hdr *)malloc(sizeof(struct addrinfo_serialized_hdr) * n + total_addrlen + total_namelen + 4);
    if(out == NULL)
        return NULL;

    pos = out;
    for(const struct addrinfo *node = ai; node != NULL; node = node->ai_next) {
        size_t len = _resolverSerializeAddrInfo(pos, node);
        pos = (struct addrinfo_serialized_hdr *)((u8 *)pos + len);
    }
    *(u32 *)pos = 0; // Sentinel value

    *out_size = (u8 *)pos - (u8 *)out + 4;
    return out;
}

static struct addrinfo *_resolverDeserializeAddrInfo(size_t *out_len, const struct addrinfo_serialized_hdr *hdr) {
    struct addrinfo_node {
        struct addrinfo info;
        struct sockaddr_storage addr;
        char canonname[];
    };

    size_t subsize1 = hdr->ai_addrlen == 0 ? 4 : ntohl(hdr->ai_addrlen);
    size_t subsize2 = strlen((const char *)hdr + sizeof(struct addrinfo_serialized_hdr) + subsize1) + 1;
    struct addrinfo_node *node = (struct addrinfo_node *)malloc(sizeof(struct addrinfo_node) + subsize2);

    *out_len = sizeof(struct addrinfo_serialized_hdr) + subsize1 + subsize2;
    if(node == NULL)
        return NULL;

    node->info.ai_flags = ntohl(hdr->ai_flags);
    node->info.ai_family = ntohl(hdr->ai_family);
    node->info.ai_socktype = ntohl(hdr->ai_socktype);
    node->info.ai_protocol = ntohl(hdr->ai_protocol);
    node->info.ai_addrlen = ntohl(hdr->ai_addrlen);

    // getaddrinfo enforces addrlen = sizeof(struct sockaddr) and family = AF_INET, ie. only IPv4, anyways...
    if(node->info.ai_addrlen > sizeof(struct sockaddr_storage))
        node->info.ai_addrlen = sizeof(struct sockaddr_storage);

    if(node->info.ai_addrlen == 0)
        node->info.ai_addr = NULL;
    else {
        node->info.ai_addr = (struct sockaddr *)&node->addr;
        memcpy(node->info.ai_addr, (const u8 *)hdr + sizeof(struct addrinfo_serialized_hdr), node->info.ai_addrlen);
        // Nintendo just byteswaps everything recursively... even fields that are already byteswapped.
        switch(node->info.ai_family) {
            case AF_INET: {
                struct sockaddr_in *sa = (struct sockaddr_in *)node->info.ai_addr;
                sa->sin_len = 6;
                sa->sin_port = ntohs(sa->sin_port);
                sa->sin_addr.s_addr = ntohl(sa->sin_addr.s_addr);
                break;
            }
            case AF_INET6: {
                struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)node->info.ai_addr;
                sa6->sin6_port = ntohs(sa6->sin6_port);
                sa6->sin6_flowinfo = ntohl(sa6->sin6_flowinfo);
                sa6->sin6_scope_id = ntohl(sa6->sin6_scope_id);
                break;
            }
            default:
                break;
        }
    }

    if(subsize2 == 1)
        node->info.ai_canonname = NULL;
    else {
        node->info.ai_canonname = node->canonname;
        memcpy(node->info.ai_canonname, (const u8 *)hdr + sizeof(struct addrinfo_serialized_hdr) + subsize1, subsize2);
    }

    node->info.ai_next = NULL;

    return &node->info;
}

static struct addrinfo *_resolverDeserializeAddrInfoList(struct addrinfo_serialized_hdr *hdr) {
    struct addrinfo *first = NULL, *prev = NULL;

    while(hdr->magic == htonl(0xBEEFCAFE)) {
        size_t len;
        struct addrinfo *node = _resolverDeserializeAddrInfo(&len, hdr);
        if(node == NULL) {
            if(first != NULL)
                freeaddrinfo(first);
            return NULL;
        }

        if(first == NULL)
            first = node;

        if(prev != NULL)
            prev->ai_next = node;

        prev = node;
        hdr = (struct addrinfo_serialized_hdr *)((u8 *)hdr + len);
    }

    return first;
}

void freehostent(struct hostent *he) {
    free(he);
}

void freeaddrinfo(struct addrinfo *ai) {
    if(ai == NULL) return; // not specified by POSIX, but that's convenient (FreeBSD does this too, etc.).
    for(struct addrinfo *node = ai, *next; node != NULL; node = next) {
        next = node->ai_next;
        free(node);
    }
}

struct hostent *gethostbyname(const char *name) {
    Result rc = 0;
    void *out_he_serialized = malloc(g_resolverHostByNameBufferSize);
    struct hostent *he = NULL;
    u32 ret = 0;
    u32 errno_ = 0;

    if(out_he_serialized == NULL) {
        h_errno = NO_RECOVERY;
        errno = ENOMEM; // POSIX leaves this unspecified
        goto cleanup;
    }
    rc = sfdnsresGetHostByNameRequest(
        g_resolverCancelHandle,
        !g_resolverDisableServiceDiscovery,
        name,
        &ret,
        &errno_,
        out_he_serialized, g_resolverHostByNameBufferSize,
        NULL);
    g_resolverCancelHandle = 0;

    if(rc == 0xD401) {
        h_errno = NO_RECOVERY;
        errno = EFAULT; // POSIX leaves this unspecified
        goto cleanup;
    }
    else if(R_FAILED(rc) && R_MODULE(rc) == 1) { // Kernel
        h_errno = TRY_AGAIN;
        errno = EAGAIN; // POSIX leaves this unspecified
        goto cleanup;
    }
    else if(R_FAILED(rc)) {
        h_errno = NO_RECOVERY;
        errno = EINVAL; // POSIX leaves this unspecified
        goto cleanup;
    }
    if(ret != NETDB_SUCCESS) {
        h_errno = ret;
        errno = errno_; // POSIX leaves this unspecified
        goto cleanup;
    }

    he = _resolverDeserializeHostent(&h_errno, out_he_serialized);
    if(he == NULL) {
        h_errno = NO_RECOVERY;
        errno = ENOMEM; // POSIX leaves this unspecified
    }
cleanup:
    g_resolverResult = rc;
    free(out_he_serialized);
    return he;
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type) {
    Result rc = 0;
    void *out_he_serialized = malloc(g_resolverHostByAddrBufferSize);
    struct hostent *he = NULL;
    u32 ret = 0;
    u32 errno_ = 0;

    if(out_he_serialized == NULL) {
        h_errno = NO_RECOVERY;
        errno = ENOMEM; // POSIX leaves this unspecified
        goto cleanup;
    }
    rc = sfdnsresGetHostByAddrRequest(
        addr, len,
        type,
        g_resolverCancelHandle,
        &ret,
        &errno_,
        out_he_serialized, g_resolverHostByAddrBufferSize,
        NULL);
    g_resolverCancelHandle = 0;

    if(rc == 0xD401) {
        h_errno = NO_RECOVERY; // POSIX leaves this unspecified
        errno = EFAULT;
        goto cleanup;
    }
    else if(R_FAILED(rc) && R_MODULE(rc) == 1) { // Kernel
        h_errno = TRY_AGAIN;
        errno = EAGAIN; // POSIX leaves this unspecified
        goto cleanup;
    }
    else if(R_FAILED(rc)) {
        h_errno = NO_RECOVERY;
        errno = EINVAL; // POSIX leaves this unspecified
        goto cleanup;
    }
    if(ret != NETDB_SUCCESS) {
        h_errno = ret;
        errno = errno_; // POSIX leaves this unspecified
        goto cleanup;
    }

    he = _resolverDeserializeHostent(&h_errno, out_he_serialized);
    if(he == NULL) {
        h_errno = NO_RECOVERY;
        errno = ENOMEM; // POSIX leaves this unspecified
    }
cleanup:
    g_resolverResult = rc;
    free(out_he_serialized);
    return he;
}

const char *hstrerror(int err) {
    static __thread char buf[0x80]; // ResolverOptionLocalKey::GetHostErrorStringBufferSizeUnsigned64
    Result rc = sfdnsresGetHostStringErrorRequest(err, buf, sizeof(buf));
    if(R_FAILED(rc)) // a bit limiting, given the broad range of errors the kernel can give to us...
        strcpy(buf, "System busy, try again.");
    g_resolverResult = rc;
    return buf;
}

void herror(const char *str) {
    fprintf(stderr, "%s: %s\n", str, hstrerror(h_errno));
}

const char *gai_strerror(int err) {
    static __thread char buf[0x80]; // ResolverOptionLocalKey::GaiErrorStringBufferSizeUnsigned64
    Result rc = sfdnsresGetGaiStringErrorRequest(err, buf, sizeof(buf));
    if(R_FAILED(rc))
        strcpy(buf, "System busy, try again.");
    g_resolverResult = rc;
    return buf;
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    if (!node && !service)
        return EAI_NONAME;

    if (!res) {
        errno = EINVAL;
        return EAI_SYSTEM;
    }

    if (!g_resolverAddrInfoBufferSize) {
        errno = ENOSPC;
        return EAI_SYSTEM;
    }

    size_t hints_sz = 0;
    struct addrinfo_serialized_hdr *hints_serialized = NULL;
    if (hints) {
        // TODO: fixed size for serialized hints (g_resolverAddrInfoHintsBufferSize)
        (void)g_resolverAddrInfoHintsBufferSize;
        hints_serialized = _resolverSerializeAddrInfoList(&hints_sz, hints);
        if (!hints_serialized) {
            errno = ENOMEM;
            return EAI_MEMORY;
        }
    }

    struct addrinfo_serialized_hdr *out_serialized = malloc(g_resolverAddrInfoBufferSize);
    if (!out_serialized) {
        free(hints_serialized);
        errno = ENOMEM;
        return EAI_FAIL;
    }

    s32 gaie = 0;
    Result rc = sfdnsresGetAddrInfoRequest(
        g_resolverCancelHandle,
        !g_resolverDisableServiceDiscovery,
        node,
        service,
        hints_serialized, hints_sz,
        out_serialized, g_resolverAddrInfoBufferSize,
        (u32*)&errno,
        &gaie,
        NULL);
    g_resolverResult = rc;
    g_resolverCancelHandle = 0;
    free(hints_serialized);

    if (R_FAILED(rc)) {
        if (R_MODULE(rc) == 21) // SM
            errno = EAGAIN;
        else if (R_MODULE(rc) == 1) // Kernel
            errno = EFAULT;
        else
            errno = EPIPE;
        gaie = EAI_SYSTEM;
    }

    if (gaie == 0) {
        *res = _resolverDeserializeAddrInfoList(out_serialized);
        if (!*res) {
            errno = ENOMEM;
            gaie = EAI_MEMORY;
        }
    }

    free(out_serialized);
    return gaie;
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen,
                int flags) {
    Result rc = 0;
    u32 errno_ = 0;
    s32 gaie = 0;

    rc = sfdnsresGetNameInfoRequest(
        flags,
        sa, salen,
        host, hostlen,
        serv, servlen,
        g_resolverCancelHandle,
        &errno_,
        &gaie);
    g_resolverCancelHandle = 0;

    if(rc == 0xD401) {
        gaie = EAI_SYSTEM;
        errno = EFAULT;
        goto cleanup;
    }
    else if(R_FAILED(rc) && R_MODULE(rc) == 1) { // Kernel
        gaie = EAI_AGAIN;
        goto cleanup;
    }
    else if(R_FAILED(rc)) {
        gaie = EAI_FAIL;
        goto cleanup;
    }

    if(gaie != 0) {
        if(gaie == EAI_SYSTEM)
            errno = errno_;
    }

cleanup:
    g_resolverResult = rc;
    return gaie;
}

long gethostid(void) {
    u32 id = INADDR_LOOPBACK;

    Result rc = nifmInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = nifmGetCurrentIpAddress(&id);
        nifmExit();
    }

    g_resolverResult = rc;
    return id;
}

int gethostname(char *name, size_t namelen) {
    // The Switch doesn't have a proper name, so let's use its IP
    struct in_addr in;
    in.s_addr = gethostid();
    const char *hostname = inet_ntop(AF_INET, &in, name, namelen);
    return hostname == NULL ? -1 : 0;
}

// Unimplementable functions, left for compliance:
struct hostent *gethostent(void) { h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct netent *getnetbyaddr(uint32_t a, int b) { (void)a; (void)b; h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct netent *getnetbyname(const char *s) { (void)s; h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct netent *getnetent(void) { h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct protoent *getprotobyname(const char *s) { (void)s; h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct protoent *getprotobynumber(int a) { (void)a; h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct protoent *getprotoent(void) { h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct servent *getservbyname(const char *s1, const char *s2) { (void)s1; (void)s2; h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct servent *getservbyport(int a, const char *s) { (void)a; (void)s; h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
struct servent *getservent(void) { h_errno = NO_RECOVERY; errno = ENOSYS; return NULL; }
void sethostent(int a) { (void)a;}
void setnetent(int a) { (void)a;}
void setprotoent(int a) { (void)a; }
