/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkMacOS.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <array>
#include <errno.h>
#include <set>

#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet6/in6_var.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <unistd.h>

CNetworkInterfaceMacOS::CNetworkInterfaceMacOS(CNetworkPosix* network,
                                               const std::string& interfaceName,
                                               char interfaceMacAddrRaw[6])
  : CNetworkInterfacePosix(network, interfaceName, interfaceMacAddrRaw)
{
}

std::string CNetworkInterfaceMacOS::GetCurrentDefaultGateway() const
{
  return GetGateway(AF_INET);
}

std::string CNetworkInterfaceMacOS::GetCurrentIPv6DefaultGateway() const
{
  return GetGateway(AF_INET6);
}

// Walk the kernel routing table (via sysctl net.route dump) and return the
// next-hop gateway of the default route (destination ::/0 or 0.0.0.0/0) for
// the requested address family.
std::string CNetworkInterfaceMacOS::GetGateway(int family) const
{
  std::string gateway;
  int mib[] = {CTL_NET, PF_ROUTE, 0, family, NET_RT_FLAGS, RTF_GATEWAY};

  size_t needed = 0;
  if (sysctl(mib, sizeof(mib) / sizeof(int), nullptr, &needed, nullptr, 0) < 0)
    return gateway;

  char* buf = new char[needed];

  if (sysctl(mib, sizeof(mib) / sizeof(int), buf, &needed, nullptr, 0) < 0)
  {
    CLog::Log(LOGERROR, "sysctl: net.route.0.0.dump");
    delete[] buf;
    return gateway;
  }

  struct rt_msghdr* rt;
  for (char* p = buf; p < buf + needed; p += rt->rtm_msglen)
  {
    rt = reinterpret_cast<struct rt_msghdr*>(p);
    struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(rt + 1);
    struct sockaddr* sa_tab[RTAX_MAX];
    for (int i = 0; i < RTAX_MAX; i++)
    {
      if (rt->rtm_addrs & (1 << i))
      {
        sa_tab[i] = sa;
        // Routing-socket sockaddrs are padded to sizeof(uint32_t) on Darwin
        // (not sizeof(long)); the latter overshoots 28-byte sockaddr_in6
        // entries and corrupts parsing of subsequent addresses.
        sa = reinterpret_cast<struct sockaddr*>(
            reinterpret_cast<char*>(sa) +
            ((sa->sa_len) > 0 ? (1 + (((sa->sa_len) - 1) | (sizeof(uint32_t) - 1)))
                              : sizeof(uint32_t)));
      }
      else
      {
        sa_tab[i] = nullptr;
      }
    }

    if (((rt->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) ||
        sa_tab[RTAX_DST]->sa_family != family || sa_tab[RTAX_GATEWAY]->sa_family != family)
      continue;

    if (family == AF_INET)
    {
      // default route has an all-zero (0.0.0.0) destination
      if ((reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_DST]))->sin_addr.s_addr != 0)
        continue;

      char dstStr4[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET,
                    &(reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_GATEWAY]))->sin_addr,
                    dstStr4, INET_ADDRSTRLEN) != nullptr)
        gateway = dstStr4;
      break;
    }
    else if (family == AF_INET6)
    {
      // default route has an unspecified (::) destination
      if (!IN6_IS_ADDR_UNSPECIFIED(
              &(reinterpret_cast<struct sockaddr_in6*>(sa_tab[RTAX_DST]))->sin6_addr))
        continue;

      struct in6_addr gwAddr =
          (reinterpret_cast<struct sockaddr_in6*>(sa_tab[RTAX_GATEWAY]))->sin6_addr;

      // Darwin embeds the scope (interface index) of a link-local address in
      // bytes 2-3 of the address itself (the KAME convention) rather than in
      // sin6_scope_id. A default gateway is almost always link-local, so clear
      // the embedded id first or inet_ntop renders it literally (e.g.
      // "fe80:4::1" instead of "fe80::1").
      if (IN6_IS_ADDR_LINKLOCAL(&gwAddr))
        gwAddr.s6_addr[2] = gwAddr.s6_addr[3] = 0;

      char dstStr6[INET6_ADDRSTRLEN];
      if (inet_ntop(AF_INET6, &gwAddr, dstStr6, INET6_ADDRSTRLEN) != nullptr)
        gateway = dstStr6;
      break;
    }
  }

  delete[] buf;

  return gateway;
}

// Some IN6_IFF_* address flags are only present in recent SDKs; provide
// fallbacks so the ranking compiles regardless of deployment target.
#ifndef IN6_IFF_TENTATIVE
#define IN6_IFF_TENTATIVE 0x0002
#endif
#ifndef IN6_IFF_DUPLICATED
#define IN6_IFF_DUPLICATED 0x0004
#endif
#ifndef IN6_IFF_DEPRECATED
#define IN6_IFF_DEPRECATED 0x0010
#endif
#ifndef IN6_IFF_AUTOCONF
#define IN6_IFF_AUTOCONF 0x0040
#endif
#ifndef IN6_IFF_TEMPORARY
#define IN6_IFF_TEMPORARY 0x0080
#endif
#ifndef IN6_IFF_OPTIMISTIC
#define IN6_IFF_OPTIMISTIC 0x0200
#endif
#ifndef IN6_IFF_SECURED
#define IN6_IFF_SECURED 0x0400
#endif

// Gets the highest-ranked permanent IPv6 address on the current interface.
// getifaddrs does not expose per-address state, so each candidate's flags are
// queried with SIOCGIFAFLAG_IN6 to filter out temporary/deprecated addresses
// and prefer stable ones, mirroring the Linux/Android behaviour.
std::string CNetworkInterfaceMacOS::GetCurrentIPv6Address() const
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

    // Remove anything that is not a usable permanent address. A tentative
    // address is still usable if it is optimistic (RFC 4429).
    int disqualify = IN6_IFF_DEPRECATED | IN6_IFF_DUPLICATED | IN6_IFF_TEMPORARY;
    if (!(flags & IN6_IFF_OPTIMISTIC))
      disqualify |= IN6_IFF_TENTATIVE;

    if (flags & disqualify)
      continue;

    // Rank the remaining permanent candidates (highest wins). These three
    // categories are exhaustive for an address that passed the filter above.
    // SECURED:   RFC 7217 stable-privacy address
    // !AUTOCONF: manually configured or DHCPv6 — explicitly assigned
    // AUTOCONF:  SLAAC parent (EUI-64, or the stable source of temporaries)
    unsigned int rank;
    if (flags & IN6_IFF_SECURED)
      rank = 3;
    else if (!(flags & IN6_IFF_AUTOCONF))
      rank = 2;
    else
      rank = 1;

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

bool CNetworkInterfaceMacOS::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  bool ret = false;
  size_t needed;
  constexpr std::array<int, 6> mib = {
      CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO,
  };

  mac = "";

  if (sysctl(const_cast<int*>(mib.data()), mib.size(), nullptr, &needed, nullptr, 0) == 0)
  {
    char* buf = static_cast<char*>(malloc(needed));
    if (buf)
    {
      if (sysctl(const_cast<int*>(mib.data()), mib.size(), buf, &needed, nullptr, 0) == 0)
      {
        struct rt_msghdr* rtm;
        for (char* next = buf; next < buf + needed; next += rtm->rtm_msglen)
        {

          rtm = reinterpret_cast<struct rt_msghdr*>(next);
          struct sockaddr_inarp* sin = reinterpret_cast<struct sockaddr_inarp*>(rtm + 1);
          struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(sin + 1);

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
  return std::make_unique<CNetworkMacOS>();
}

CNetworkMacOS::CNetworkMacOS() : CNetworkPosix()
{
  queryInterfaceList();
}

void CNetworkMacOS::GetMacAddress(const std::string& interfaceName, char rawMac[6])
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

void CNetworkMacOS::queryInterfaceList()
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
      m_interfaces.push_back(new CNetworkInterfaceMacOS(this, cur->ifa_name, macAddrRaw));
  }

  freeifaddrs(list);
}

namespace
{
// Query the active system resolver configuration and return every nameserver
// (both IPv4 and IPv6). Callers filter by address family.
//
// The dynamic store (State:/Network/Global/DNS) reflects the resolvers
// actually in use, including those supplied by DHCP. SCPreferences only
// exposes manually entered DNS, so it returns nothing on a typical
// DHCP-configured machine.
std::vector<std::string> QueryDNSServers()
{
  std::vector<std::string> result;
  @autoreleasepool
  {
    SCDynamicStoreRef store =
        SCDynamicStoreCreate(nullptr, CFSTR("GetDNSInfo"), nullptr, nullptr);
    if (!store)
    {
      CLog::Log(LOGWARNING, "Unable to determine nameservers");
      return result;
    }

    CFStringRef key = SCDynamicStoreKeyCreateNetworkGlobalEntity(
        nullptr, kSCDynamicStoreDomainState, kSCEntNetDNS);
    CFDictionaryRef dnsConfig =
        static_cast<CFDictionaryRef>(SCDynamicStoreCopyValue(store, key));
    CFRelease(key);

    if (dnsConfig)
    {
      NSDictionary* dnsSettings = (__bridge NSDictionary*)dnsConfig;
      NSArray* dnsServers = dnsSettings[static_cast<NSString*>(kSCPropNetDNSServerAddresses)];
      for (NSString* dnsServer in dnsServers)
      {
        result.emplace_back(dnsServer.UTF8String);
      }
      CFRelease(dnsConfig);
    }
    CFRelease(store);
  }

  return result;
}
} // namespace

std::vector<std::string> CNetworkMacOS::GetNameServers()
{
  std::vector<std::string> result;

  // a ':' only appears in IPv6 address literals
  for (const auto& server : QueryDNSServers())
  {
    if (server.find(':') == std::string::npos)
      result.emplace_back(server);
  }

  if (result.empty())
  {
    CLog::Log(LOGWARNING, "Unable to determine nameservers");
  }
  return result;
}

std::vector<std::string> CNetworkMacOS::GetIPv6NameServers()
{
  std::vector<std::string> result;

  for (const auto& server : QueryDNSServers())
  {
    if (server.find(':') != std::string::npos)
      result.emplace_back(server);
  }

  return result;
}

bool CNetworkMacOS::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  char cmd_line[64];

  struct in_addr host_ip;
  host_ip.s_addr = static_cast<in_addr_t>(remote_ip);

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
