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

#include <collection.h>
#include <errno.h>
#include "filesystem/SpecialProtocol.h"
#include <iphlpapi.h>
#include <string.h>
#include "PlatformDefs.h"
#include "platform/win32/WIN32Util.h"
#include "NetworkWin10.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"

#pragma pack(push, 8)

using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;

CNetworkInterfaceWin10::CNetworkInterfaceWin10(CNetworkWin10* network, Windows::Networking::Connectivity::ConnectionProfile^ profile)
{
  m_network = network;
  m_adapter = profile;
  g_charsetConverter.wToUTF8(std::wstring(profile->ProfileName->Data()), m_adaptername, false);
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

  g_charsetConverter.wToUTF8(std::wstring(ipAddress->Data()), result, false);

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentNetmask(void)
{
  return "";
}

std::string CNetworkInterfaceWin10::GetCurrentWirelessEssId(void)
{
  std::string result = "";
  if (!IsWireless())
    return result;

  auto ssid = m_adapter->WlanConnectionProfileDetails->GetConnectedSsid();
  g_charsetConverter.wToUTF8(std::wstring(ssid->Data()), result, false);
  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentDefaultGateway(void)
{
  return "";
}

CNetworkWin10::CNetworkWin10(void)
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
  return std::vector<std::string>();
}

void CNetworkWin10::SetNameServers(const std::vector<std::string>& nameServers)
{
  return;
}

bool CNetworkWin10::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  return false;
}

#pragma pack(pop)
