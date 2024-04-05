/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "NetworkAndroid.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/android/activity/XBMCApp.h"

#include <mutex>

#include <androidjni/ConnectivityManager.h>
#include <androidjni/InetAddress.h>
#include <androidjni/LinkAddress.h>
#include <androidjni/NetworkInfo.h>
#include <androidjni/RouteInfo.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/wait.h>

CNetworkInterfaceAndroid::CNetworkInterfaceAndroid(const CJNINetwork& network,
                                                   const CJNILinkProperties& lp,
                                                   const CJNINetworkInterface& intf)
  : m_network(network), m_lp(lp), m_intf(intf)
{
  m_name = m_intf.getName();
}

std::vector<std::string> CNetworkInterfaceAndroid::GetNameServers()
{
  std::vector<std::string> ret;

  CJNIList<CJNIInetAddress> lia = m_lp.getDnsServers();
  ret.reserve(lia.size());
  for (int i=0; i < lia.size(); ++i)
  {
    ret.push_back(lia.get(i).getHostAddress());
  }

  return ret;
}

bool CNetworkInterfaceAndroid::IsEnabled() const
{
  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  CJNINetworkInfo ni = connman.getNetworkInfo(m_network);
  if (!ni)
    return false;

  return ni.isAvailable();
}

bool CNetworkInterfaceAndroid::IsConnected() const
{
  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  CJNINetworkInfo ni = connman.getNetworkInfo(m_network);
  if (!ni)
    return false;

  return ni.isConnected();
}

std::string CNetworkInterfaceAndroid::GetMacAddress() const
{
  auto interfaceMacAddrRaw = m_intf.getHardwareAddress();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionClear();
    CLog::Log(LOGERROR, "CNetworkInterfaceAndroid::GetMacAddress Exception getting HW address");
    return "";
  }
  if (interfaceMacAddrRaw.size() >= 6)
  {
    return (StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                                (uint8_t)interfaceMacAddrRaw[0], (uint8_t)interfaceMacAddrRaw[1],
                                (uint8_t)interfaceMacAddrRaw[2], (uint8_t)interfaceMacAddrRaw[3],
                                (uint8_t)interfaceMacAddrRaw[4], (uint8_t)interfaceMacAddrRaw[5]));
  }
  return "";
}

void CNetworkInterfaceAndroid::GetMacAddressRaw(char rawMac[6]) const
{
  auto interfaceMacAddrRaw = m_intf.getHardwareAddress();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionClear();
    CLog::Log(LOGERROR, "CNetworkInterfaceAndroid::GetMacAddress Exception getting HW address");
    return;
  }
  if (interfaceMacAddrRaw.size() >= 6)
    memcpy(rawMac, interfaceMacAddrRaw.data(), 6);
}

bool CNetworkInterfaceAndroid::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  struct arpreq areq;
  struct sockaddr_in* sin;

  memset(&areq, 0x0, sizeof(areq));

  sin = (struct sockaddr_in *) &areq.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = host_ip;

  sin = (struct sockaddr_in *) &areq.arp_ha;
  sin->sin_family = ARPHRD_ETHER;

  strncpy(areq.arp_dev, m_name.c_str(), sizeof(areq.arp_dev));
  areq.arp_dev[sizeof(areq.arp_dev)-1] = '\0';

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock != -1)
  {
    int result = ioctl (sock, SIOCGARP, (caddr_t) &areq);
    close(sock);

    if (result != 0)
    {
      //  CLog::Log(LOGERROR, "{} - GetHostMacAddress/ioctl failed with errno ({})", __FUNCTION__, errno);
      return false;
    }
  }
  else
    return false;

  struct sockaddr* res = &areq.arp_ha;
  mac = StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", (uint8_t)res->sa_data[0],
                            (uint8_t)res->sa_data[1], (uint8_t)res->sa_data[2],
                            (uint8_t)res->sa_data[3], (uint8_t)res->sa_data[4],
                            (uint8_t)res->sa_data[5]);

  for (int i=0; i<6; ++i)
    if (res->sa_data[i])
      return true;

  return false;
}

std::string CNetworkInterfaceAndroid::GetCurrentIPAddress() const
{
  CJNIList<CJNILinkAddress> lla = m_lp.getLinkAddresses();
  if (lla.size() == 0)
    return "";

  int i = 0;
  for (;i < lla.size(); ++i)
  {
    if (lla.get(i).getAddress().getAddress().size() > 4)  // IPV4 only
      continue;
    break;
  }
  if (i == lla.size())
    return "";

  CJNILinkAddress la = lla.get(i);
  return la.getAddress().getHostAddress();
}

std::string CNetworkInterfaceAndroid::GetCurrentNetmask() const
{
  CJNIList<CJNILinkAddress> lla = m_lp.getLinkAddresses();
  if (lla.size() == 0)
    return "";

  int i = 0;
  for (;i < lla.size(); ++i)
  {
    if (lla.get(i).getAddress().getAddress().size() > 4)  // IPV4 only
      continue;
    break;
  }
  if (i == lla.size())
    return "";

  CJNILinkAddress la = lla.get(i);

  int prefix = la.getPrefixLength();
  unsigned long mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
  return StringUtils::Format("{}.{}.{}.{}", mask >> 24, (mask >> 16) & 0xFF, (mask >> 8) & 0xFF,
                             mask & 0xFF);
}

std::string CNetworkInterfaceAndroid::GetCurrentDefaultGateway() const
{
  CJNIList<CJNIRouteInfo> ris = m_lp.getRoutes();

  for (int i = 0; i < ris.size(); ++i)
  {
    CJNIRouteInfo ri = ris.get(i);
    if (!ri.isDefaultRoute())
      continue;

    return ri.getGateway().getHostAddress();
  }

  return "";
}

std::string CNetworkInterfaceAndroid::GetHostName()
{
  CJNIList<CJNILinkAddress> lla = m_lp.getLinkAddresses();
  if (lla.size() == 0)
    return "";

  int i = 0;
  for (;i < lla.size(); ++i)
  {
    if (lla.get(i).getAddress().getAddress().size() > 4)  // IPV4 only
      continue;
    break;
  }
  if (i == lla.size())
    return "";

  CJNILinkAddress la = lla.get(i);
  return la.getAddress().getHostName();
}


/*************************/

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkAndroid>();
}

CNetworkAndroid::CNetworkAndroid() : CNetworkBase(), CJNIXBMCConnectivityManagerNetworkCallback()
{
  RetrieveInterfaces();

  CJNIConnectivityManager connman{CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE)};
  connman.registerDefaultNetworkCallback(this->get_raw());
}

CNetworkAndroid::~CNetworkAndroid()
{
  for (auto intf : m_interfaces)
    delete intf;
  for (auto intf : m_oldInterfaces)
    delete intf;

  CJNIConnectivityManager connman{CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE)};
  connman.unregisterNetworkCallback(this->get_raw());
}

bool CNetworkAndroid::GetHostName(std::string& hostname)
{
  CNetworkInterfaceAndroid* intf = dynamic_cast<CNetworkInterfaceAndroid*>(GetFirstConnectedInterface());
  if (intf)
  {
    hostname = intf->GetHostName();
    return true;
  }
  return false;
}

std::vector<CNetworkInterface*>& CNetworkAndroid::GetInterfaceList()
{
  std::unique_lock<CCriticalSection> lock(m_refreshMutex);
  return m_interfaces;
}

CNetworkInterface* CNetworkAndroid::GetFirstConnectedInterface()
{
  std::unique_lock<CCriticalSection> lock(m_refreshMutex);

  if (m_defaultInterface)
    return m_defaultInterface.get();
  else
  {
    for (CNetworkInterface* intf : m_interfaces)
    {
      if (intf->IsEnabled() && intf->IsConnected() && !intf->GetCurrentDefaultGateway().empty())
        return intf;
    }
  }

  return nullptr;
}

std::vector<std::string> CNetworkAndroid::GetNameServers()
{
  CNetworkInterfaceAndroid* intf = static_cast<CNetworkInterfaceAndroid*>(GetFirstConnectedInterface());
  if (intf)
    return intf->GetNameServers();

  return std::vector<std::string>();
}

bool CNetworkAndroid::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  char cmd_line [64];

  struct in_addr host_ip;
  host_ip.s_addr = remote_ip;

  snprintf(cmd_line, sizeof(cmd_line), "ping -c 1 -w %d %s",
           timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));

  int status = system (cmd_line);

  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = {}, errno = {} : '{}'", status, errno, cmd_line);

  return result == 0;
}

void CNetworkAndroid::RetrieveInterfaces()
{
  std::unique_lock<CCriticalSection> lock(m_refreshMutex);

  // Cannot delete interfaces here, as there still might have references to it
  for (auto intf : m_oldInterfaces)
    delete intf;
  m_oldInterfaces = m_interfaces;
  m_interfaces.clear();

  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  std::vector<CJNINetwork> networks = connman.getAllNetworks();

  for (const auto& n : networks)
  {
    CJNILinkProperties lp = connman.getLinkProperties(n);
    if (lp)
    {
      CJNINetworkInterface intf = CJNINetworkInterface::getByName(lp.getInterfaceName());
      if (xbmc_jnienv()->ExceptionCheck())
      {
        xbmc_jnienv()->ExceptionClear();
        CLog::Log(LOGERROR, "CNetworkAndroid::RetrieveInterfaces Cannot get interface by name: {}",
                  lp.getInterfaceName());
        continue;
      }
      if (intf)
        m_interfaces.push_back(new CNetworkInterfaceAndroid(n, lp, intf));
      else
        CLog::Log(LOGERROR, "CNetworkAndroid::RetrieveInterfaces Cannot get interface by name: {}",
                  lp.getInterfaceName());
    }
    else
      CLog::Log(LOGERROR,
                "CNetworkAndroid::RetrieveInterfaces Cannot get link properties for network: {}",
                n.toString());
  }
}

void CNetworkAndroid::onAvailable(const CJNINetwork n)
{
  CLog::Log(LOGDEBUG, "CNetworkAndroid::onAvailable The default network is now: {}", n.toString());

  CJNIConnectivityManager connman{CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE)};
  CJNILinkProperties lp = connman.getLinkProperties(n);

  if (lp)
  {
    CJNINetworkInterface intf = CJNINetworkInterface::getByName(lp.getInterfaceName());
    if (intf)
      m_defaultInterface = std::make_unique<CNetworkInterfaceAndroid>(n, lp, intf);
  }
}

void CNetworkAndroid::onLost(const CJNINetwork n)
{
  CLog::Log(LOGDEBUG, "CNetworkAndroid::onLost No default network (the last was: {})",
            n.toString());
  m_defaultInterface = nullptr;
}
