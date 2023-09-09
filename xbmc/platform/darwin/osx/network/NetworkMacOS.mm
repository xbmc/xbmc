/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkmacOS.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <array>
#include <errno.h>

#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <poll.h>
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
  std::string result;

  FILE* pipe = popen("echo \"show State:/Network/Global/IPv4\" | scutil | grep Router", "r");
  usleep(100000);
  if (pipe)
  {
    std::string tmpStr;
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
    {
      tmpStr = buffer;
      if (tmpStr.length() >= 11)
        result = tmpStr.substr(11);
    }
    pclose(pipe);
  }
  if (result.empty())
    CLog::Log(LOGWARNING, "Unable to determine gateway");

  return result;
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
      m_interfaces.push_back(new CNetworkInterfaceMacOS(this, cur->ifa_name, macAddrRaw));
  }

  freeifaddrs(list);
}

std::vector<std::string> CNetworkMacOS::GetNameServers()
{
  __block std::vector<std::string> result;
  @autoreleasepool
  {
    // Create an SCPreferencesRef object for the system preferences
    SCPreferencesRef preferences = SCPreferencesCreate(nullptr, CFSTR("GetDNSInfo"), nullptr);
    if (!preferences)
    {
      CLog::Log(LOGWARNING, "Unable to determine nameservers");
      return result;
    }

    // Get the current DNS configuration from system preferences
    CFDictionaryRef currentDNSConfig =
        static_cast<CFDictionaryRef>(SCPreferencesGetValue(preferences, kSCPrefNetworkServices));
    if (currentDNSConfig)
    {
      NSDictionary* networkServices = (__bridge NSDictionary*)currentDNSConfig;
      [networkServices enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL* stop) {
        NSDictionary* service = networkServices[key];
        NSDictionary* dnsSettings = service[static_cast<NSString*>(kSCEntNetDNS)];
        NSArray* dnsServers = dnsSettings[static_cast<NSString*>(kSCPropNetDNSServerAddresses)];
        for (NSString* dnsServer in dnsServers)
        {
          result.emplace_back(dnsServer.UTF8String);
        }
      }];
      CFRelease(preferences);
    }
  }

  if (result.empty())
  {
    CLog::Log(LOGWARNING, "Unable to determine nameservers");
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
