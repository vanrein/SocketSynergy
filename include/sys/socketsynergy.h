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


#endif /* SYS_SOCKETSYNERGY_H */
