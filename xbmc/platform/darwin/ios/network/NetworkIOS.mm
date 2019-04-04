/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cstdlib>

#include "NetworkIOS.h"
#include "PlatformDefs.h"
#include "Util.h"
#include "platform/darwin/ios/network/ioshacks.h"
#include "platform/darwin/ios/network/route.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include <arpa/inet.h>
#include <dns.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>

CNetworkInterfaceIOS::CNetworkInterfaceIOS(CNetworkIOS* network,
                                           std::string interfaceName,
                                           char interfaceMacAddrRaw[6])
    : m_interfaceName(interfaceName)
    , m_interfaceMacAdr(StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
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

CNetworkInterfaceIOS::~CNetworkInterfaceIOS(void) = default;

const std::string& CNetworkInterfaceIOS::GetName(void) const
{
  return m_interfaceName;
}

bool CNetworkInterfaceIOS::IsWireless() const
{
  return true;
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
  struct ifreq ifr;
  int zero = 0;
  memset(&ifr, 0, sizeof(struct ifreq));
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
    return false;

  // ignore loopback
  int iRunning = ((ifr.ifr_flags & IFF_RUNNING) && (!(ifr.ifr_flags & IFF_LOOPBACK)));

  if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) < 0)
    return false;

  // return only interfaces which has ip address
  return iRunning && (0 != memcmp(ifr.ifr_addr.sa_data + sizeof(short), &zero, sizeof(int)));
}

std::string CNetworkInterfaceIOS::GetMacAddress() const
{
  return m_interfaceMacAdr;
}

void CNetworkInterfaceIOS::GetMacAddressRaw(char rawMac[6]) const
{
  memcpy(rawMac, m_interfaceMacAddrRaw, 6);
}

std::string CNetworkInterfaceIOS::GetCurrentIPAddress(void) const
{
  std::string result;

  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interfaceName.c_str());
  ifr.ifr_addr.sa_family = AF_INET;
  if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) >= 0)
  {
    result = inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
  }

  if (StringUtils::StartsWith(m_interfaceName, "pdp_ip"))
  {
    result += " (cellular)";
  }
  else if (StringUtils::StartsWith(m_interfaceName, "utun"))
  {
    result += " (vpn)";
  }
  else if (StringUtils::StartsWith(m_interfaceName, "en"))
  {
    result += " (wifi)";
  }

  return result;
}

std::string CNetworkInterfaceIOS::GetCurrentNetmask(void) const
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

std::string CNetworkInterfaceIOS::GetCurrentWirelessEssId(void) const
{
  std::string result;
  return result;
}

const struct sockaddr* GetSockAddr(const struct rt_msghdr2* rtm)
{
  //sockaddrs are after the message header
  const struct sockaddr* dst_sa = reinterpret_cast<const struct sockaddr*>(rtm + 1);

  if ((rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY)) == (RTA_DST | RTA_GATEWAY))
  {
    if (dst_sa[RTAX_DST].sa_family == AF_INET &&
        !((rtm->rtm_flags & RTF_WASCLONED) && (rtm->rtm_parentflags & RTF_PRCLONING)))
    {

      if (dst_sa[RTAX_GATEWAY].sa_family == AF_INET)
      {
        return &dst_sa[RTAX_GATEWAY];
      }
    }
  }
  return nullptr;
}

bool GetGatewayForIfName(std::string ifName, struct sockaddr** foundSockAddr)
{
  bool result = false;
  size_t needed = 0;
  int mib[6];
  char* buf = nullptr;
  char* next = nullptr;
  char* lim = nullptr;
  ;
  struct rt_msghdr2* rtm = nullptr;

  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_INET;
  mib[4] = NET_RT_FLAGS;
  mib[5] = RTF_GATEWAY;

  if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
  {
    CLog::Log(LOGERROR, "sysctl: net.route.0.0.dump estimate");
    return result;
  }

  if ((buf = new char[needed]) == 0)
  {
    CLog::Log(LOGERROR, "malloc(%lu)", (unsigned long)needed);
    return result;
  }
  if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
  {
    CLog::Log(LOGERROR, "sysctl: net.route.0.0.dump");
    delete[] buf;
    return result;
  }

  lim = buf + needed;

  for (next = buf; next < lim; next += rtm->rtm_msglen)
  {
    rtm = (struct rt_msghdr2*)next;
    char nameBuff[IF_NAMESIZE];
    std::string name = if_indextoname(rtm->rtm_index, nameBuff);
    int flagVal = 1 << RTAX_GATEWAY;

    if (name == ifName && rtm->rtm_addrs & flagVal)
    {
      const struct sockaddr* addr = GetSockAddr(rtm);
      if (addr != nullptr)
      {
        *foundSockAddr = const_cast<struct sockaddr*>(addr);
        result = true;
        break;
      }
    }
  }
  delete[] buf;

  return result;
}

std::string CNetworkInterfaceIOS::GetCurrentDefaultGateway(void) const
{
  std::string gateway;
  struct sockaddr* gatewayAdr;
  bool result = GetGatewayForIfName(GetName(), &gatewayAdr);

  if (!result)
  {
    gateway = "none";
  }
  else
  {
    struct sockaddr_in* si = reinterpret_cast<struct sockaddr_in*>(gatewayAdr);
    if (si->sin_addr.s_addr == INADDR_ANY)
    {
      gateway = "default";
    }
    else
    {
      gateway = inet_ntoa(si->sin_addr);
    }
  }

  return gateway;
}

CNetworkIOS::CNetworkIOS()
    : CNetworkBase()
{
  m_sock = socket(AF_INET, SOCK_DGRAM, 0);
  queryInterfaceList();
}

CNetworkIOS::~CNetworkIOS(void)
{
  if (m_sock != -1)
    close(CNetworkIOS::m_sock);

  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while (it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkIOS::GetInterfaceList(void)
{
  return m_interfaces;
}

CNetworkInterface* CNetworkIOS::GetFirstConnectedInterface(void)
{
  std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
  std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
  bool hasVPN = false;
  bool hasCellular = false;
  bool hasWifi = false;

  while (iter != ifaces.end())
  {
    CNetworkInterface* iface = *iter;
    if (iface && StringUtils::StartsWith(iface->GetName(), "utun"))
    {
      hasVPN = true;
    }
    if (iface && StringUtils::StartsWith(iface->GetName(), "en0"))
    {
      hasWifi = true;
    }
    if (iface && StringUtils::StartsWith(iface->GetName(), "pdp_ip"))
    {
      hasCellular = true;
    }
    ++iter;
  }

  iter = ifaces.begin();

  while (iter != ifaces.end())
  {
    CNetworkInterface* iface = *iter;
    if (iface && iface->IsConnected())
    {
      if (StringUtils::StartsWith(iface->GetName(), "en0") && !hasVPN)
        return iface;

      if (StringUtils::StartsWith(iface->GetName(), "pdp_ip") && !hasVPN && !hasWifi)
        return iface;

      if (StringUtils::StartsWith(iface->GetName(), "utun") && hasVPN)
        return iface;
    }
    ++iter;
  }

  return NULL;
}

void CNetworkIOS::GetMacAddress(const std::string& interfaceName, char rawMac[6])
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

void CNetworkIOS::queryInterfaceList()
{
  char macAddrRaw[6];
  m_interfaces.clear();

  // Query the list of interfaces.
  struct ifaddrs* list;
  if (getifaddrs(&list) < 0)
    return;

  struct ifaddrs* cur;
  for (cur = list; cur != NULL; cur = cur->ifa_next)
  {
    if (cur->ifa_addr->sa_family != AF_INET)
      continue;

    GetMacAddress(cur->ifa_name, macAddrRaw);

    // Add the interface.
    m_interfaces.push_back(new CNetworkInterfaceIOS(this, cur->ifa_name, macAddrRaw));
  }

  freeifaddrs(list);
}

std::vector<std::string> CNetworkIOS::GetNameServers(void)
{
  std::vector<std::string> nameServers;

  res_state res = static_cast<res_state>(malloc(sizeof(struct __res_state)));

  int result = res_ninit(res);

  if (result == 0)
  {
    for (int i = 0; i < res->nscount; i++)
    {
      std::string s = inet_ntoa(res->nsaddr_list[i].sin_addr);
      nameServers.push_back(s);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CNetworkIOS::GetNameServers - no nameservers could be fetched (error %d)",
              result);
  }

  free(res);
  return nameServers;
}

void CNetworkIOS::SetNameServers(const std::vector<std::string>& nameServers)
{
}

bool CNetworkIOS::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  return false;
}

bool CNetworkInterfaceIOS::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  bool ret = false;
  size_t needed;
  char *buf, *next;
  struct rt_msghdr* rtm;
  struct sockaddr_inarp* sin;
  struct sockaddr_dl* sdl;
  int mib[6];

  mac = "";

  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_INET;
  mib[4] = NET_RT_FLAGS;
  mib[5] = RTF_LLINFO;

  if (sysctl(mib, ARRAY_SIZE(mib), NULL, &needed, NULL, 0) == 0)
  {
    buf = (char*)malloc(needed);
    if (buf)
    {
      if (sysctl(mib, ARRAY_SIZE(mib), buf, &needed, NULL, 0) == 0)
      {
        for (next = buf; next < buf + needed; next += rtm->rtm_msglen)
        {

          rtm = (struct rt_msghdr*)next;
          sin = (struct sockaddr_inarp*)(rtm + 1);
          sdl = (struct sockaddr_dl*)(sin + 1);

          if (host_ip != sin->sin_addr.s_addr || sdl->sdl_alen < 6)
            continue;

          u_char* cp = (u_char*)LLADDR(sdl);

          mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3],
                                    cp[4], cp[5]);
          ret = true;
          break;
        }
      }
      free(buf);
    }
  }
  return ret;
}

std::vector<NetworkAccessPoint> CNetworkInterfaceIOS::GetAccessPoints(void) const
{
  std::vector<NetworkAccessPoint> result;
  return result;
}

void CNetworkInterfaceIOS::GetSettings(NetworkAssignment& assignment,
                                       std::string& ipAddress,
                                       std::string& networkMask,
                                       std::string& defaultGateway,
                                       std::string& essId,
                                       std::string& key,
                                       EncMode& encryptionMode) const
{
  ipAddress = "0.0.0.0";
  networkMask = "0.0.0.0";
  defaultGateway = "0.0.0.0";
  essId = "";
  key = "";
  encryptionMode = ENC_NONE;
  assignment = NETWORK_DISABLED;
}

void CNetworkInterfaceIOS::SetSettings(const NetworkAssignment& assignment,
                                       const std::string& ipAddress,
                                       const std::string& networkMask,
                                       const std::string& defaultGateway,
                                       const std::string& essId,
                                       const std::string& key,
                                       const EncMode& encryptionMode)
{
}
