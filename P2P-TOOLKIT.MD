# p2p Toolkit

> *The p2p toolkit is a set of network tools that simplify peer-to-peer
> communication.  SocketSynergy is one of these tools.*

The p2p Toolkit consists of the following tools:

  * [SocketSynergy](http://github.com/vanrein/SocketSyngergy)
    is a portable trick for opening holes in outgoing firewalls.
    When used on both ends prior to actually connecting, this leads
    to the ability of connecting UDP, TCP and SCTP in spite of local
    firewalls on both ends.  SocketSynergy relies on well-defined
    firewall behaviour.  It assumes transparant addressing.  It offers
    an extra socket API call, which is applied directly for root users,
    or passed on to a daemon for others.

  * [6bed4](https://github.com/vanrein/6bed4)
    is a tunnel that allows IPv6 connectivity anywhere.  Although it can
    fallback to a server that connects end points, it only does this when
    direct connections fail.  Such failure is hardly a serious expectation,
    it only occurs as a result of Symmetric NAT.  The server is there to
    give a guarantee even in such exceptional cases.  The result of using
    6bed4 is that one can assume *IPv6 Anywhere*.  The IPv6 facility needs
    no configuration, and is suitable for fast and interactive traffic such
    as interactive media.

  * [Symmetric TLS](https://github.com/arpa2/draft-vanrein-tls-symmetry)
    is a mechanism that supports the use of TLS without prior decision on
    which side plays the role of client or server.  This would otherwise be
    needed for TLS only, when transport and application layers are mirrorred.
    The work modifies the message flow of TLS such that it can negotiate the
    desired independency; it also works together with classic implementations
    of TLS that insist on either the client or server role.  Symmetric TLS is
    implemented in GnuTLS.


## Acknowledgements

The p2p Toolkit was funded by
[SIDN Fonds](https://www.sidnfonds.nl).
