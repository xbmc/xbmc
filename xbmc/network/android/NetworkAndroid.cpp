/*
 *      Copyright (C) 2016 Christian Browet
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "NetworkAndroid.h"

#include <androidjni/ConnectivityManager.h>
#include <androidjni/InetAddress.h>
#include <androidjni/LinkAddress.h>
#include <androidjni/RouteInfo.h>
#include <androidjni/WifiManager.h>
#include <androidjni/WifiInfo.h>

#include "platform/android/activity/XBMCApp.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

CNetworkInterfaceAndroid::CNetworkInterfaceAndroid(CJNINetwork network, CJNILinkProperties lp, CJNINetworkInterface intf)
  : m_network(network)
  , m_lp(lp)
  , m_intf(intf)
{
  m_name = m_intf.getName();
}

std::vector<std::string> CNetworkInterfaceAndroid::GetNameServers()
{
  std::vector<std::string> ret;

  CJNIList<CJNIInetAddress> lia = m_lp.getDnsServers();
  for (int i=0; i < lia.size(); ++i)
  {
    ret.push_back(lia.get(i).getHostAddress());
  }

  return ret;
}

std::string& CNetworkInterfaceAndroid::GetName()
{
  return m_name;
}

bool CNetworkInterfaceAndroid::IsEnabled()
{
  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  CJNINetworkInfo ni = connman.getNetworkInfo(m_network);
  if (!ni)
    return false;

  return ni.isAvailable();
}

bool CNetworkInterfaceAndroid::IsConnected()
{
  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  CJNINetworkInfo ni = connman.getNetworkInfo(m_network);
  if (!ni)
    return false;

  return ni.isConnected();
}

bool CNetworkInterfaceAndroid::IsWireless()
{
  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  CJNINetworkInfo ni = connman.getNetworkInfo(m_network);
  if (!ni)
    return false;

  int type = ni.getType();
  return !(type == CJNIConnectivityManager::TYPE_ETHERNET || type == CJNIConnectivityManager::TYPE_DUMMY);
}

std::string CNetworkInterfaceAndroid::GetMacAddress()
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
    return (StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
                                      (uint8_t)interfaceMacAddrRaw[0],
                                      (uint8_t)interfaceMacAddrRaw[1],
                                      (uint8_t)interfaceMacAddrRaw[2],
                                      (uint8_t)interfaceMacAddrRaw[3],
                                      (uint8_t)interfaceMacAddrRaw[4],
                                      (uint8_t)interfaceMacAddrRaw[5]));
  }
  return "";
}

void CNetworkInterfaceAndroid::GetMacAddressRaw(char rawMac[6])
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

bool CNetworkInterfaceAndroid::GetHostMacAddress(unsigned long host_ip, std::string& mac)
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
      //  CLog::Log(LOGERROR, "%s - GetHostMacAddress/ioctl failed with errno (%d)", __FUNCTION__, errno);
      return false;
    }
  }
  else
    return false;

  struct sockaddr* res = &areq.arp_ha;
  mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t) res->sa_data[0], (uint8_t) res->sa_data[1], (uint8_t) res->sa_data[2],
    (uint8_t) res->sa_data[3], (uint8_t) res->sa_data[4], (uint8_t) res->sa_data[5]);

  for (int i=0; i<6; ++i)
    if (res->sa_data[i])
      return true;

  return false;
}

std::string CNetworkInterfaceAndroid::GetCurrentIPAddress()
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

std::string CNetworkInterfaceAndroid::GetCurrentNetmask()
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
  return StringUtils::Format("%lu.%lu.%lu.%lu", mask >> 24, (mask >> 16) & 0xFF, (mask >> 8) & 0xFF, mask & 0xFF);
}

std::string CNetworkInterfaceAndroid::GetCurrentDefaultGateway()
{
  CJNIList<CJNIRouteInfo> ris = m_lp.getRoutes();
  for (int i = 0; i < ris.size(); ++i)
  {
    CJNIRouteInfo ri = ris.get(i);
    if (!ri.isDefaultRoute())
      continue;

    CJNIInetAddress ia = ri.getGateway();
    std::vector<char> adr = ia.getAddress();
    return StringUtils::Format("%u.%u.%u.%u", adr[0], adr[1], adr[2], adr[3]);
  }
  return "";
}

std::string CNetworkInterfaceAndroid::GetCurrentWirelessEssId()
{
  std::string ret;

  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  CJNINetworkInfo ni = connman.getNetworkInfo(m_network);
  if (!ni)
    return "";

  if (ni.getType() == CJNIConnectivityManager::TYPE_WIFI)
  {
    CJNIWifiManager wm = CXBMCApp::getSystemService("wifi");
    if (wm.isWifiEnabled())
    {
      CJNIWifiInfo wi = wm.getConnectionInfo();
      ret = wi.getSSID();
    }
  }
  return ret;
}

std::vector<NetworkAccessPoint> CNetworkInterfaceAndroid::GetAccessPoints()
{
  // TODO
  return std::vector<NetworkAccessPoint>();
}

void CNetworkInterfaceAndroid::GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
  // Not implemented
}

void CNetworkInterfaceAndroid::SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
  // Not implemented
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

CNetworkAndroid::CNetworkAndroid()
{
  RetrieveInterfaces();
}

CNetworkAndroid::~CNetworkAndroid()
{
  for (auto intf : m_interfaces)
    delete intf;
  for (auto intf : m_oldInterfaces)
    delete intf;
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
  CSingleLock lock(m_refreshMutex);
  return m_interfaces;
}

CNetworkInterface* CNetworkAndroid::GetFirstConnectedInterface()
{
  CSingleLock lock(m_refreshMutex);

  for(CNetworkInterface* intf : m_interfaces)
  {
    if (intf->IsEnabled() && intf->IsConnected() && !intf->GetCurrentDefaultGateway().empty())
      return intf;
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

void CNetworkAndroid::SetNameServers(const std::vector<std::string>& nameServers)
{
  // Not implemented
}

bool CNetworkAndroid::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  char cmd_line [64];

  struct in_addr host_ip;
  host_ip.s_addr = remote_ip;

  sprintf(cmd_line, "ping -c 1 -w %d %s", timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));

  int status = system (cmd_line);

  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = %d, errno = %d : '%s'", status, errno, cmd_line);

  return result == 0;
}

void CNetworkAndroid::RetrieveInterfaces()
{
  CSingleLock lock(m_refreshMutex);

  // Cannot delete interfaces here, as there still might have references to it
  for (auto intf : m_oldInterfaces)
    delete intf;
  m_oldInterfaces = m_interfaces;
  m_interfaces.clear();

  CJNIConnectivityManager connman(CXBMCApp::getSystemService(CJNIContext::CONNECTIVITY_SERVICE));
  std::vector<CJNINetwork> networks = connman.getAllNetworks();

  for (auto n : networks)
  {
    CJNILinkProperties lp = connman.getLinkProperties(n);
    if (lp)
    {
      CJNINetworkInterface intf = CJNINetworkInterface::getByName(lp.getInterfaceName());
      if (xbmc_jnienv()->ExceptionCheck())
      {
        xbmc_jnienv()->ExceptionClear();
        CLog::Log(LOGERROR, "CNetworkAndroid::RetrieveInterfaces Cannot get interface by name: %s", lp.getInterfaceName().c_str());
        continue;
      }
      if (intf)
        m_interfaces.push_back(new CNetworkInterfaceAndroid(n, lp, intf));
      else
        CLog::Log(LOGERROR, "CNetworkAndroid::RetrieveInterfaces Cannot get interface by name: %s", lp.getInterfaceName().c_str());
    }
    else
      CLog::Log(LOGERROR, "CNetworkAndroid::RetrieveInterfaces Cannot get link properties for network: %s", n.toString().c_str());
  }
}
