#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <sys/socket.h>

const struct in6_addr in6addr_any = {0};
const struct in6_addr in6addr_loopback = {.__u6_addr32 = {0, 0, 0, __builtin_bswap32(1)}};

// Adapted from libctru
static int _inetAtonDetail(int *outBase, size_t *outNumBytes, const char *cp, struct in_addr *inp) {
    int      base;
    uint32_t val;
    int      c;
    char     bytes[4];
    size_t   num_bytes = 0;

    c = *cp;
    for(;;) {
        if(!isdigit(c)) return 0;

        val = 0;
        base = 10;
        if(c == '0') {
            c = *++cp;
            if(c == 'x' || c == 'X') {
                base = 16;
                c = *++cp;
            }
            else base = 8;
        }

        for(;;) {
            if(isdigit(c)) {
                if(base == 8 && c >= '8') return 0;
                val *= base;
                val += c - '0';
                c    = *++cp;
            }
            else if(base == 16 && isxdigit(c)) {
                val *= base;
                val += c + 10 - (islower(c) ? 'a' : 'A');
                c    = *++cp;
            }
            else break;
        }

        if(c == '.') {
            if(num_bytes > 3) return 0;
            if(val > 0xFF) return 0;
            bytes[num_bytes++] = val;
            c = *++cp;
        }
        else break;
    }

    if(c != 0) {
        *outNumBytes = num_bytes;
        *outBase = base;
        return 0;
    }

    switch(num_bytes) {
    case 0:
        break;

    case 1:
        if(val > 0xFFFFFF) return 0;
        val |= bytes[0] << 24;
        break;

    case 2:
        if(val > 0xFFFF) return 0;
        val |= bytes[0] << 24;
        val |= bytes[1] << 16;
        break;

    case 3:
        if(val > 0xFF) return 0;
        val |= bytes[0] << 24;
        val |= bytes[1] << 16;
        val |= bytes[2] << 8;
        break;
    }

    if(inp)
        inp->s_addr = htonl(val);

    *outNumBytes = num_bytes;
    *outBase = base;

    return 1;
}

// Adapted from libctru
static const char *inet_ntop4(const void *src, char *dst, socklen_t size) {
    const uint8_t *ip = src;

    char *p;
    size_t i;
    unsigned int n;

    if(size < INET_ADDRSTRLEN) {
        errno = ENOSPC;
        return NULL;
    }

    for(p = dst, i = 0; i < 4; ++i) {
        if(i > 0) *p++ = '.';

        n = ip[i];
        if(n >= 100) {
            *p++ = n/100 + '0';
            n %= 100;
        }
        if(n >= 10 || ip[i] >= 100) {
            *p++ = n/10 + '0';
            n %= 10;
        }
        *p++ = n + '0';
    }
    *p = 0;

    return dst;
}

static int inet_pton4(const char *src, void *dst) {
    int base;
    size_t numBytes;

    int ret = _inetAtonDetail(&base, &numBytes, src, (struct in_addr *)dst);
    return (ret == 1 && base == 10 && numBytes == 3) ? 1 : 0;
}

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#define INADDRSZ 4
#define IN6ADDRSZ 16
#define INT16SZ 2
/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */
static const char *
inet_ntop6(src, dst, size)
    const u_char *src;
    char *dst;
    size_t size;
{
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct { int base, len; } best = {0}, cur = {0};
    u_int words[IN6ADDRSZ / INT16SZ];
    int i;

    /*
     * Preprocess:
     *	Copy the input (bytewise) array into a wordwise array.
     *	Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, 0, sizeof words);
    for (i = 0; i < IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    cur.base = -1;
    for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base &&
            i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
            if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
                return (NULL);
            tp += strlen(tp);
            break;
        }
        //TuxSH:
        //sprintf(tp, "%x", words[i]);
        {
            char hexbuf[8];
            char *e = hexbuf + 7;
            u_int word = words[i];
            while(word > 0) {
                static const char digits[] = "0123456789abcdef";
                *e-- = digits[word & 0xF];
                word >>= 4;
            }

            memcpy(tp, e + 1, hexbuf + 8 - (e + 1));
        }
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) == (IN6ADDRSZ / INT16SZ))
        *tp++ = ':';
    *tp++ = '\0';

    /*
     * Check for overflow, copy, and we're done.
     */
    if ((tp - tmp) > size) {
        errno = ENOSPC;
        return (NULL);
    }
    strcpy(dst, tmp);
    return (dst);
}

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton6(src, dst)
    const char *src;
    u_char *dst;
{
    static const char xdigits_l[] = "0123456789abcdef",
              xdigits_u[] = "0123456789ABCDEF";
    u_char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, saw_xdigit;
    u_int val;

    memset((tp = tmp), 0, IN6ADDRSZ);
    endp = tp + IN6ADDRSZ;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return (0);
    curtok = src;
    saw_xdigit = 0;
    val = 0;
    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return (0);
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':') {
            curtok = src;
            if (!saw_xdigit) {
                if (colonp)
                    return (0);
                colonp = tp;
                continue;
            }
            if (tp + INT16SZ > endp)
                return (0);
            *tp++ = (u_char) (val >> 8) & 0xff;
            *tp++ = (u_char) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
            inet_pton4(curtok, tp) > 0) {
            tp += INADDRSZ;
            saw_xdigit = 0;
            break;	/* '\0' was seen by inet_pton4(). */
        }
        return (0);
    }
    if (saw_xdigit) {
        if (tp + INT16SZ > endp)
            return (0);
        *tp++ = (u_char) (val >> 8) & 0xff;
        *tp++ = (u_char) val & 0xff;
    }
    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;
        int i;

        for (i = 1; i <= n; i++) {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return (0);
    /* bcopy(tmp, dst, IN6ADDRSZ); */
    memcpy(dst, tmp, IN6ADDRSZ);
    return (1);
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    switch(af) {
        case AF_INET:
            return inet_ntop4(src, dst, size);
        case AF_INET6:
            return inet_ntop6(src, dst, size);
        default:
            errno = EAFNOSUPPORT;
            return NULL;
    }
}

int inet_pton(int af, const char *src, void *dst) {
    switch(af) {
        case AF_INET:
            return inet_pton4(src, dst);
        case AF_INET6:
            return inet_pton6(src, dst);
        default:
            errno = EAFNOSUPPORT;
            return -1;
    }
}

char *inet_ntoa(struct in_addr in) {
    static __thread char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &in.s_addr, buffer, INET_ADDRSTRLEN);
    return buffer;
}

int inet_aton(const char *cp, struct in_addr *inp) {
    int base;
    size_t numBytes;
    return _inetAtonDetail(&base, &numBytes, cp, inp);
}

in_addr_t inet_addr(const char *cp) {
    struct in_addr addr = { .s_addr = INADDR_BROADCAST };
    inet_aton(cp, &addr);
    return addr.s_addr;
}
