/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkPosix.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <utility>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

CNetworkInterfacePosix::CNetworkInterfacePosix(CNetworkPosix* network,
                                               std::string interfaceName,
                                               char interfaceMacAddrRaw[6])
  : m_interfaceName(std::move(interfaceName)),
    m_interfaceMacAdr(StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                                          (uint8_t)interfaceMacAddrRaw[0],
                                          (uint8_t)interfaceMacAddrRaw[1],
                                          (uint8_t)interfaceMacAddrRaw[2],
                                          (uint8_t)interfaceMacAddrRaw[3],
                                          (uint8_t)interfaceMacAddrRaw[4],
                                          (uint8_t)interfaceMacAddrRaw[5]))
{
  m_network = network;
  memcpy(m_interfaceMacAddrRaw, interfaceMacAddrRaw, sizeof(m_interfaceMacAddrRaw));
}

bool CNetworkInterfacePosix::IsEnabled() const
{
  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
    return false;

  return ((ifr.ifr_flags & IFF_UP) == IFF_UP);
}

bool CNetworkInterfacePosix::IsConnected() const
{
  struct ifreq ifr;
  int zero = 0;
  memset(&ifr, 0, sizeof(struct ifreq));
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
    return false;

  // ignore loopback
  int iRunning = ((ifr.ifr_flags & IFF_RUNNING) && (!(ifr.ifr_flags & IFF_LOOPBACK)));
  if (!iRunning)
    return false;

  // accept the interface if it has a non-zero IPv4 address bound...
  if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) >= 0 &&
      0 != memcmp(ifr.ifr_addr.sa_data + sizeof(short), &zero, sizeof(int)))
    return true;

  // ...otherwise fall back to checking for a usable (non-link-local) IPv6 address,
  // so IPv6-only interfaces are still reported as connected
  return HasUsableIPv6Address();
}
bool CNetworkInterfacePosix::HasUsableIPv6Address() const
{
  struct ifaddrs* interfaces = nullptr;
  if (getifaddrs(&interfaces) != 0)
    return false;

  bool found = false;
  for (struct ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next)
  {
    if (iface->ifa_addr == nullptr || iface->ifa_addr->sa_family != AF_INET6 ||
        (iface->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING) ||
        m_interfaceName != iface->ifa_name)
      continue;

    const auto* addr6 = reinterpret_cast<const struct sockaddr_in6*>(iface->ifa_addr);
    if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr))
      continue;

    found = true;
    break;
  }

  freeifaddrs(interfaces);
  return found;
}

std::string CNetworkInterfacePosix::GetCurrentIPv4Address() const
{
  std::string result;

  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  ifr.ifr_addr.sa_family = AF_INET;
  if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) >= 0)
  {
    result = inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
  }

  return result;
}

std::string CNetworkInterfacePosix::GetCurrentNetmask() const
{
  std::string result;

  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  ifr.ifr_addr.sa_family = AF_INET;
  if (ioctl(m_network->GetSocket(), SIOCGIFNETMASK, &ifr) >= 0)
  {
    result = inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
  }

  return result;
}

std::string CNetworkInterfacePosix::GetCurrentIPv6Address() const
{
  std::string address;
  struct ifaddrs* interfaces = nullptr;

  if (getifaddrs(&interfaces) != 0)
    return address;

  for (struct ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next)
  {
    if (iface->ifa_name != m_interfaceName ||
        (iface->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING) ||
        iface->ifa_addr == nullptr || iface->ifa_addr->sa_family != AF_INET6)
      continue;

    char str6[INET6_ADDRSTRLEN];
    const struct sockaddr_in6* addr6 =
        reinterpret_cast<const struct sockaddr_in6*>(iface->ifa_addr);

    if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr) ||
        (inet_ntop(AF_INET6, &addr6->sin6_addr, str6, INET6_ADDRSTRLEN) == nullptr))
      continue;

    address = str6;
    break;
  }

  freeifaddrs(interfaces);

  return address;
}

std::string CNetworkInterfacePosix::GetMacAddress() const
{
  return m_interfaceMacAdr;
}

void CNetworkInterfacePosix::GetMacAddressRaw(char rawMac[6]) const
{
  memcpy(rawMac, m_interfaceMacAddrRaw, 6);
}

CNetworkPosix::CNetworkPosix() : CNetworkBase()
{
  m_sock = socket(AF_INET, SOCK_DGRAM, 0);
}

CNetworkPosix::~CNetworkPosix()
{
  if (m_sock != -1)
    close(CNetworkPosix::m_sock);

  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while (it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkPosix::GetInterfaceList()
{
  return m_interfaces;
}

//! @bug
//! Overwrite the GetFirstConnectedInterface and requery
//! the interface list if no connected device is found
//! this fixes a bug when no network is available after first start of xbmc
//! and the interface comes up during runtime
CNetworkInterface* CNetworkPosix::GetFirstConnectedInterface()
{
  CNetworkInterface* pNetIf = CNetworkBase::GetFirstConnectedInterface();

  // no connected Interfaces found? - requeryInterfaceList
  if (!pNetIf)
  {
    CLog::Log(LOGDEBUG, "{} no connected interface found - requery list", __FUNCTION__);
    queryInterfaceList();
    //retry finding a connected if
    pNetIf = CNetworkBase::GetFirstConnectedInterface();
  }

  return pNetIf;
}
