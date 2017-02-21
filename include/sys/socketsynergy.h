/* SocketSynergy adds an API call to open a hole in outgoing firewalls */

#ifndef SYS_SOCKETSYNERGY_H
#define SYS_SOCKETSYNERGY_H


#include <stdint.h>
#include <netinet/in.h>


/* The general API call will be distributed to either a daemon for non-root
 * users, or a RAW header will be constructed directly for root users.
 */
int synergy (int sockfd, uint8_t hoplimit, struct sockaddr_in6 *symcli);


/* No more than a guess, the following hoplimit is likely to work in most
 * places -- but it does not guarantee anything, so it is a default at best.
 */
#define SYNERGY_HOPLIMIT_GUESS 3


/* The following request format is relayed to a socket running synergy.d
 * service.  The socket is a connection-less UNIX domain socket, which sends
 * no response, so the request is fully asynchronous.
 *
 * Along with the request, the socket to influence must be sent as ancillary
 * data, which is a portable POSIX technique.  Doing it this way means that
 * the receiving daemon can actually use the socket, because it will be a
 * duplicate.  The daemon will close the socket after it is done, to revert
 * the duplication process on the file descriptor.
 */
struct synergy_request_message {
	struct sockaddr_in6 symcli;
	uint8_t hoplimit;
};


/* The path leading to the synergy daemon socket.
 */
#define SYNERGY_DAEMON_SOCKET_PATH "/var/run/synergy.sock"

/* The path leading to the synergy daemon pidfile.
 */
#define SYNERGY_DAEMON_PID_FILE "/var/run/synergy.pid"


#endif /* SYS_SOCKETSYNERGY_H */
