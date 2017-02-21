/* SocketSynergy adds an API call to open a hole in outgoing firewalls */

#ifndef SYS_SOCKETSYNERGY_H
#define SYS_SOCKETSYNERGY_H


#include <stdint.h>
#include <netinet/in.h>

int synergy (int sockfd, uint8_t hoplimit, struct sockaddr_in6 *symcli);


#endif /* SYS_SOCKETSYNERGY_H */
