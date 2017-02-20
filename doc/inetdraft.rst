-----------------------------------------------
Synergy: Peer-to-Peer traffic through Firewalls
-----------------------------------------------

Updates: RFC 5128


Abstract
========

This specification defines a technique for punching holes in firewalls
in such a way that it enables peer-to-peer communication over UDP and TCP.
This technique does not require specific protocols (such as UPnP) to open
the holes, nor does it create friction with the original design of the
protocols.  However, when network addresses and ports are translated
en route, it is likely to fail.  In most practical scenarios, this means
that IPv6 should be used.


.. contents::


Introduction
============

Many Internet protocols are designed as client/server interactions.
The server in this part is passively listening for incoming traffic
on a well-known port.  Any firewalls that would protect such
incoming traffic are usually under the same control as the server,
and so they can be manually opened up to permit incoming traffic to the
server.

Peer-to-peer interactions are different, in that they are setup
as ad-hoc connections between endpoints.  Those endpoints usually
sit behind one or more firewalls that default to stopping
al incoming initiatives, and some of these may be under
control of an third party, such as an Internet Service Provider.

Peer-to-peer interactions are also different in that they are
setup in a more synchronous fashion than a client/server
connection; an external protocol such as SDP has usually
exchanged the endpoints that are ready to engage in the
peer-to-peer connection, so there need not be one passive and one
active side, such as with client/server connectivity.  The
possibility to use the Simultaneous Open facility built into TCP is in
this sense intruiging; it means that two clients actively seek
to connect to the other side, and by taking that initiative
they will open the firewalls protecting their end of the
communication.

Unfortunately, if a connection attempt hits upon a remote firewall
before it has sent out the remote side's connection attempt,
it will block the traffic.  Loosing packets is no problem if the protocol is UDP,
but in case of TCP there is a problem.  Some remote firewalls respond with
a connection reset, thereby tearing down the hole that was being
punched in the local firewall, and the situation returns to the
closed state that existed before the connection attempt.

The solution to this problem is to initiate a connection in such a
way that it passes out through all local firewalls, but never
arrives at the remote firewall.  An easy way of doing that
is by setting the TTL (for IPv4) or Hop Limit (for IPv6) so low
that the packet will be dropped between the local and remote firewalls.

This specification first discuss the model assumed for the
connection build-up, then how the specially crafted message passes
through it.
Later sections discuss  how to gradually learn from failures to connect,
as well as an extension to the socket interface that
supports the generation of the intended messages.

A descriptive name is often useful to be able to refer to a
technique; the one described here is referred to as "synergy"
because that seems to make some informal sense in light of the
peer-to-peer application area; at the same time, it is a
light reference to the SYN packet crafted for the TCP variation
of the technique.


Modelling the peer-to-peer Internet
===================================

This specification assumes that the Internet is transparantly routed,
without any built-in firewalls, but endpoints are assumed to be protected
by zero of more firewalls::

   local       local firewalls
   endpoint   +-----+   +-----+          /
   ||---------| fw1 |---| fw2 |---------< Internet
              +-----+   +-----+          \

Where this specification speaks of the local endpoint, it represents
the viewpoint of a single endpoint at a time; the same does however
apply from the viewpoint of the other endpoints.  References to the
local endpoint and the local firewalls represent the firewalls that
protect the local side against unwanted access from the Internet.  It
is assumed in this specification, that the local firewalls are setup
to enable outgoing initiatives, and welcome continued responses from the Internet.
This is common for the majority of firewalls on today's market.

Where this specification speaks of a remote endpoint and remote
firewalls, it represents the opposite perspective of the same
diagram, but from the viewpoint of entering from the Internet::

                      remote firewalls      remote
           \          +-----+   +-----+   endpoint
   Internet >---------| fw3 |---| fw4 |---------||
           /          +-----+   +-----+

Here, zero or more remote firewalls are assumed to protect the
remote endpoint.  The assumption made in this specification is that
this protection will default to blocking or rejecting
initiatives from the Internet, but to accept such traffic if it is a continuation of
traffic that was initiated by the remote endpoint.  This is common
for the majority of today's firewalls.  It is possible that a
firewall has explicit holes made for access to servers, which
should be of no consequence to this specification.

The total path between a local endpoint and a remote endpoint
is composed of the local firewalls, the Internet and
finally the remote firewalls.  In case of the images drawn, the
nodes on the path would be local endpoint, fw1, fw2, Internet, fw3, fw4,
remote endpoint.

Note that there may be situations where there are multiple
remote endpoints to one local endpoint; such can be the case with
multicast protocols.  Since this specification discusses only UDP
and TCP, that situation could only apply to UDP.

Where this specification mentions the Hop Limit, which is a
term specific to IPv6, it may also be read as the Time To Live,
which is the field with the similar function in IPv4.  Similarly,
wherever ICMP is mentioned, it can be read to mean ICMPv6
or ICMPv4, whichever applies.


Punching holes into local firewalls
===================================

To create a reliable peer-to-peer connection, it is necessary to open
up the local firewall for incoming access.  This can be done by
initiating an outgoing connection, but in such a way that the remote
firewalls are never reached.  This is accomplished by setting the
Hop Limit to a value that makes the packet drop somewhere on the
Internet, between the local and remote firewalls.

The remote firewall can be at various distinces from the local endpoint,
but for a given outgoing route, the local firewall is usually at a fixed distance.
More accurately put, the distance from any endpoint to the Internet is
normally a fixed value.  This means that setting the Hop Limit
to a predetermined value is a realistic possibility.

Once an outgoing packet has been sent with a proper Hop Limit value,
the local firewalls will assume that the connection is open, whereas
remote firewalls have not been able to reject it.  The routing
infrastructure should send back an ICMP message indicating that the
Hop Limit has been reached.  But as indiciated by REQ-12 in RFC 4787
for UDP and by REQ-10 in RFC 5382 for TCP, such ICMP messages should
not lead to closing the firewall hole in the local router.
These RFCs were written with Network Address Translation (NAT) in
mind, but it seems reasonable to assume that a firewall on its
own should not restrict things that it should permit when combined
with NAT.

When the ICMP message about the Hop Limit returns to the initiating
local endpoint, it will be in an unconnected state, and prone to
ignoring ICMP messages.


Punching holes for UDP
----------------------

To punch a hole for UDP into local firewalls, the simplest packet
possible is to send out a UDP packet with a minimal length.  According
to RFC 768, the minimal length can be 0, or in terms of the length
field in the UDP header, a value 8 octets.

UDP has no mechanism to actively reject or reset connections, because
it is not a connection-oriented protocol.  What this means is that
it would be possible to send a packet with a Hop Limit that makes it
reach the remote firewalls.  However, if the remote firewalls were
punched open by the remote endpoint, this would lead to the arrival
of this (empty) UDP packet of the remote endpoint, leading to the
processing of the packet.  This incurs a change in protocol semantics, which
is not generally a good idea.  So even with UDP, a suitable Hop Limit
should be used.

UDP is generally easy for peer-to-peer connectivity, because it is
no violation of the protocol to drop initial packets if they arrive at a closed
remote firewall.  But an improved effort to receive packets is
better, so it is generally good to punch a hole into UDP before
starting the actual transmissions.  As an added benefit, the
protocol that follows could even be unidirectional.


Punching holes for TCP
----------------------

The technique of punching a hole in the local firewalls for TCP is to send an early SYN
packet out through these local firewalls, and have it reach
its Hop Limit before it reaches the remote firewall.  What this
accomplishes is a bit complicated, but it does not violate any
standards and is therefore a reasonable expectation from any
firewall.

After this hole has been punched, the local firewall thinks that
a SYN has been sent to open the connection, and proper responses
would be SYN+ACK or SYN on its own.  The latter represents the
mechanism of Simultaneous TCP Open, shown in Figure 8 of
RFC 793.  It is furthermore worth noting that this mechanism is
mentioned explicitly in support of peer-to-peer connectivity in
RFC 5382 and RFC 5128.

The interesting aspect of the semantics of this technique however,
is that the local endpoint does not move to another state in its
TCP finite state machine.  It need not even be aware of this
initial message that was sent to punch the hole.  This is important,
because it means that the local endpoint could actually be a
passively listening TCP server socket.  It is important however,
that ICMP replies due to reaching the Hop Limit do not affect the
local endpoint either.

The only parts in the network that now believe that the connection
has been opened, are the local firewalls.  They will have created
a hole, reflecting that the connection is open.  Or more accurately
put, that it may be open -- the firewall has no way of knowing if
the SYN arrived on the remote end.  And that is why this approach
will actually work on the majority of firewalls.  The local endpoint
may send another SYN to actually connect.  And that would be a SYN
that does reach the remote firewalls which, in the procedures that
follow, will have opened up to the incoming SYN.


Connection procedures
=====================

Following are two distributed procedures for connecting two parties in a
peer-to-peer fashion.  At this level, the protocol does not
matter anymore, it could be either TCP or UDP.  Only when
observed at the levels of socket programming or packets
exchanged, would there be a noticeable difference.


Connecting in client/server mode
--------------------------------

In this mode of peer-to-peer connection, there is a clear
distinction between a passive and active party.  Just like
in the normal client/server connection mode, one party will
passively listen for incoming connections, while the other
actively connects to that party.  The only difference
is that the passive party punches a hole into its local
firewalls first.  To accomplish that, it uses locally
available knowledge of the optimal Hop Limit.

After the hole is punched, the active endpoint can proceed
to connect.  This will easily pass out its firewalls, and it
will find the pre-punched hole permitting its access to the
passive endpoint.  The passive endpoint continues to respond
as it would for any normal incoming traffic.

To make this approach work, it is important to first punch
the hole before announcing availability to the active side.
The external protocol that helps to build up a connection
can be supportive in this; otherwise, a few connection
attempts separated by exponential backoff timing on the
active part may work too.


Connecting in client/client mode
--------------------------------

This is the mode of operation that aims to use Simultaneous
TCP Open.  Even though this approach has a fair degree of success
under proper circumstances, it runs the principal risk of
contacting a remote firewall that resets the connection (for
TCP) or systematically drops packets (for UDP).

To solve this, it is a good idea to first punch holes in the
local firewalls on each end before actually starting the
exchange of traffic.  Again, the external protocol that
negotiates the connection endpoints can help.  If not, it is
also possible to accept the consequences of
unreliable delivery.  In practice, most protocols
either take unreliability into account by
repeating an attempt or, as is the case with RTP, be
sufficiently robust to handle temporary hickups in the
data stream.


Finding the ideal Hop Limit
===========================

The following subsections describe how to find the proper Hop Limit
value to send in messages intended for hole punching.  As the
setup of each site may differ, it is important that each endpoint
finds its own ideal Hop Limit, independent of what works for
other endpoints.


General concerns
----------------

Given a path from a local endpoint to a remote endpoint, there is
a range of possible Hop Limit values that work well.  This is the
series of values that make the packet be processed by the local
firewall but not reach the remote firewall.

Within this range, the ideal Hop Limit is the lowest value that
opens the local firewall for peer-to-peer traffic.  The lowest value is best because the chances of hitting upon
another remote firewall increase when the path is longer.  The
shortest path gives the best possible assurance that no remote
router would violate the attempted peer-to-peer connection.
Depending on the firewall's routing logic,
the ideal hole punching packet would leave with the
Hop Limit set to zero, or it would just not make it to leave the
firewall.

If the remote firewall is attached directly (that is, through
switches but not routers) to the local firewall then it may
still bounce off.  This however, is not the situation in
everyday network configurations.  In common configurations, the hole
punching packet passes through at least one router, which can send back
an ICMP message indicating that the Hop Limit has been reached.


Special cases for the Hop Limit
-------------------------------

The lowest possible Hop Limit that could work is 0.  This is
not a value that marks a non-action; if that setting is desired
than -1 could be used.  The value 0 indicates that a Hop Limit
of 0 would get the work done.  This is usable when the only
local firewall is directly connected to the same link as the
local endpoint, and if the local firewall commits to hole punching
before working on the Hop Limit.

The highest possible Hop Limit is half the diameter of the
Internet.  The usual assumption of this diameter is the value
transmitted as the Hop Limit for normal network traffic;
the hole punching should use a Hop Limit no more than half
that value.  This is because it is unreasonable to have a
local firewalling setup reach further into the link than half
the diameter of the Internet.

As a result, acceptable ranges for the Hop Limit may be as
low as 0 and as high as half the diameter of the Internet.
Until any further knowledge is learnt, these are the extremes that should be
taking into consideration by endpoints.


Incremental learning
--------------------

It is possible to set a certain initial value for the Hop Limit,
and change it as needed.  Generally, when peer-to-peer connectivity
fails on account of the local firewalls, the Hop Limit should be
incremented.  And if a TCP connection is reset, then clearly the
Hop Limit should be lowered.

It is however difficult to know when the local firewall is the cause of
failed peer-to-peer connectivity, as it often involves actions
by the human end user.  On the other hand, a TCP connection reset
can easily be reconstructed a number of times, until it is not
replied anymore.  A binary search would probably be optimal.

This effectively means that a system that implements Incremental
Learning starts with a Hop Limit set to maximum, so half the normal
Hop Limit.  Whenever a TCP connection reset comes in as a response
to a hole punching packet, the sent Hop Limit value minus one is
clearly the maximum permissible Hop Limit to use.

After a connection reset, another attempt can be made to punch
a hole in the local firewall.  Various mechanisms can be used,
including binary search or using hints from the Hop Limit returned
in the TCP connection reset message.

The distance is not the same for all remote firewalls.  The ones
nearest to the local firewalls are going to influence the Hop Limit
most.  Since this is an incremental learning process that closes in
on the local firewall configuration, it will converge to a value
that is low enough.

Effectively, this procedure learns to lower to end of the range
of acceptable Hop Limits.  It does not learn the beginning of
that range.  To avoid learning the same Hop Limit over and over again, a host should store
the values learnt as a route-dependent maximum Hop Limit.  Note that
having a higher Hop Limit than the ideal value does not harm this
approach because it will learn from the situations that would otherwise
pose problems.


Explicit learning procedure
---------------------------

If a second route to the Internet is available, for instance through
a tunnelling mechanism or any external protocol that connects back
through the local firewall, it is possible to explicitly learn the
ideal Hop Limit value.

The procedure starts before any hole punching command has been sent.
The second route is then used to contact a local endpoint.  This
should fail if a local firewall is present.  A successful connection
may be the accidental result of an older connection, so it is good
to confirm the non-existence of a local firewall hole by attempting
other ports as well.

When the connection over the second route consistently fails, the local endpoint
can start to send out hole punching messages, and trying to connect
afterwards.  Note that a hole punching messages should lead to an
ICMP message on the Hop Limit, if not a few resends are advisable.
When it can be safely assumed that the hole punching packet has
reached as far as it can go, the second route can be used to attempt
to send traffic through the local firewalls.  If this fails, the local firewalls have
not all been opened; if it succeeds, the hole has been punched and
the Hop Limit used suffices; if the packet drops, a few re-sends are
advisable, until it can be safely assumed that this is a failed
connetion attempt.  In case of a failed connection attempt, the
Hop Limit can be incremented and another attempt made.

The value learnt by this procedure is the lower end of the range
of acceptable Hop Limits.  Since this equals the ideal Hop Limit
value, this procedure is preferred over Incremental Learning.
As before, it is a good idea to store this
information and share it with as many participants of the same
route as possible.  For practical reasons,
it may only be possible to share the value within a host.


Dealing with alternate routing paths
------------------------------------

Some networks use a backup uplink to be used if their primary uplink fails.
In this case, the Hop Limit to use could change.  It is for this
reason that manual overrides should be allowed.  This effectively
makes the network behave slightly less than ideal, for the
overriding reason that it should need to learn a minimum of new
things after a transparant switch of routes.

In theory, this could lead to problems when a remote firewall is
nearby; in practice this will hardly ever happen.  If it does, there
will be a clear sign of that problem, namely an empty range of
acceptable Hop Limits.  If that happens, an overruling policy or
a request for manual intervention is still possible.


Manual setting
--------------

It is always possible to manually correct the value of the Hop Limit.
Given the possibility of alternate routes, it is probably best to
do this by changing the beginning and end of the range of acceptable
Hop Limit values.

Since the second route necessary for learning the lower end of this
range is not always available, it is especially useful if an
operator can set the minimum acceptable Hop Limit, and have it used as
the actual Hop Limit on outgoing traffic.  If ever this leads to a
connection reset, the current uplink must be supportive of a lower
Hop Limit, which can then be used only for that connection attempt.


Extending the Socket API
========================

The packets that punch holes can be constructed in a number of ways,
including access to the raw sockets of RFC 2292.  In
practice however, this mechanism is too general, and break the
security model of an application if it requires a higher
privilege level.  Furthermore, when using raw sockets it is not possible to retrieve the
window counter that ought to go into a well-formed TCP SYN packet.

For these reasons, the best approach in support of hole punching is
an extension to the Application Programmer's Interface (API) for
sockets.  The extension takes the shape of a socket call that
constructs and sends a packet to punch a hole in the local firewall.

The descriptions below do not describe in detail how connection resets
are picked up; usually this will show up as an error, and it may take
the external protocol that setup the peer-to-peer exchange to determine
whether this was the result of a firewall problem.  As this external
protocol is not discussed in this specification, such forms of handling
must also be dealt with externally.


A new Socket Call
-----------------

The following operation should be called to send a single, unconfirmed
message to open a hole in local firewalls::

  int synergy (int fdsock,
                        struct sockaddr *tgt, socklen_t tgtlen,
                        int hoplimit)

The operation returns 0 on success.  If it fails, it returns -1 and
sets ``errno`` to a descriptive value.  Unlike full access to a raw
socket, this operation requires no special permissions, other than
access to the socket for the local endpoint.

The ``fdsock`` parameter is the socket for the local endpoint.  This
socket must already be setup with at least a local address and port,
and it must be either a ``SOCK_STREAM`` type socket for TCP, or a
``SOCK_DGRAM`` socket for UDP.  The address family should be either
``AF_INET6`` or ``AF_INET``.  The implementation of ``AF_INET6`` is
required; the implementation of ``AF_INET`` is advised in support of
IPv4 hosts that have globally routeable addresses assigned.

The parameters ``tgt`` and ``tgtlen`` describe the remote party
that should be set as the target for the hole punching packet, which
is likely to end up the firewall's connection tracker.

The ``hoplimit`` parameter is the value to be sent as the Hop Limit
in the outgoing packet.

Finally, it is worth mentioning that multiple synergy() calls
can be made on a single socket.  For server sockets, this could
be used to punch holes in the firewall for multiple remote peers.
All sockets could profit from the ability to resend synergy()
packets, as their arrival is not guaranteed and the firewall
usually sets a limited time on their follow-up.


Timing of Address Exchange
--------------------------

Both endpoints must be known before the synergy() socket call can be
made.  This is a direct result of what the local firewall will
need to store, namely a connection between an internal address
and port and a remote address and port.

What this implies is that the external protocol (such as SDP)
that exchanges the peer-to-peer endpoint addresses
and ports, must have communicated the remote endpoint to the
local endpoint before this call is possible.  Since the initiative
must start somewhere, that same protocol may need to communicate
the local endpoint before this call is being made.  If possible
however, the ideal is to first punch the hole and then to pass
on the information on the local endpoint.

The certain way to avoid any chances of reset connections is to punch
a hole in the firewall on the server side before communicating
its address to the client side.  In other situations, a few
attempts may be needed to initiate the connection.  It is
particularly interesting that this approach to peer-to-peer
communication does not call for Simultaneous TCP Open,
but instead uses its existence only to get
support from the local firewall.


Setting up a TCP Client Socket
------------------------------

The sequence of operations to create a client-side TCP socket is:

#. socket(...)
#. bind(...)
#. synergy(...)
#. connect(...)

The call to bind() precedes synergy() because there must be a local
address and port to punch a hole from.  The synergy() call must be
supplied with a remote address and port to complete the pair that the
firewall should open.

The call to synergy() adds nothing to the
active side of a client/server connection, so
this sequence would primarily be used under client/client operation, where
both parties perform this procedure, but with mirrorred endpoints.
To facilitate immediate acceptance of the connect() it is good to use
the external protocol that exchanges the endpoints as a source of
synchronisation.  If that ends up being a recursive problem, it is possible to wait between synergy() and
connect() for at least the maximum expected communication delay between
the peers, so it can be assumed that both ends have performed their
synergy() call.  After that, the connect() should not be at risk of
being reset.


Setting up a TCP Server Socket
------------------------------

The sequence of operations to create a server-side TCP socket is:

#. socket(...)
#. bind(...)
#. listen(...)
#. synergy(...)
#. accept(...)

This is used on the passive side of client/server connections.  It is
required to invoke synergy() after bind() because that defines its
locally listening address and port; calling synergy() after listen()
means that the port is already setup to accept incoming connections
before the hole in the firewall is punched.  Since accept() is a
blocking call, it has to be called after synergy() because the
latter would otherwise be issued too late.

Ideally, the synergy() call would be made before submitting the local
endpoint address and port to any peers; this may not always be possible,
given the external protocol that exchanges the endpoint address and port.
If that is the case, a bit of
waiting and possibly a few retries on the active side can help to
resolve this problem.  Other than this, the remote end can be a regular,
initiation of a connection.


Setting up a UDP Peer Socket
----------------------------

The sequence of operations to create a client-side UDP socket would be:

#. socket(...)
#. bind(...)
#. synergy(...)
#. connect(...)

As before, the local endpoint must be known before synergy() can be
called, so a bind() should precede it.
UDP is not connection-oriented, but connect() is the common method
to set a single communication partner, as will be the case in most
of its application for peer-to-peer purposes.  Nevertheless, it is
also possible to use multiple peers in sendto() calls, because it
is possible to initiate multiple synergy() calls, even after
sendto() calls have taken place, to welcome additional peers.

The local firewall can infer no semantics from the general UDP
protocol, so it will always rely on timeouts to close the hole that
was punched.  What that means in practice, is that the hole must
be actively kept open.  This can be done by regular sending of
traffic, or by sending occasional synergy() calls.  The advantage
of the latter is that it is a general mechanism that imposes no
constraints on the protocol itself.

As for receiving traffic on a UDP socket, this is possible with
the usual mechanisms, namely the recv() or recvfrom() calls.  The
listen() mechanism can be used to decide whether the next action
is a send() or recv(), if the UDP-carried protocol does not
define that.


Privacy Issues
==============

This protocol relies on directly addressable endpoints, which are
shared with communications partners.  In that sense, they reveal
more information than communication based on NAT would.  This is
in line with the general intention of the IETF to have a transparant Internet.
Furthermore, the information is only submitted
to communication peers, which the external protocol exchanging them
will usually do on a need-to-know basis only.


Security Issues
===============

This specification does not describe any new protocols, merely a way
of using existing ones.  Still, this is a suitable place for discussing
possible repercussions of the techniques in the area of security.


Man in the middle
-----------------

The exchange of endpoints is assumed to take place over an external
protocol, making it prone to all the problems involved in that
protocol.  For instance, if the external protocol is SIP/SDP, there
is the possibility for an intermediate to play the role of a man
(usually named Eve) in the middle.  These issues should be dealt
with either by this outside protocol or by protecting the protocol
carried between peers.  If neither can provide such protection, it
is also possible to rely on IPsec, but that could put a serious
restriction on the number of successful interactions.


Local malware
-------------

Opening up local services for external parties may be an interesting
facility for a large variety of malware.  Using this technique, it
might be possible to initiate a denial of service attack that was
agreed upon between peers, without any traceable reference to a central server.

One line of defence against this is the short time that a firewall
will keep a connection attempt open; this is assumed to range from
half a minute to an hour at most.  The reason why a firewall will
want to limit such attempts, aside from security, is lack of
resources.  There is a maximum to the number of connection attempts
that it can have open at any time.  This means that it is not
possible to simply open a port to a piece of malware to the entire
Internet.

Especially if a symmetric firewall is used, which stores both
endpoints in a connection, the problems are limited.  An attempt
at contacting a local piece of malware over a hole in a symmetric
firewall depends on guessing two addresses correctly within the
timespan available to the hole.

Guessing a single address might
not even be that hard.  Even in case of an IPv6 address, this could be
a practical attack::

  +-------+-------+-------+-------+-------+-------+-------+-------+
  | prefix|  assignment   |  SLA  |      interface identifier     |
  +-------+-------+-------+-------+-------+-------+-------+-------+

1. The prefix of an address appears to mostly be 2001 or 2a01; so for
   all practical purposes, an attacker sees only 1 bit of entropy in
   the prefix.

2. The assignment to a /48 is free-form.  However, any hit is welcomed.
   If a piece of malware reaches a coverage of 25%, for example because
   it is installed automatically onto a widely adopted OS, the assignment to
   a /48 would only add 2 bits of entropy.  This is definately assuming
   a high degree of infiltration of the malware, and it would be very
   difficult to get there.

3. The SLA is determined based on a limited number of patterns.  The
   factors determining it are routers, culture and ease-of-use.  For
   local and SOHO users, which form the most interesting attack
   vector, it is very often going to be 0 or 1, so the SLA accounts for 1 bit of entropy.

4. Autoconfiguration and DHCPv6 are easy to influence, and a value for the interface identifier may be
   chosen at will by any machine.  Depending on the degree of integration of
   the malware into the host OS, it could actually set a value of its
   own choosing.  This may be a fixed value, or one that could be
   obfuscated with the aid of the other address parts.  The entropy
   would be 0.

5. The protocol to use is a free choice by the malware, so whether UDP
   or TCP is used, or perhaps another protocol, amounts 0 bits to the
   entropy to beat.

6. Add to this a port number.  This too can always be picked at will by
   the malware.  The entropy added is therefore 0 bits.

The total entropy in a single address would therefore be only 1+2+1+0+0+0=4 bits
for an endpoint!  This gives a piece of malware a good chance
of communicating with peers.  This is not a result of this document, it
is merely explained by it.

When a firewall is symmetrical, the chances are better.  For those, a
combination of address/port pairs must find each other to setup a
successful communication.  This means that the incoming side poses
a limit as a result of the number of simultaneous incoming connections
it permits.  Specifically the bits of the assignment now count fully,
making the entropy of the local endpoint 1+32+1+0+0+0=34 bits.  The
combined entropy of the two endpoints now is 34+4=38 bits.  Liberally assuming
that the firewall can hold a million connections open at any time,
subtract 20 bits and arrive at a hit rate of 1 in 2^18 or 1 in 262144.
This still is not very high, and may well be worked around with tricks like
distributed time-synchronised attempts between the peering malware.

Note however, that any such behaviour is easily recognised by
excessive numbers of Hop Limit reached messages arriving back at
the router, or being generated just before passing a message out.
A high number of such Hop Limit reached meassages is clearly
indicative of a problem.  Note that such messages are sent back
to the originating host, which means that they can easily be
traced back to the offending host on the network.

This situation is not very new; UDP has had this ability with
all firewalls in common use.  In general, if internal software cannot
be trusted to behave properly, the policy of letting internal software
go out and build arbitrary connections is not wise.  The conclusion should not be that
peer-to-peer connections als malicuous, expecially because they are actually just a
partial recovery of the transparant nature for which the Internet was
designed, and which is required to make it work as well as it does.
The true lesson here is that internal control over software is very
important and is a responsibility of all users that wish to remain
active on the Internet.


Acknowledgements
================

The author is grateful to Dutch performing artists for their inspiration.
Their art and culture help to think out of the box and find generally useful
technical innovations like these.

This work was partially funded by the WBSO program of the Dutch ministry
of Economic Affairs, Agriculture and Innovation.


