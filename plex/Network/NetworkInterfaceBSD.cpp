/*
 * Written and (c) 2014 by Ben Woods
 * Licensed under the two-clause (new) BSD license.
 * Some code copied from Matthias Andree's try-rtsock.c:
 *  http://people.freebsd.org/~mandree/try-rtsock.c
 */

#ifdef __FreeBSD__
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <net/if.h>
#include <net/route.h>

// only for decoding AF_LINK addresses:
#include <net/if_dl.h>

#include <netdb.h>

#include <string.h>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "NetworkInterface.h"

using namespace boost;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Message agnostic receive buffer
union u {
    char buf[1024];
    struct if_msghdr ifm;
    struct ifa_msghdr ifam;
    struct if_announcemsghdr ifann;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
static void PrintRouteMsg(const union u *buff, size_t len)
{
  dprintf("NetworkInterface: PF_ROUTE socket received message. Version %d, Type %#x, Len %d.", buff->ifm.ifm_version, buff->ifm.ifm_type, buff->ifm.ifm_msglen);

  switch (buff->ifm.ifm_type)
  {
    case RTM_NEWADDR:
    case RTM_DELADDR:
    dprintf("NetworkInterface: ##%s## - Addrmask %#x, Flags %#x, Index %hu, Metric %d",
      buff->ifm.ifm_type == RTM_NEWADDR ? " NEW ADDRESS " : " DELETE ADDR ",
      buff->ifam.ifam_addrs,
      buff->ifam.ifam_flags,
      buff->ifam.ifam_index,
      buff->ifam.ifam_metric);
    break;

    case RTM_IFINFO:
    dprintf("  INFO - Addrmask %#x, Index %hu, Flags %#x:",
      buff->ifm.ifm_addrs, buff->ifm.ifm_index, buff->ifm.ifm_flags);
    switch (buff->ifm.ifm_flags)
    {
      case IFF_UP: dprintf("UP"); break;
      case IFF_BROADCAST: dprintf("bcast_valid"); break;
      case IFF_LOOPBACK: dprintf("loopback"); break;
      case IFF_POINTOPOINT: dprintf("P2P"); break;
      case IFF_DRV_RUNNING: dprintf("running"); break;
      case IFF_NOARP: dprintf("noARP"); break;
      case IFF_PROMISC: dprintf("promisc"); break;
      case IFF_DYING: dprintf("dying"); break;
      case IFF_RENAMING: dprintf("renaming"); break;
    }

    case RTM_IFANNOUNCE:
    dprintf("  ANNOUNCE iface %.*s index %hu",
      IFNAMSIZ, buff->ifann.ifan_name, buff->ifann.ifan_index);
    switch(buff->ifann.ifan_what)
    {
      case IFAN_ARRIVAL: dprintf(" ARRIVED"); break;
      case IFAN_DEPARTURE: dprintf(" DEPARTED"); break;
      default: dprintf("Unknown action %hu", buff->ifann.ifan_what);
    }
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkChanged()
{
  dprintf("Network change.");
  NetworkInterface::NotifyOfNetworkChange();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void RunWatchingForChanges()
{
  dprintf("NetworkInterface: Watching for changes on the interfaces.");
  
  // Create the socket that's going to watch for interface changes, and make it non-blocking.
  int sock = socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC); /* AF_UNSPEC: all addr families */
  if (sock == -1)
    eprintf("Error creating PF_ROUTE socket: %d", errno);

  // Add socket to null initialised file descriptor set
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);

  // Now sit in a loop waiting for messages.
  int ret1;
  while ((ret1 = select(sock+1, &fds, 0, 0, 0)), 1)
  {
    if (ret1 == -1)
    {
      eprintf("NetworkInterface: PF_ROUTE socket select error (%d).", errno);
      continue;
    }

    // Message waiting
    if (ret1 > 0 && FD_ISSET(sock, &fds))
    {
      // Setup message agnostic receive buffer
      union u buff;
      buff.ifm.ifm_msglen = 4;

      // Receive messages into buffer
      int ret2 = recv(sock, &buff, sizeof(buff), 0);
      if (ret2 == -1)
      {
        eprintf("NetworkInterface: PF_ROUTE socket receive error (%d).", errno);
        continue;
      }

      // Read through messages and determine if any indicate that any interface records should be rebuilt.
      if (ret2 < 4 || ret2 < buff.ifm.ifm_msglen)
      {
        eprintf("NetworkInterface: PF_ROUTE socket short read (have %d want %hu), skipping.", ret2, buff.ifm.ifm_msglen);
        continue;
      }

      if (buff.ifm.ifm_version != RTM_VERSION)
      {
        eprintf("NetworkInterface: PF_ROUTE socket unknown message version %d, skipping.", buff.ifm.ifm_version);
        continue;
      }

      // Dump the message.
      PrintRouteMsg(&buff, ret2);

      // See if something notable changed.
      if (buff.ifm.ifm_type == RTM_IFINFO || buff.ifm.ifm_type == RTM_IFANNOUNCE ||
          buff.ifm.ifm_type == RTM_DELADDR || buff.ifm.ifm_type == RTM_NEWADDR)
      {
        // Notify about it.
        NetworkChanged();
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkInterface::WatchForChanges()
{
  // Start the thread.
  dprintf("NetworkInterface: Starting watch thread.");
  thread t = thread(boost::bind(&RunWatchingForChanges));
  t.detach();
  
  // Start with a change, because otherwise we're in steady state.
  NetworkChanged();
}

#endif
