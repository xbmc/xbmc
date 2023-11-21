/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "NetworkIOS.h"

#import "utils/StringUtils.h"
#import "utils/log.h"

#import "platform/darwin/ios-common/network/route.h"

#import <array>
#include <utility>

#import <arpa/inet.h>
#import <ifaddrs.h>
#import <net/if.h>
#import <net/if_dl.h>
#import <netdb.h>
#import <netinet/in.h>
#import <netinet6/in6.h>
#import <resolv.h>
#import <sys/ioctl.h>
#import <sys/socket.h>
#import <sys/sockio.h>
#import <sys/sysctl.h>

CNetworkInterfaceIOS::CNetworkInterfaceIOS(CNetworkIOS* network, std::string interfaceName)
  : m_interfaceName(std::move(interfaceName)), m_network(network)
{
}

CNetworkInterfaceIOS::~CNetworkInterfaceIOS() = default;

std::string CNetworkInterfaceIOS::GetInterfaceName() const
{
  return m_interfaceName;
}

bool CNetworkInterfaceIOS::IsEnabled() const
{
  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
    return false;

  return ((ifr.ifr_flags & IFF_UP) == IFF_UP);
}

bool CNetworkInterfaceIOS::IsConnected() const
{
  struct ifaddrs* interfaces = nullptr;

  if (getifaddrs(&interfaces) != 0)
    return false;

  for (struct ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next)
  {
    if (StringUtils::StartsWith(iface->ifa_name, m_interfaceName))
    {
      if ((iface->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING) &&
          iface->ifa_dstaddr != nullptr)
      {
        if (interfaces != nullptr)
          freeifaddrs(interfaces);
        return true;
      }
    }
  }

  if (interfaces != nullptr)
    freeifaddrs(interfaces);

  return false;
}

std::string CNetworkInterfaceIOS::GetMacAddress() const
{
  //! @todo Unable to retrieve MAC address of an interface/ARP table from ios11 onwards.
  return "";
}

void CNetworkInterfaceIOS::GetMacAddressRaw(char rawMac[6]) const
{
  //! @todo Unable to retrieve MAC address of an interface/ARP table from ios11 onwards.
  memset(&rawMac[0], 0, 6);
}

std::string CNetworkInterfaceIOS::GetCurrentIPAddress() const
{
  std::string address;
  struct ifaddrs* interfaces = nullptr;

  if (getifaddrs(&interfaces) != 0)
    return "";

  for (struct ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next)
  {
    if (StringUtils::StartsWith(iface->ifa_name, m_interfaceName))
    {
      if ((iface->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING) &&
          iface->ifa_dstaddr != nullptr)
      {
        switch (iface->ifa_addr->sa_family)
        {
          case AF_INET:
            char str4[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,
                      &((reinterpret_cast<struct sockaddr_in*>(iface->ifa_addr))->sin_addr), str4,
                      INET_ADDRSTRLEN);
            address = str4;
            break;
          case AF_INET6:
            char str6[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6,
                      &((reinterpret_cast<struct sockaddr_in6*>(iface->ifa_addr))->sin6_addr), str6,
                      INET6_ADDRSTRLEN);
            address = str6;
            break;
          default:
            break;
        }
      }
    }
  }

  if (interfaces != nullptr)
    freeifaddrs(interfaces);

  return address;
}

std::string CNetworkInterfaceIOS::GetCurrentNetmask() const
{
  std::string netmask;
  struct ifaddrs* interfaces = nullptr;

  if (getifaddrs(&interfaces) != 0)
    return "";

  for (struct ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next)
  {
    if (StringUtils::StartsWith(iface->ifa_name, m_interfaceName))
    {
      if ((iface->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING) &&
          iface->ifa_dstaddr != nullptr)
      {
        switch (iface->ifa_addr->sa_family)
        {
          case AF_INET:
            char mask4[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,
                      &((reinterpret_cast<struct sockaddr_in*>(iface->ifa_netmask))->sin_addr),
                      mask4, INET_ADDRSTRLEN);
            netmask = mask4;
            break;
          case AF_INET6:
            char mask6[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6,
                      &((reinterpret_cast<struct sockaddr_in6*>(iface->ifa_netmask))->sin6_addr),
                      mask6, INET6_ADDRSTRLEN);
            netmask = mask6;
            break;
          default:
            break;
        }
      }
    }
  }

  if (interfaces != nullptr)
    freeifaddrs(interfaces);

  return netmask;
}

std::string CNetworkInterfaceIOS::GetCurrentDefaultGateway() const
{
  std::string gateway;
  int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_FLAGS, RTF_GATEWAY};
  int afinet_type[] = {AF_INET, AF_INET6};

  for (int ip_type = 0; ip_type <= 1; ip_type++)
  {
    mib[3] = afinet_type[ip_type];

    size_t needed = 0;
    if (sysctl(mib, sizeof(mib) / sizeof(int), nullptr, &needed, nullptr, 0) < 0)
      return "";

    char* buf;
    if ((buf = new char[needed]) == 0)
      return "";

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
          sa = reinterpret_cast<struct sockaddr*>(
              reinterpret_cast<char*>(sa) +
              ((sa->sa_len) > 0 ? (1 + (((sa->sa_len) - 1) | (sizeof(long) - 1))) : sizeof(long)));
        }
        else
        {
          sa_tab[i] = nullptr;
        }
      }

      if (((rt->rtm_addrs & (RTA_DST | RTA_GATEWAY)) == (RTA_DST | RTA_GATEWAY)) &&
          sa_tab[RTAX_DST]->sa_family == afinet_type[ip_type] &&
          sa_tab[RTAX_GATEWAY]->sa_family == afinet_type[ip_type])
      {
        if (afinet_type[ip_type] == AF_INET)
        {
          if ((reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_DST]))->sin_addr.s_addr == 0)
          {
            char dstStr4[INET_ADDRSTRLEN];
            char srcStr4[INET_ADDRSTRLEN];
            memcpy(srcStr4,
                   &(reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_GATEWAY]))->sin_addr,
                   sizeof(struct in_addr));
            if (inet_ntop(AF_INET, srcStr4, dstStr4, INET_ADDRSTRLEN) != nullptr)
              gateway = dstStr4;
            break;
          }
        }
        else if (afinet_type[ip_type] == AF_INET6)
        {
          if ((reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_DST]))->sin_addr.s_addr == 0)
          {
            char dstStr6[INET6_ADDRSTRLEN];
            char srcStr6[INET6_ADDRSTRLEN];
            memcpy(srcStr6,
                   &(reinterpret_cast<struct sockaddr_in6*>(sa_tab[RTAX_GATEWAY]))->sin6_addr,
                   sizeof(struct in6_addr));
            if (inet_ntop(AF_INET6, srcStr6, dstStr6, INET6_ADDRSTRLEN) != nullptr)
              gateway = dstStr6;
            break;
          }
        }
      }
    }
    free(buf);
  }

  return gateway;
}

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkIOS>();
}

CNetworkIOS::CNetworkIOS() : CNetworkBase()
{
  m_sock = socket(AF_INET, SOCK_DGRAM, 0);
  queryInterfaceList();
}

CNetworkIOS::~CNetworkIOS()
{
  if (m_sock != -1)
    close(CNetworkIOS::m_sock);

  for (std::vector<CNetworkInterfaceIOS*>::iterator it = m_interfaces.begin();
       it != m_interfaces.end();)
  {
    CNetworkInterfaceIOS* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkIOS::GetInterfaceList()
{
  return reinterpret_cast<std::vector<CNetworkInterface*>&>(m_interfaces);
}

CNetworkInterface* CNetworkIOS::GetFirstConnectedInterface()
{
  // Renew m_interfaces to be able to handle hard interface changes (adapters removed/added)
  // This allows interfaces to be discovered if none are available at start eg. (Airplane mode on)
  queryInterfaceList();
  std::vector<CNetworkInterfaceIOS*>& ifaces = m_interfaces;

  CNetworkInterface* ifVPN = nullptr;
  CNetworkInterface* ifWired = nullptr;
  CNetworkInterface* ifWifi = nullptr;
  CNetworkInterface* ifCell = nullptr;

#if defined(TARGET_DARWIN_IOS)
  std::string ifWifiName = "en0";
  // Unsure interface number for lightning to ethernet adapter, need to confirm
  std::string ifWiredName = "en1";
#else // defined(TARGET_DARWIN_TVOS)
  std::string ifWifiName = "en1";
  std::string ifWiredName = "en0";
#endif

  for (auto iteriface : ifaces)
  {
    if (iteriface && iteriface->IsConnected())
    {
      // VPN interface
      if (StringUtils::StartsWith(iteriface->GetInterfaceName(), "utun"))
        ifVPN = static_cast<CNetworkInterface*>(iteriface);
      // Wired interface
      else if (StringUtils::StartsWith(iteriface->GetInterfaceName(), ifWiredName))
        ifWired = static_cast<CNetworkInterface*>(iteriface);
      // Wifi interface
      else if (StringUtils::StartsWith(iteriface->GetInterfaceName(), ifWifiName))
        ifWifi = static_cast<CNetworkInterface*>(iteriface);
      // Cellular interface
      else if (StringUtils::StartsWith(iteriface->GetInterfaceName(), "pdp_ip"))
        ifCell = static_cast<CNetworkInterface*>(iteriface);
    }
  }

  // Priority = VPN -> Wired -> Wifi -> Cell
  if (ifVPN != nullptr)
    return ifVPN;
  else if (ifWired != nullptr)
    return ifWired;
  else if (ifWifi != nullptr)
    return ifWifi;
  else if (ifCell != nullptr)
    return ifCell;
  else
    return nullptr;
}

void CNetworkIOS::queryInterfaceList()
{
  m_interfaces.clear();

  struct ifaddrs* list;
  if (getifaddrs(&list) < 0)
    return;

  for (struct ifaddrs* cur = list; cur != nullptr; cur = cur->ifa_next)
  {
    if (cur->ifa_addr->sa_family != AF_INET || (cur->ifa_flags & IFF_LOOPBACK) == IFF_LOOPBACK)
      continue;

    m_interfaces.push_back(new CNetworkInterfaceIOS(this, cur->ifa_name));
  }

  freeifaddrs(list);
}

std::vector<std::string> CNetworkIOS::GetNameServers()
{
  std::vector<std::string> nameServers;
  res_state res = static_cast<res_state>(malloc(sizeof(struct __res_state)));
  int result = res_ninit(res);

  if (result != 0)
  {
    CLog::Log(LOGERROR, "GetNameServers - no nameservers could be fetched (error {})", result);
    free(res);
    return nameServers;
  }

  union res_sockaddr_union servers[NI_MAXSERV];
  int serversFound = res_9_getservers(res, servers, NI_MAXSERV);

  for (int i = 0; i < serversFound; i++)
  {
    union res_sockaddr_union s = servers[i];
    if (s.sin.sin_len > 0)
    {
      char hostBuffer[NI_MAXHOST];
      if (getnameinfo(reinterpret_cast<struct sockaddr*>(&s.sin),
                      static_cast<socklen_t>(s.sin.sin_len), static_cast<char*>(hostBuffer),
                      sizeof(hostBuffer), nullptr, 0, NI_NUMERICHOST) == 0)
      {
        nameServers.emplace_back(hostBuffer);
      }
    }
  }

  res_ndestroy(res);
  return nameServers;
}

bool CNetworkIOS::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  /*! @todo ARP table is not accessible from iOS11 on. Was initially deprecated in iOS7
   *  WOL/WakeOnAccess can not work without MAC addresses, so was no need to implement
   *  this at this stage.
   */
  return false;
}

bool CNetworkInterfaceIOS::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  //! @todo Unable to retrieve MAC address of an interface/ARP table from ios11 onwards.
  return false;
}
