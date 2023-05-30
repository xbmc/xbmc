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

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/sockio.h>
#include <sys/wait.h>

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

  struct ifaddrs* cur;
  for (cur = list; cur != NULL; cur = cur->ifa_next)
  {
    if (cur->ifa_addr->sa_family != AF_INET)
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
