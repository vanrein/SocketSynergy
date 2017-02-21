# README for SocketSynergy

> *SocketSynergy helps to connect peers behind firewalls.  It is a generic
> solution to a problem that most peer-to-peer applications run into.*

From: Rick van Rein <rick@openfortress.nl>


## Purpose

The intention of SocketSynergy is to allow peers to connect in spite of
*both* being behind a firewall.  This is traditionally considered
impossible, because always one of the firewalls blocks.  That is a problem
for peer-to-peer applications, but SocketSynergy makes it possible.  And
easy, all it takes is another call `synergy()` on top of the general
Socket API.

The assumptions made by SocketSynergy are

  * transparent addressing
  * firewalls following the general
    [BEHAVE WG](https://datatracker.ietf.org/wg/behave/charter/)
    specifications

Specifically note: There is no support for Network Address Translation.
If we want the network to function, we should rely on IPv6.  Do have a look
at the other elements in the
[P2P Toolkit](P2P-TOOLKIT.MD)
to learn how reasonable this assumption is.


## Quickstart demo

Following is a quick intro showing how to use the synergy() library
call and, perhaps one day, syscall.

Listen for a particular peer::

  ./listendemo tcp 2001:db8:420a:1::11 4444 2001:db8:420a:1::5 5555 3

This creates a TCP listening socket on the first address/port that
accepts incoming connections from the second address/port.  In fact
that is very close to what a tool like netcat6 can do.

However, the trick is in the last parameter, 3 in this case.  This is
the hoplimit for an initial SYN message sent out.  The idea is that the
value of the last parameter is the number of steps to get outside of
the last firewall shielding the local network from incoming initiative.
The SYN should be dropped as soon as it has crossed that last firewall,
so that all ports are open.  Meanwhile, the listendemo is a listening
socket, so it awaits incoming connections.  Punching the hole first
makes it accessible from the specified host, that's all.

It is now possible to connect from the specified peer, using some
standard tool like netcat6::

  nc6 -6    -s 2001:db8:420a:1::5 -p 5555 2001:db8:420a:1::11 4444

It is also possible to do the same things based on UDP.  Just give a
first argument ``udp`` to ``listendemo`` and add a ``-u`` option
to the ``nc6`` commandline.  Likewise, SCTP is possible.

Of course, with UDP there are no sharp
boundaries to the start and end of a session.  The trick used to
open the port is sending out a UDP packet that, once again, drops
as soon as it has crossed the last firewall protecting this domain.

The real trick here is knowing the number of hops to get to the
big, bad Internet.  This should not be taken too low (as it would
not punch the desired hole) and not too high (as some nearby
remote firewalls might receive it and respond with RST, causing
the incoming firewalls to close the just-punched hole).


## Code reference

This code was written based on RFC 2292, "Advanced Sockets API for
IPv6", by W. Stevens and M. Thomas.

