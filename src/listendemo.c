/* listener.c -- Demonstration of libsynergy
 *
 * This is a simple demonstration tool that will listen with synergy.
 * That is, it will punch a hole in the firewalls that normally
 * protect it against attacks.
 * 
 * Parameters:
 *  1. The word "sctp", "tcp" or "udp" to signify the protocol to use
 *  2. The listening IPv6 address
 *  3. The listening port
 *  4. The remote IPv6 address that will connect
 *  5. The remote port
 *  6. The hop limit to pass any and all firewalls to open up
 *
 * The daemon will copy stdin out over the network, and any return
 * traffic will be dumped on stdout.  Given a properly tuned (that
 * is, minimal-but-effective) hop limit, this should always work.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <arpa/inet.h>
#include <netinet/ip6.h>


/*
 * Global variables
 */
int cnxtp;
int proto;
struct sockaddr_in6 local, remot;
int hoplimit = 3;


int main (int argc, char *argv []) {
	//
	// Initialise
	memset (&local, 0, sizeof (local));
	memset (&remot, 0, sizeof (remot));
	local.sin6_family = AF_INET6;
	remot.sin6_family = AF_INET6;

	//
	// Parse parameters
	if (argc != 7) {
		fprintf (stderr, "Usage: %s sctp|tcp|udp local-addr local-port remote-addr remote-port hoplimit\n", argv [0]);
		exit (1);
	}
	if (strcmp (argv [1], "sctp") == 0) {
		cnxtp = SOCK_SEQPACKET;
		proto = IPPROTO_SCTP;
	} else if (strcmp (argv [1], "tcp") == 0) {
		cnxtp = SOCK_STREAM;
		proto = 0;
	} else if (strcmp (argv [1], "udp") == 0) {
		cnxtp = SOCK_DGRAM;
		proto = 0;
	} else {
		fprintf (stderr, "%s: First parameter should be sctp, tcp or udp, not '%s'\n",
				argv [0], argv [1]);
		exit (1);
	}
	errno = EINVAL; // Cover retval 0 from inet_pton()
	if (inet_pton (AF_INET6, argv [2], &local.sin6_addr) <= 0) {
		fprintf (stderr, "%s: Failed to parse '%s' as a local IPv6 address\n",
				argv [0], argv [2]);
		exit (1);
	}
	if (inet_pton (AF_INET6, argv [4], &remot.sin6_addr) <= 0) {
		fprintf (stderr, "%s: Failed to parse '%s' as a remote IPv6 address\n",
				argv [0], argv [2]);
		exit (1);
	}
	errno = 0;
	long intval;
	intval = atol (argv [3]);
	if ((intval <= 0) || (intval >= 65536)) {
		fprintf (stderr, "%s: Local port %s is not valid\n",
				argv [0], argv [3]);
		exit (1);
	}
	local.sin6_port = htons (intval);
	intval = atol (argv [5]);
	if ((intval <= 0) || (intval >= 65536)) {
		fprintf (stderr, "%s: Remote port %s is not valid\n",
				argv [0], argv [5]);
		exit (1);
	}
	remot.sin6_port = htons (intval);
	intval = atol (argv [6]);
	if ((intval <= 0) || (intval >= 256)) {
		fprintf (stderr, "%s: Hop limit %s is not in the valid range 0..255\n",
				argv [0], argv [6]);
		exit (1);
	}

	//
	// Setup the socket for TCP or UDP over IPv6
	int sox = socket (PF_INET6, cnxtp, proto);
	if (sox == -1) {
		fprintf (stderr, "%s: Failed to create a socket: %s\n",
				argv [0], strerror (errno));
		exit (1);
	}
	if (bind (sox, (struct sockaddr *) &local, sizeof (local)) == -1) {
		fprintf (stderr, "%s: Failed to bind to local address/port: %s\n",
				argv [0], strerror (errno));
		exit (1);
	}

	//
	// Handle TCP and SCTP over IPv6 with listen/synergy/accept
	if ((cnxtp == SOCK_STREAM) || (cnxtp == SOCK_SEQPACKET)) {
		if (listen (sox, 5) == -1) {
			fprintf (stderr, "%s: Failed to listen to socket: %s\n",
				argv [0], strerror (errno));
			exit (1);
		}
		if (synergy (sox, hoplimit, &remot) == -1) {
			fprintf (stderr, "%s: Failed to run synergy on SCTP or TCP socket: %s\n",
				argv [0], strerror (errno));
			exit (1);
		}
		sox = accept (sox, NULL, 0);
		if (sox == -1) {
			fprintf (stderr, "%s: Failed to accept connection from socket: %s\n",
				argv [0], strerror (errno));
			exit (1);
		}
	}

	//
	// Handle UDP over IPv6 with connect/synergy
	if (cnxtp == SOCK_DGRAM) {
		if (connect (sox, (struct sockaddr *) &remot, sizeof (remot)) == -1) {
			fprintf (stderr, "%s: Failed to listen to socket: %s\n",
				argv [0], strerror (errno));
			exit (1);
		}
		if (synergy (sox, hoplimit, &remot) == -1) {
			fprintf (stderr, "%s: Failed to run synergy on UDP socket: %s\n",
				argv [0], strerror (errno));
			exit (1);
		}
	}

	//
	// Copy data between stdin/stdout and the socket
	while (1) {
		fd_set both;
		uint8_t buf [1024];
		FD_ZERO (    &both);
		FD_SET (0,   &both);
		FD_SET (sox, &both);
		if (select (sox+1, &both, NULL, NULL, NULL) == -1) {
			fprintf (stderr, "%s: Failure to select on stdin file descriptor and socket: %s\n",
				argv [0], strerror (errno));
			exit (1);
		}
		if (FD_ISSET (0,   &both)) {
			ssize_t len = read (0,   &buf, 1024);
			if (len == 0) {
				break;
			}
			write (sox, &buf, len);
		}
		if (FD_ISSET (sox, &both)) {
			ssize_t len = read (sox, &buf, 1024);
			if (len == 0) {
				break;
			}
			write (1,   &buf, len);
		}
	}

	//
	// Close down and report succes
	shutdown (sox, SHUT_RDWR);
	exit (0);
}
