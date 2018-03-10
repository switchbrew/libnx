// TuxSH: removed definitions under _KERNEL ifdef blocks, modify the prototype of some functions, some other cleanup, etc.
#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif

/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)socket.h	8.4 (Berkeley) 2/21/94
 * $FreeBSD$
 */

#ifndef _SYS_SOCKET_H_
#define	_SYS_SOCKET_H_

#include <sys/cdefs.h>
#include <sys/_types.h>
#include <sys/_iovec.h>

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Data types.
 */

// Stripped some of them
#include <sys/types.h>  // for ssize_t, etc.
#include <stdint.h>

#ifndef _SA_FAMILY_T_DECLARED
typedef	__sa_family_t	sa_family_t;
#define	_SA_FAMILY_T_DECLARED
#endif

#ifndef _SOCKLEN_T_DECLARED
typedef	__socklen_t	socklen_t;
#define	_SOCKLEN_T_DECLARED
#endif

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#if __BSD_VISIBLE
#define	SOCK_RDM	4		/* reliably-delivered message */
#endif
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

#if __BSD_VISIBLE
/*
 * Creation flags, OR'ed into socket() and socketpair() type argument.
 */
#define	SOCK_CLOEXEC	0x10000000
#define	SOCK_NONBLOCK	0x20000000

#endif	/* __BSD_VISIBLE */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#if __BSD_VISIBLE
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#endif
#define	SO_LINGER	0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */
#if __BSD_VISIBLE
#define	SO_REUSEPORT	0x0200		/* allow local address & port reuse */
#define	SO_TIMESTAMP	0x0400		/* timestamp received dgram traffic */
#define	SO_NOSIGPIPE	0x0800		/* no SIGPIPE from EPIPE */
#define	SO_ACCEPTFILTER	0x1000		/* there is an accept filter */
#define	SO_BINTIME	0x2000		/* timestamp received dgram traffic */
#endif
#define	SO_NO_OFFLOAD	0x4000		/* socket cannot be offloaded */
#define	SO_NO_DDP	0x8000		/* disable direct data placement */

/*
 * Additional options, not kept in so_options.
 */
#define	SO_SNDBUF	0x1001		/* send buffer size */
#define	SO_RCVBUF	0x1002		/* receive buffer size */
#define	SO_SNDLOWAT	0x1003		/* send low-water mark */
#define	SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define	SO_SNDTIMEO	0x1005		/* send timeout */
#define	SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#if __BSD_VISIBLE
#define	SO_LABEL	0x1009		/* socket's MAC label */
#define	SO_PEERLABEL	0x1010		/* socket's peer's MAC label */
#define	SO_LISTENQLIMIT	0x1011		/* socket's backlog limit */
#define	SO_LISTENQLEN	0x1012		/* socket's complete queue length */
#define	SO_LISTENINCQLEN	0x1013	/* socket's incomplete queue length */
#define	SO_SETFIB	0x1014		/* use this FIB to route */
#define	SO_USER_COOKIE	0x1015		/* user cookie (dummynet etc.) */
#define	SO_PROTOCOL	0x1016		/* get socket protocol (Linux name) */
#define	SO_PROTOTYPE	SO_PROTOCOL	/* alias for SO_PROTOCOL (SunOS name) */
#define	SO_TS_CLOCK	0x1017		/* clock type used for SO_TIMESTAMP */
#define	SO_MAX_PACING_RATE	0x1018	/* socket's max TX pacing rate (Linux name) */
#endif

#if __BSD_VISIBLE
#define	SO_TS_REALTIME_MICRO	0	/* microsecond resolution, realtime */
#define	SO_TS_BINTIME		1	/* sub-nanosecond resolution, realtime */
#define	SO_TS_REALTIME		2	/* nanosecond resolution, realtime */
#define	SO_TS_MONOTONIC		3	/* nanosecond resolution, monotonic */
#define	SO_TS_DEFAULT		SO_TS_REALTIME_MICRO
#define	SO_TS_CLOCK_MAX		SO_TS_MONOTONIC
#endif

/*
 * Space reserved for new socket options added by third-party vendors.
 * This range applies to all socket option levels.  New socket options
 * in FreeBSD should always use an option value less than SO_VENDOR.
 */
#if __BSD_VISIBLE
#define	SO_VENDOR	0x80000000
#endif

/*
 * Structure used for manipulating linger option.
 */
struct linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

#if __BSD_VISIBLE
struct accept_filter_arg {
	char	af_name[16];
	char	af_arg[256-16];
};
#endif

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#if __BSD_VISIBLE
#define	AF_LOCAL	AF_UNIX		/* local to host (pipes, portals) */
#endif
#define	AF_UNIX		1		/* standardized name for AF_LOCAL */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#if __BSD_VISIBLE
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NETBIOS	6		/* SMB protocols */
#define	AF_ISO		7		/* ISO protocols */
#define	AF_OSI		AF_ISO
#define	AF_ECMA		8		/* European computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* DEC Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define	AF_APPLETALK	16		/* Apple Talk */
#define	AF_ROUTE	17		/* Internal Routing Protocol */
#define	AF_LINK		18		/* Link layer interface */
#define	pseudo_AF_XTP	19		/* eXpress Transfer Protocol (no AF) */
#define	AF_COIP		20		/* connection-oriented IP, aka ST II */
#define	AF_CNT		21		/* Computer Network Technology */
#define pseudo_AF_RTIP	22		/* Help Identify RTIP packets */
#define	AF_IPX		23		/* Novell Internet Protocol */
#define	AF_SIP		24		/* Simple Internet Protocol */
#define	pseudo_AF_PIP	25		/* Help Identify PIP packets */
#define	AF_ISDN		26		/* Integrated Services Digital Network*/
#define	AF_E164		AF_ISDN		/* CCITT E.164 recommendation */
#define	pseudo_AF_KEY	27		/* Internal key-management function */
#endif
#define	AF_INET6	28		/* IPv6 */
#if __BSD_VISIBLE
#define	AF_NATM		29		/* native ATM access */
#define	AF_ATM		30		/* ATM */
#define pseudo_AF_HDRCMPLT 31		/* Used by BPF to not rewrite headers
					 * in interface output routine
					 */
#define	AF_NETGRAPH	32		/* Netgraph sockets */
#define	AF_SLOW		33		/* 802.3ad slow protocol */
#define	AF_SCLUSTER	34		/* Sitara cluster protocol */
#define	AF_ARP		35
#define	AF_BLUETOOTH	36		/* Bluetooth sockets */
#define	AF_IEEE80211	37		/* IEEE 802.11 protocol */
#define	AF_INET_SDP	40		/* OFED Socket Direct Protocol ipv4 */
#define	AF_INET6_SDP	42		/* OFED Socket Direct Protocol ipv6 */
#define	AF_MAX		42
/*
 * When allocating a new AF_ constant, please only allocate
 * even numbered constants for FreeBSD until 134 as odd numbered AF_
 * constants 39-133 are now reserved for vendors.
 */
#define AF_VENDOR00 39
#define AF_VENDOR01 41
#define AF_VENDOR02 43
#define AF_VENDOR03 45
#define AF_VENDOR04 47
#define AF_VENDOR05 49
#define AF_VENDOR06 51
#define AF_VENDOR07 53
#define AF_VENDOR08 55
#define AF_VENDOR09 57
#define AF_VENDOR10 59
#define AF_VENDOR11 61
#define AF_VENDOR12 63
#define AF_VENDOR13 65
#define AF_VENDOR14 67
#define AF_VENDOR15 69
#define AF_VENDOR16 71
#define AF_VENDOR17 73
#define AF_VENDOR18 75
#define AF_VENDOR19 77
#define AF_VENDOR20 79
#define AF_VENDOR21 81
#define AF_VENDOR22 83
#define AF_VENDOR23 85
#define AF_VENDOR24 87
#define AF_VENDOR25 89
#define AF_VENDOR26 91
#define AF_VENDOR27 93
#define AF_VENDOR28 95
#define AF_VENDOR29 97
#define AF_VENDOR30 99
#define AF_VENDOR31 101
#define AF_VENDOR32 103
#define AF_VENDOR33 105
#define AF_VENDOR34 107
#define AF_VENDOR35 109
#define AF_VENDOR36 111
#define AF_VENDOR37 113
#define AF_VENDOR38 115
#define AF_VENDOR39 117
#define AF_VENDOR40 119
#define AF_VENDOR41 121
#define AF_VENDOR42 123
#define AF_VENDOR43 125
#define AF_VENDOR44 127
#define AF_VENDOR45 129
#define AF_VENDOR46 131
#define AF_VENDOR47 133
#endif

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	unsigned char	sa_len;		/* total length */
	sa_family_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
};
#if __BSD_VISIBLE
#define	SOCK_MAXADDRLEN	255		/* longest possible addresses */

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	unsigned short	sp_family;		/* address family */
	unsigned short	sp_protocol;		/* protocol */
};
#endif

#include <sys/_sockaddr_storage.h>

#if __BSD_VISIBLE
/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_LOCAL	AF_LOCAL
#define	PF_UNIX		PF_LOCAL	/* backward compatibility */
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NETBIOS	AF_NETBIOS
#define	PF_ISO		AF_ISO
#define	PF_OSI		AF_ISO
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_ROUTE	AF_ROUTE
#define	PF_LINK		AF_LINK
#define	PF_XTP		pseudo_AF_XTP	/* really just proto family, no AF */
#define	PF_COIP		AF_COIP
#define	PF_CNT		AF_CNT
#define	PF_SIP		AF_SIP
#define	PF_IPX		AF_IPX
#define PF_RTIP		pseudo_AF_RTIP	/* same format as AF_INET */
#define PF_PIP		pseudo_AF_PIP
#define	PF_ISDN		AF_ISDN
#define	PF_KEY		pseudo_AF_KEY
#define	PF_INET6	AF_INET6
#define	PF_NATM		AF_NATM
#define	PF_ATM		AF_ATM
#define	PF_NETGRAPH	AF_NETGRAPH
#define	PF_SLOW		AF_SLOW
#define PF_SCLUSTER	AF_SCLUSTER
#define	PF_ARP		AF_ARP
#define	PF_BLUETOOTH	AF_BLUETOOTH
#define	PF_IEEE80211	AF_IEEE80211
#define	PF_INET_SDP	AF_INET_SDP
#define	PF_INET6_SDP	AF_INET6_SDP

#define	PF_MAX		AF_MAX

/*
 * Definitions for network related sysctl, CTL_NET.
 *
 * Second level is protocol family.
 * Third level is protocol number.
 *
 * Further levels are defined by the individual families.
 */

/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *	Fourth: address family, 0 is wildcard
 *	Fifth: type of info, defined below
 *	Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define NET_RT_DUMP	1		/* dump; may limit to a.f. */
#define NET_RT_FLAGS	2		/* by flags, e.g. RESOLVING */
#define NET_RT_IFLIST	3		/* survey interface list */
#define	NET_RT_IFMALIST	4		/* return multicast address list */
#define	NET_RT_IFLISTL	5		/* Survey interface list, using 'l'en
					 * versions of msghdr structs. */
#endif /* __BSD_VISIBLE */

/*
 * Maximum queue length specifiable by listen.
 */
#define	SOMAXCONN	128

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr {
	void		*msg_name;		/* optional address */
	socklen_t	 msg_namelen;		/* size of address */
	struct iovec	*msg_iov;		/* scatter/gather array */
	int		 msg_iovlen;		/* # elements in msg_iov */
	void		*msg_control;		/* ancillary data, see below */
	socklen_t	 msg_controllen;	/* ancillary data buffer len */
	int		 msg_flags;		/* flags on received message */
};

#define	MSG_OOB		 0x00000001	/* process out-of-band data */
#define	MSG_PEEK	 0x00000002	/* peek at incoming message */
#define	MSG_DONTROUTE	 0x00000004	/* send without using routing tables */
#define	MSG_EOR		 0x00000008	/* data completes record */
#define	MSG_TRUNC	 0x00000010	/* data discarded before delivery */
#define	MSG_CTRUNC	 0x00000020	/* control data lost before delivery */
#define	MSG_WAITALL	 0x00000040	/* wait for full request or error */
#if __BSD_VISIBLE
#define	MSG_DONTWAIT	 0x00000080	/* this message should be nonblocking */
#define	MSG_EOF		 0x00000100	/* data completes connection */
/*			 0x00000200	   unused */
/*			 0x00000400	   unused */
/*			 0x00000800	   unused */
/*			 0x00001000	   unused */
#define	MSG_NOTIFICATION 0x00002000	/* SCTP notification */
#define	MSG_NBIO	 0x00004000	/* FIONBIO mode, used by fifofs */
#define	MSG_COMPAT       0x00008000		/* used in sendit() */
#endif
#if __POSIX_VISIBLE >= 200809
#define	MSG_NOSIGNAL	 0x00020000	/* do not generate SIGPIPE on EOF */
#endif
#if __BSD_VISIBLE
#define	MSG_CMSG_CLOEXEC 0x00040000	/* make received fds close-on-exec */
#define	MSG_WAITFORONE	 0x00080000	/* for recvmmsg() */
#endif

/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
	socklen_t	cmsg_len;		/* data byte count, including hdr */
	int		cmsg_level;		/* originating protocol */
	int		cmsg_type;		/* protocol-specific type */
/* followed by	u_char  cmsg_data[]; */
};

// socket credential stuff, we don't need this
// cmsg macros, uses some obscure macro

/* "Socket"-level control message types: */
#define	SCM_RIGHTS	0x01		/* access rights (array of int) */
#if __BSD_VISIBLE
#define	SCM_TIMESTAMP	0x02		/* timestamp (struct timeval) */
#define	SCM_CREDS	0x03		/* process creds (struct cmsgcred) */
#define	SCM_BINTIME	0x04		/* timestamp (struct bintime) */
#define	SCM_REALTIME	0x05		/* timestamp (struct timespec) */
#define	SCM_MONOTONIC	0x06		/* timestamp (struct timespec) */
#define	SCM_TIME_INFO	0x07		/* timestamp info */

struct sock_timestamp_info {
	uint32_t	st_info_flags;
	uint32_t	st_info_pad0;
	uint64_t	st_info_rsv[7];
};

#define	ST_INFO_HW		0x0001		/* SCM_TIMESTAMP was hw */
#define	ST_INFO_HW_HPREC	0x0002		/* SCM_TIMESTAMP was hw-assisted
						   on entrance */
#endif

#if __BSD_VISIBLE
/*
 * 4.3 compat sockaddr, move to compat file later
 */
struct osockaddr {
	unsigned short sa_family;	/* address family */
	char	sa_data[14];		/* up to 14 bytes of direct address */
};

/*
 * 4.3-compat message header (move to compat file later).
 */
struct omsghdr {
	char	*msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	char	*msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};
#endif

/*
 * howto arguments for shutdown(2), specified by Posix.1g.
 */
#define	SHUT_RD		0		/* shut down the reading side */
#define	SHUT_WR		1		/* shut down the writing side */
#define	SHUT_RDWR	2		/* shut down both sides */

#if __BSD_VISIBLE
/* for SCTP */
/* we cheat and use the SHUT_XX defines for these */
#define PRU_FLUSH_RD     SHUT_RD
#define PRU_FLUSH_WR     SHUT_WR
#define PRU_FLUSH_RDWR   SHUT_RDWR
#endif


#if __BSD_VISIBLE
/*
 * sendfile(2) header/trailer struct
 */
struct sf_hdtr {
	struct iovec *headers;	/* pointer to an array of header struct iovec's */
	int hdr_cnt;		/* number of header iovec's */
	struct iovec *trailers;	/* pointer to an array of trailer struct iovec's */
	int trl_cnt;		/* number of trailer iovec's */
};

/*
 * Sendfile-specific flag(s)
 */
#define	SF_NODISKIO     0x00000001
#define	SF_MNOWAIT	0x00000002	/* obsolete */
#define	SF_SYNC		0x00000004
#define	SF_USER_READAHEAD	0x00000008
#define	SF_NOCACHE	0x00000010
#define	SF_FLAGS(rh, flags)	(((rh) << 16) | (flags))
/*
 * Sendmmsg/recvmmsg specific structure(s)
 */
struct mmsghdr {
	struct msghdr	msg_hdr;		/* message header */
	ssize_t		msg_len;		/* message length */
};
#endif /* __BSD_VISIBLE */

#include <sys/cdefs.h>

// Note: rewrote the definitions to make them more POSIX-compliant and such, adding back arg names
// Remove some function declarations

__BEGIN_DECLS

// Note: POSIX claims that some prototypes should take restrict ptrs
// But this causes more problem that it solves, so much like linux
// We won't declare those args restrict.

int socket(int domain, int type, int protocol);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t send(int sockfd, const void* buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int listen(int sockfd, int backlog);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int shutdown(int sockfd, int how);

int sockatmark(int sockfd);
int socketpair(int domain, int type, int protocol, int sv[2]);

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);

#if __BSD_VISIBLE
struct timespec;
int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags);
int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout);
#endif
__END_DECLS

#endif /* !_SYS_SOCKET_H_ */
