/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkLinux.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <errno.h>
#include <utility>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <resolv.h>
#include <sys/ioctl.h>

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkPosix* network,
                                               std::string interfaceName,
                                               char interfaceMacAddrRaw[6])
  : CNetworkInterfacePosix(network, std::move(interfaceName), interfaceMacAddrRaw)
{
}

std::string CNetworkInterfaceLinux::GetCurrentDefaultGateway() const
{
  std::string result;

  FILE* fp = fopen("/proc/net/route", "r");
  if (!fp)
  {
    // TBD: Error
    return result;
  }

  char* line = NULL;
  char iface[16];
  char dst[128];
  char gateway[128];
  size_t linel = 0;
  int n;
  int linenum = 0;
  while (getdelim(&line, &linel, '\n', fp) > 0)
  {
    // skip first two lines
    if (linenum++ < 1)
      continue;

    // search where the word begins
    n = sscanf(line, "%15s %127s %127s", iface, dst, gateway);

    if (n < 3)
      continue;

    if (strcmp(iface, m_interfaceName.c_str()) == 0 && strcmp(dst, "00000000") == 0 &&
        strcmp(gateway, "00000000") != 0)
    {
      unsigned char gatewayAddr[4];
      int len = CNetworkBase::ParseHex(gateway, gatewayAddr);
      if (len == 4)
      {
        struct in_addr in;
        in.s_addr = (gatewayAddr[0] << 24) | (gatewayAddr[1] << 16) | (gatewayAddr[2] << 8) |
                    (gatewayAddr[3]);
        result = inet_ntoa(in);
        break;
      }
    }
  }
  free(line);
  fclose(fp);

  return result;
}

bool CNetworkInterfaceLinux::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  struct arpreq areq;
  struct sockaddr_in* sin;

  memset(&areq, 0x0, sizeof(areq));

  sin = (struct sockaddr_in*)&areq.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = host_ip;

  sin = (struct sockaddr_in*)&areq.arp_ha;
  sin->sin_family = ARPHRD_ETHER;

  strncpy(areq.arp_dev, m_interfaceName.c_str(), sizeof(areq.arp_dev));
  areq.arp_dev[sizeof(areq.arp_dev) - 1] = '\0';

  int result = ioctl(m_network->GetSocket(), SIOCGARP, (caddr_t)&areq);

  if (result != 0)
  {
    //  CLog::Log(LOGERROR, "%s - GetHostMacAddress/ioctl failed with errno (%d)", __FUNCTION__, errno);
    return false;
  }

  struct sockaddr* res = &areq.arp_ha;
  mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", (uint8_t)res->sa_data[0],
                            (uint8_t)res->sa_data[1], (uint8_t)res->sa_data[2],
                            (uint8_t)res->sa_data[3], (uint8_t)res->sa_data[4],
                            (uint8_t)res->sa_data[5]);

  for (int i = 0; i < 6; ++i)
    if (res->sa_data[i])
      return true;

  return false;
}

CNetworkLinux::CNetworkLinux() : CNetworkPosix()
{
  queryInterfaceList();
}

void CNetworkLinux::GetMacAddress(const std::string& interfaceName, char rawMac[6])
{
  memset(rawMac, 0, 6);

  struct ifreq ifr;
  strcpy(ifr.ifr_name, interfaceName.c_str());
  if (ioctl(GetSocket(), SIOCGIFHWADDR, &ifr) >= 0)
  {
    memcpy(rawMac, ifr.ifr_hwaddr.sa_data, 6);
  }
}

void CNetworkLinux::queryInterfaceList()
{
  char macAddrRaw[6];
  m_interfaces.clear();

  FILE* fp = fopen("/proc/net/dev", "r");
  if (!fp)
  {
    // TBD: Error
    return;
  }

  char* line = NULL;
  size_t linel = 0;
  int n;
  char* p;
  int linenum = 0;
  while (getdelim(&line, &linel, '\n', fp) > 0)
  {
    // skip first two lines
    if (linenum++ < 2)
      continue;

    // search where the word begins
    p = line;
    while (isspace(*p))
      ++p;

    // read word until :
    n = strcspn(p, ": \t");
    p[n] = 0;

    // save the result
    std::string interfaceName = p;
    GetMacAddress(interfaceName, macAddrRaw);

    // only add interfaces with non-zero mac addresses
    if (macAddrRaw[0] || macAddrRaw[1] || macAddrRaw[2] || macAddrRaw[3] || macAddrRaw[4] ||
        macAddrRaw[5])
      m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaceName, macAddrRaw));
  }
  free(line);
  fclose(fp);
}

std::vector<std::string> CNetworkLinux::GetNameServers()
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

bool CNetworkLinux::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  char cmd_line[64];

  struct in_addr host_ip;
  host_ip.s_addr = remote_ip;

  sprintf(cmd_line, "ping -c 1 -w %d %s", timeout_ms / 1000 + (timeout_ms % 1000) != 0,
          inet_ntoa(host_ip));

  int status = -1;
  status = system(cmd_line);
  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = %d, errno = %d : '%s'", status, errno, cmd_line);

  return result == 0;
}
