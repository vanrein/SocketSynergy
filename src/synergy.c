/* synergy.c -- Send a SYN to open a hole in an outgoing firewall
 *
 * In theory, UDP/TCP ports can just listen for incoming traffic.
 * In practice, firewalls make this impossible for traffic that crosses
 * over the Internet.
 *
 * Incoming holes can be configured on a firewall for generally available
 * servers.  This however, is not an answer to more ad-hoc, or peer-to-peer
 * holes.  For those, it is better to punch a hole at the precise time it
 * is required.
 *
 * This program sends out a fake SYN message that will open the firewall,
 * and that drops with an ICMPv6 hop limit exceeded message somewhere in
 * an Internet router.  After this, the port should be open for further
 * use, provided it arrives quickly.  The hop limit message should be
 * ignored by firewalls, as well as by a non-initiated, listening socket.
 *
 * The firewall is assumed to record both end points of the communication
 * for the connection being setup.  To make this work, it is necessary for
 * both end points to be known.  In other words, the socket supplied must
 * respond to both getsockname() and getpeername() calls.  It is further
 * assumed that these addresses are used (in reverse order) by the future
 * incoming call.
 *
 * It is vital to the reliability of this trick to set the hop limit so
 * that the message just opens the last firewall that protects against
 * the big, bad Internet.  If the hop limit is set too low, the hole
 * will not be made.  If set too high, some nearby connections may fail
 * due to a TCP RST returned from a firewall on the other end; if set
 * too low, the local firewall will not be opened.  An automatic procedure
 * for determining this value exists for systems that have IPv4 in place.
 * A similar mechanism has not been devised for native IPv6-only systems.
 *
 * If this is generally thought to be useful, the approach can be merged
 * with kernels as a new socket API call.  The hoplimit could then be
 * set with sysctl, named net.ipv6.conf.*.synergy_hoplimit defaulting to 0.
 * Note that the code below assumes a single interface is being used.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

union rawmsg {
	struct udpmsg {
		struct udphdr hdr;
	} udppkt;
	struct tcpmsg {
		struct tcphdr hdr;
	} tcppkt;
};

struct hoplimiter {
	struct cmsghdr hlcm;
	int hoplimit;
};


int synergy (int sockfd, uint8_t hoplimit, struct sockaddr_in6 *symcli) {
	struct sockaddr_in6 local, remot;
	struct sockaddr_in6 rawnm;
	int type;
	int proto;
	int rawsox;
	union rawmsg rawmsg;
	size_t namesz = sizeof (local);
	socklen_t typesz = sizeof (type);

	//
	// Fetch information, and ensure that it is proper
	if (getsockname (sockfd, (struct sockaddr *) &local, &namesz)) {
		return -1;
	}
	if (local.sin6_family != AF_INET6) {
		errno = EAFNOSUPPORT;
		return -1;
	}
	if (symcli == NULL) {
		if (getpeername (sockfd, (struct sockaddr *) &remot, &namesz)) {
			return -1;
		}
		symcli = &remot;
	}
	if (getsockopt (sockfd, SOL_SOCKET, SO_TYPE, &type, &typesz)) {
		return -1;
	}
	if (type == SOCK_STREAM) {
		proto = IPPROTO_TCP;
	} else if (type == SOCK_DGRAM) {
		proto = IPPROTO_UDP;
	} else {
		errno = EBADF;
		return -1;
	}

	// Clean resources and initialise the raw message
	rawsox = socket (PF_INET6, SOCK_RAW, proto);
	if (rawsox == -1) {
		return -1;
	}
	memset (&rawmsg, 0, sizeof (rawmsg));

	//
	// Construct TCP or UDP header
	struct iovec io [1];
	if (type == SOCK_STREAM) {
		rawmsg.tcppkt.hdr.source = local.sin6_port;
		rawmsg.tcppkt.hdr.dest   = symcli->sin6_port;
		rawmsg.tcppkt.hdr.doff   = 5;
		rawmsg.tcppkt.hdr.syn    = 1; // Sending SYN is the main purpose
		io->iov_base = &rawmsg.tcppkt;
		io->iov_len = sizeof (rawmsg.tcppkt);
	} else {
		// TODO: Checksum not calculated by kernel?
		rawmsg.udppkt.hdr.source = local.sin6_port;
		rawmsg.udppkt.hdr.dest   = symcli->sin6_port;
		rawmsg.udppkt.hdr.len    = htons (8);
		io->iov_base = &rawmsg.udppkt;
		io->iov_len = sizeof (rawmsg.udppkt);
	}

	struct hoplimiter hl;
	hl.hlcm.cmsg_level = IPPROTO_IPV6;
	hl.hlcm.cmsg_type  = IPV6_HOPLIMIT;
	hl.hlcm.cmsg_len   = sizeof (hl);
	hl.hoplimit = hoplimit;

	memcpy (&rawnm, symcli, sizeof (struct sockaddr_in6));
	rawnm.sin6_port = htons (0);	/* Socket defines IPPROTO_xxx */

	struct msghdr mh;
	mh.msg_name = &rawnm;
	mh.msg_namelen = sizeof (rawnm);
	mh.msg_iov = io;
	mh.msg_iovlen = 1;
	mh.msg_control = &hl;
	mh.msg_controllen = sizeof (hl);
	mh.msg_flags = MSG_NOSIGNAL;

	//
	// Have checksums calculated by the kernel and send the message
	if (sendmsg (rawsox, &mh, MSG_NOSIGNAL) == -1) {
		return -1;
	}

	close (rawsox);
	return 0;
}

