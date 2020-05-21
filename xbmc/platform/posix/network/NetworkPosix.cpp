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

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

CNetworkInterfacePosix::CNetworkInterfacePosix(CNetworkPosix* network,
                                               std::string interfaceName,
                                               char interfaceMacAddrRaw[6])
  : m_interfaceName(interfaceName),
    m_interfaceMacAdr(StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
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

  if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) < 0)
    return false;

  // return only interfaces which has ip address
  return iRunning && (0 != memcmp(ifr.ifr_addr.sa_data + sizeof(short), &zero, sizeof(int)));
}

std::string CNetworkInterfacePosix::GetCurrentIPAddress() const
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
    CLog::Log(LOGDEBUG, "%s no connected interface found - requery list", __FUNCTION__);
    queryInterfaceList();
    //retry finding a connected if
    pNetIf = CNetworkBase::GetFirstConnectedInterface();
  }

  return pNetIf;
}
