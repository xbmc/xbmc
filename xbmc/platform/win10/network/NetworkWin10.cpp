/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "NetworkWin10.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/CharsetConverter.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <collection.h>
#include <errno.h>
#include <iphlpapi.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <ws2ipdef.h>

using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace KODI::PLATFORM::WINDOWS;

CNetworkInterfaceWin10::CNetworkInterfaceWin10(CNetworkWin10* network, Windows::Networking::Connectivity::ConnectionProfile^ profile)
{
  m_network = network;
  m_adapter = profile;
  m_adaptername = FromW(profile->ProfileName->Data());
}

CNetworkInterfaceWin10::~CNetworkInterfaceWin10(void)
{
  m_adapter = nullptr;
}

std::string& CNetworkInterfaceWin10::GetName(void)
{
  return m_adaptername;
}

bool CNetworkInterfaceWin10::IsWireless()
{
  WlanConnectionProfileDetails^ wlanConnectionProfileDetails = m_adapter->WlanConnectionProfileDetails;
  return wlanConnectionProfileDetails != nullptr;
}

bool CNetworkInterfaceWin10::IsEnabled()
{
  return true;
}

bool CNetworkInterfaceWin10::IsConnected()
{
  return m_adapter->GetNetworkConnectivityLevel() != NetworkConnectivityLevel::None;
}

std::string CNetworkInterfaceWin10::GetMacAddress()
{
  return "Unknown";
}

bool CNetworkInterfaceWin10::GetHostMacAddress(unsigned long host, std::string& mac)
{
  mac = "";
  return false;
}

void CNetworkInterfaceWin10::GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
}

void CNetworkInterfaceWin10::SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
}

std::vector<NetworkAccessPoint> CNetworkInterfaceWin10::GetAccessPoints(void)
{
  std::vector<NetworkAccessPoint> accessPoints;
  return accessPoints;
}

void CNetworkInterfaceWin10::GetMacAddressRaw(char rawMac[6])
{
}

std::string CNetworkInterfaceWin10::GetCurrentIPAddress(void)
{
  Platform::String^ ipAddress = L"0.0.0.0";
  std::string result;

  if (m_adapter->NetworkAdapter != nullptr)
  {
    auto  hostnames = NetworkInformation::GetHostNames();
    for (unsigned int i = 0; i < hostnames->Size; ++i)
    {
      auto hostname = hostnames->GetAt(i);
      if (hostname->Type != HostNameType::Ipv4)
      {
        continue;
      }

      if (hostname->IPInformation != nullptr && hostname->IPInformation->NetworkAdapter != nullptr)
      {
        if (hostname->IPInformation->NetworkAdapter->NetworkAdapterId == m_adapter->NetworkAdapter->NetworkAdapterId)
        {
          ipAddress = hostname->CanonicalName;
          break;
        }
      }
    }
  }

  result = FromW(ipAddress->Data());

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentNetmask(void)
{
  std::string result = "255.255.255.255";

  if (m_adapter->NetworkAdapter != nullptr)
  {
    auto  hostnames = NetworkInformation::GetHostNames();
    for (unsigned int i = 0; i < hostnames->Size; ++i)
    {
      auto hostname = hostnames->GetAt(i);
      if (hostname->Type != HostNameType::Ipv4)
      {
        continue;
      }

      if (hostname->IPInformation != nullptr && hostname->IPInformation->NetworkAdapter != nullptr)
      {
        if (hostname->IPInformation->NetworkAdapter->NetworkAdapterId == m_adapter->NetworkAdapter->NetworkAdapterId)
        {
          byte prefixLength = hostname->IPInformation->PrefixLength->Value;
          uint32_t mask = 0xFFFFFFFF << (32 - prefixLength);
          result = StringUtils::Format("%u.%u.%u.%u"
                                     , ((mask & 0xFF000000) >> 24)
                                     , ((mask & 0x00FF0000) >> 16)
                                     , ((mask & 0x0000FF00) >> 8)
                                     ,  (mask & 0x000000FF));
          break;
        }
      }
    }
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentWirelessEssId(void)
{
  std::string result = "";
  if (!IsWireless())
    return result;

  result = FromW(m_adapter->WlanConnectionProfileDetails->GetConnectedSsid()->Data());

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentDefaultGateway(void)
{
  return "";
}

CNetworkWin10::CNetworkWin10(CSettings &settings)
  : CNetwork(settings)
{
  queryInterfaceList();
  NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler([this](Platform::Object^) {
    CSingleLock lock(m_critSection);
    queryInterfaceList();
  });
}

CNetworkWin10::~CNetworkWin10(void)
{
  CleanInterfaceList();
}

void CNetworkWin10::CleanInterfaceList()
{
  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while(it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkWin10::GetInterfaceList(void)
{
  CSingleLock lock (m_critSection);
  return m_interfaces;
}

void CNetworkWin10::queryInterfaceList()
{
  CleanInterfaceList();

  auto connectionProfiles = NetworkInformation::GetConnectionProfiles();
  std::for_each(begin(connectionProfiles), end(connectionProfiles), [this](ConnectionProfile^ connectionProfile)
  {
    if (connectionProfile != nullptr && connectionProfile->GetNetworkConnectivityLevel() != NetworkConnectivityLevel::None)
    {
      m_interfaces.push_back(new CNetworkInterfaceWin10(this, connectionProfile));
    }
  });
}

std::vector<std::string> CNetworkWin10::GetNameServers(void)
{
  std::vector<std::string> result;

  const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  PIP_ADAPTER_ADDRESSES adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (adapterAddresses == nullptr)
    return result;

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IF_OPER_STATUS::IfOperStatusUp)
        continue;
      for (PIP_ADAPTER_DNS_SERVER_ADDRESS dnsAddress = adapter->FirstDnsServerAddress; dnsAddress; dnsAddress = dnsAddress->Next)
      {
        std::string strIp = "";

        char buffer[INET6_ADDRSTRLEN] = { 0 };
        struct sockaddr* sa = dnsAddress->Address.lpSockaddr;
        switch (sa->sa_family)
        {
        case AF_INET:
          inet_ntop(AF_INET, &(reinterpret_cast<const struct sockaddr_in*>(sa)->sin_addr), buffer, INET_ADDRSTRLEN);
          break;
        case AF_INET6:
          inet_ntop(AF_INET6, &(reinterpret_cast<const struct sockaddr_in6*>(sa)->sin6_addr), buffer, INET6_ADDRSTRLEN);
          break;
        }

        strIp = buffer;

        if (!strIp.empty())
          result.push_back(strIp);
      }
    }
  }
  free(adapterAddresses);

  return result;
}

void CNetworkWin10::SetNameServers(const std::vector<std::string>& nameServers)
{
  return;
}

bool CNetworkWin10::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  return false;
}
