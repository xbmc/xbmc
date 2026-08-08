/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkFreebsd.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <array>
#include <errno.h>
#include <set>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <resolv.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <sys/wait.h>
#include <unistd.h>

CNetworkInterfaceFreebsd::CNetworkInterfaceFreebsd(CNetworkPosix* network,
                                                   std::string interfaceName,
                                                   char interfaceMacAddrRaw[6])
  : CNetworkInterfacePosix(network, interfaceName, interfaceMacAddrRaw)
{
}

std::string CNetworkInterfaceFreebsd::GetCurrentDefaultGateway() const
{
  std::string result;

  size_t needed;
  int mib[6];
  char *buf, *next, *lim;
  char line[16];
  struct rt_msghdr* rtm;
  struct sockaddr* sa;
  struct sockaddr_in* sockin;

  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = 0;
  mib[4] = NET_RT_DUMP;
  mib[5] = 0;
  if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
    return result;

  if ((buf = (char*)malloc(needed)) == NULL)
    return result;

  if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
  {
    free(buf);
    return result;
  }

  lim = buf + needed;
  for (next = buf; next < lim; next += rtm->rtm_msglen)
  {
    rtm = (struct rt_msghdr*)next;
    sa = (struct sockaddr*)(rtm + 1);
    sa = (struct sockaddr*)(SA_SIZE(sa) + (char*)sa);
    sockin = (struct sockaddr_in*)sa;
    if (inet_ntop(AF_INET, &sockin->sin_addr.s_addr, line, sizeof(line)) == NULL)
    {
      free(buf);
      return result;
    }
    result = line;
    break;
  }
  free(buf);

  return result;
}

// Walk the kernel routing table (via sysctl net.route dump) and return the
// next-hop gateway of the IPv6 default route (destination ::/0).
std::string CNetworkInterfaceFreebsd::GetCurrentIPv6DefaultGateway() const
{
  std::string gateway;
  int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET6, NET_RT_FLAGS, RTF_GATEWAY};

  size_t needed = 0;
  if (sysctl(mib, sizeof(mib) / sizeof(int), nullptr, &needed, nullptr, 0) < 0)
    return gateway;

  char* buf = (char*)malloc(needed);
  if (buf == nullptr)
    return gateway;

  if (sysctl(mib, sizeof(mib) / sizeof(int), buf, &needed, nullptr, 0) < 0)
  {
    free(buf);
    return gateway;
  }

  struct rt_msghdr* rtm;
  for (char* next = buf; next < buf + needed; next += rtm->rtm_msglen)
  {
    rtm = reinterpret_cast<struct rt_msghdr*>(next);
    struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(rtm + 1);
    struct sockaddr* sa_tab[RTAX_MAX] = {};
    for (int i = 0; i < RTAX_MAX; i++)
    {
      if (rtm->rtm_addrs & (1 << i))
      {
        sa_tab[i] = sa;
        sa = reinterpret_cast<struct sockaddr*>(reinterpret_cast<char*>(sa) + SA_SIZE(sa));
      }
    }

    if ((rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY) ||
        sa_tab[RTAX_DST]->sa_family != AF_INET6 || sa_tab[RTAX_GATEWAY]->sa_family != AF_INET6)
      continue;

    // default route has an unspecified (::) destination
    if (!IN6_IS_ADDR_UNSPECIFIED(
            &(reinterpret_cast<struct sockaddr_in6*>(sa_tab[RTAX_DST]))->sin6_addr))
      continue;

    struct in6_addr gwAddr =
        (reinterpret_cast<struct sockaddr_in6*>(sa_tab[RTAX_GATEWAY]))->sin6_addr;

    // A default gateway is almost always link-local. FreeBSD (KAME) embeds the
    // scope (interface index) in bytes 2-3 of a link-local address rather than
    // in sin6_scope_id; clear it so inet_ntop renders "fe80::1" and not
    // "fe80:4::1".
    if (IN6_IS_ADDR_LINKLOCAL(&gwAddr))
      gwAddr.s6_addr[2] = gwAddr.s6_addr[3] = 0;

    char dstStr6[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &gwAddr, dstStr6, INET6_ADDRSTRLEN) != nullptr)
      gateway = dstStr6;
    break;
  }

  free(buf);

  return gateway;
}

// Some IN6_IFF_* address flags may be absent depending on the FreeBSD release;
// provide fallbacks so the ranking compiles regardless.
#ifndef IN6_IFF_ANYCAST
#define IN6_IFF_ANYCAST 0x01
#endif
#ifndef IN6_IFF_TENTATIVE
#define IN6_IFF_TENTATIVE 0x02
#endif
#ifndef IN6_IFF_DUPLICATED
#define IN6_IFF_DUPLICATED 0x04
#endif
#ifndef IN6_IFF_DETACHED
#define IN6_IFF_DETACHED 0x08
#endif
#ifndef IN6_IFF_DEPRECATED
#define IN6_IFF_DEPRECATED 0x10
#endif
#ifndef IN6_IFF_AUTOCONF
#define IN6_IFF_AUTOCONF 0x40
#endif
#ifndef IN6_IFF_TEMPORARY
#define IN6_IFF_TEMPORARY 0x80
#endif

// Gets the highest-ranked permanent IPv6 address on the current interface.
// getifaddrs does not expose per-address state, so each candidate's flags are
// queried with SIOCGIFAFLAG_IN6 to filter out temporary/deprecated addresses
// and prefer stable ones, mirroring the Linux/macOS behaviour.
std::string CNetworkInterfaceFreebsd::GetCurrentIPv6Address() const
{
  std::string address;

  struct ifaddrs* interfaces = nullptr;
  if (getifaddrs(&interfaces) != 0)
    return address;

  // SIOCGIFAFLAG_IN6 needs an AF_INET6 socket
  int sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    freeifaddrs(interfaces);
    return address;
  }

  unsigned int bestRank = 0;

  for (struct ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next)
  {
    if (iface->ifa_name != m_interfaceName ||
        (iface->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING) ||
        iface->ifa_addr == nullptr || iface->ifa_addr->sa_family != AF_INET6)
      continue;

    const struct sockaddr_in6* addr6 =
        reinterpret_cast<const struct sockaddr_in6*>(iface->ifa_addr);

    // Only consider globally scoped addresses; skip link-local (fe80::/10)
    // and loopback (::1).
    if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr) || IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr))
      continue;

    // Query the address-specific flags.
    struct in6_ifreq ifr6 = {};
    strncpy(ifr6.ifr_name, iface->ifa_name, sizeof(ifr6.ifr_name) - 1);
    memcpy(&ifr6.ifr_ifru.ifru_addr, addr6, sizeof(struct sockaddr_in6));

    if (ioctl(sock, SIOCGIFAFLAG_IN6, &ifr6) < 0)
      continue;

    const int flags = ifr6.ifr_ifru.ifru_flags6;

    // Remove anything that is not a usable permanent address. FreeBSD has no
    // optimistic-DAD flag, so a tentative address is never usable.
    if (flags & (IN6_IFF_DEPRECATED | IN6_IFF_DUPLICATED | IN6_IFF_TEMPORARY | IN6_IFF_TENTATIVE |
                 IN6_IFF_DETACHED | IN6_IFF_ANYCAST))
      continue;

    // Rank the remaining permanent candidates (highest wins).
    // !AUTOCONF: manually configured or DHCPv6 — explicitly assigned
    // AUTOCONF:  SLAAC (EUI-64, or the stable source of temporaries)
    unsigned int rank = (flags & IN6_IFF_AUTOCONF) ? 1 : 2;

    // Keep the highest rank.
    if (rank <= bestRank)
      continue;

    char str6[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &addr6->sin6_addr, str6, sizeof(str6)) == nullptr)
      continue;

    address = str6;
    bestRank = rank;
  }

  close(sock);
  freeifaddrs(interfaces);

  return address;
}

bool CNetworkInterfaceFreebsd::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  bool ret = false;
  size_t needed;
  char *buf, *next;
  struct rt_msghdr* rtm;
  struct sockaddr_inarp* sin;
  struct sockaddr_dl* sdl;
  constexpr std::array<int, 6> mib = {
      CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO,
  };

  mac = "";

  if (sysctl(mib.data(), mib.size(), nullptr, &needed, nullptr, 0) == 0)
  {
    buf = (char*)malloc(needed);
    if (buf)
    {
      if (sysctl(mib.data(), mib.size(), buf, &needed, nullptr, 0) == 0)
      {
        for (next = buf; next < buf + needed; next += rtm->rtm_msglen)
        {

          rtm = (struct rt_msghdr*)next;
          sin = (struct sockaddr_inarp*)(rtm + 1);
          sdl = (struct sockaddr_dl*)(sin + 1);

          if (host_ip != sin->sin_addr.s_addr || sdl->sdl_alen < 6)
            continue;

          u_char* cp = (u_char*)LLADDR(sdl);

          mac = StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", cp[0], cp[1],
                                    cp[2], cp[3], cp[4], cp[5]);
          ret = true;
          break;
        }
      }
      free(buf);
    }
  }
  return ret;
}

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkFreebsd>();
}

CNetworkFreebsd::CNetworkFreebsd() : CNetworkPosix()
{
  queryInterfaceList();
}

void CNetworkFreebsd::GetMacAddress(const std::string& interfaceName, char rawMac[6])
{
  memset(rawMac, 0, 6);

#if !defined(IFT_ETHER)
#define IFT_ETHER 0x6 /* Ethernet CSMACD */
#endif
  const struct sockaddr_dl* dlAddr = NULL;
  const uint8_t* base = NULL;
  // Query the list of interfaces.
  struct ifaddrs* list;
  struct ifaddrs* interface;

  if (getifaddrs(&list) < 0)
  {
    return;
  }

  for (interface = list; interface != NULL; interface = interface->ifa_next)
  {
    if (interfaceName == interface->ifa_name)
    {
      if ((interface->ifa_addr->sa_family == AF_LINK) &&
          (((const struct sockaddr_dl*)interface->ifa_addr)->sdl_type == IFT_ETHER))
      {
        dlAddr = (const struct sockaddr_dl*)interface->ifa_addr;
        base = (const uint8_t*)&dlAddr->sdl_data[dlAddr->sdl_nlen];

        if (dlAddr->sdl_alen > 5)
        {
          memcpy(rawMac, base, 6);
        }
      }
      break;
    }
  }

  freeifaddrs(list);
}

void CNetworkFreebsd::queryInterfaceList()
{
  char macAddrRaw[6];
  m_interfaces.clear();

  // Query the list of interfaces.
  struct ifaddrs* list;
  if (getifaddrs(&list) < 0)
    return;

  // getifaddrs reports a separate entry per address, so an interface appears
  // once per bound address (and, on an IPv6-only host, never via an AF_INET
  // entry). Walk every entry and add each interface once, keyed by name, so
  // IPv6-only interfaces are enumerated too.
  std::set<std::string> added;
  for (struct ifaddrs* cur = list; cur != NULL; cur = cur->ifa_next)
  {
    if (cur->ifa_name == nullptr || !added.insert(cur->ifa_name).second)
      continue;

    GetMacAddress(cur->ifa_name, macAddrRaw);

    // only add interfaces with non-zero mac addresses
    if (macAddrRaw[0] || macAddrRaw[1] || macAddrRaw[2] || macAddrRaw[3] || macAddrRaw[4] ||
        macAddrRaw[5])
      // Add the interface.
      m_interfaces.push_back(new CNetworkInterfaceFreebsd(this, cur->ifa_name, macAddrRaw));
  }

  freeifaddrs(list);
}

std::vector<std::string> CNetworkFreebsd::GetNameServers()
{
  std::vector<std::string> result;

  res_init();

  for (int i = 0; i < _res.nscount; i++)
  {
    std::string ns = inet_ntoa(_res.nsaddr_list[i].sin_addr);
    result.push_back(ns);
  }

  return result;
}

std::vector<std::string> CNetworkFreebsd::GetIPv6NameServers()
{
  std::vector<std::string> result;

  res_init();

  // _res.nsaddr_list only holds the IPv4 servers; the resolver keeps any IPv6
  // servers in an extension array. res_getservers() returns both families.
  union res_sockaddr_union servers[MAXNS];
  int count = res_getservers(&_res, servers, MAXNS);

  for (int i = 0; i < count; i++)
  {
    if (servers[i].sin6.sin6_family != AF_INET6)
      continue;

    char buf[INET6_ADDRSTRLEN] = {};
    if (inet_ntop(AF_INET6, &servers[i].sin6.sin6_addr, buf, sizeof(buf)) != nullptr)
      result.emplace_back(buf);
  }

  return result;
}

bool CNetworkFreebsd::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  char cmd_line[64];

  struct in_addr host_ip;
  host_ip.s_addr = remote_ip;

  snprintf(cmd_line, sizeof(cmd_line), "ping -c 1 -t %d %s",
           timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));

  int status = -1;
  status = system(cmd_line);
  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = {}, errno = {} : '{}'", status, errno, cmd_line);

  return result == 0;
}
