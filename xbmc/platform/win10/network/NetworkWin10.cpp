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
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/CharsetConverter.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <errno.h>
#include <iphlpapi.h>
#include <ppltasks.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <ws2ipdef.h>

#include <Ipexport.h>
#ifndef IP_STATUS_BASE

// --- c&p from Ipexport.h ----------------
typedef ULONG IPAddr;       // An IP address.

typedef struct ip_option_information {
  UCHAR   Ttl;                // Time To Live
  UCHAR   Tos;                // Type Of Service
  UCHAR   Flags;              // IP header flags
  UCHAR   OptionsSize;        // Size in bytes of options data
  _Field_size_bytes_(OptionsSize)
    PUCHAR  OptionsData;        // Pointer to options data
} IP_OPTION_INFORMATION, *PIP_OPTION_INFORMATION;

typedef struct icmp_echo_reply {
  IPAddr  Address;            // Replying address
  ULONG   Status;             // Reply IP_STATUS
  ULONG   RoundTripTime;      // RTT in milliseconds
  USHORT  DataSize;           // Reply data size in bytes
  USHORT  Reserved;           // Reserved for system use
  _Field_size_bytes_(DataSize)
    PVOID   Data;               // Pointer to the reply data
  struct ip_option_information Options; // Reply options
} ICMP_ECHO_REPLY, *PICMP_ECHO_REPLY;

#define IP_STATUS_BASE              11000
#define IP_SUCCESS                  0
#define IP_REQ_TIMED_OUT            (IP_STATUS_BASE + 10)

#endif //! IP_STATUS_BASE
#include <Icmpapi.h>

using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Connectivity;
using namespace KODI::PLATFORM::WINDOWS;

std::string GetIpStr(struct sockaddr* sa)
{
  std::string strIp = "";

  char buffer[INET6_ADDRSTRLEN] = { 0 };
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

  return strIp;
}

std::string GetMaskByPrefix(ADDRESS_FAMILY family, uint8_t prefix)
{
  std::string result = "";

  if (family == AF_INET6) // IPv6
  {
    if (prefix > 128) // invalid prefix
      return result;

    struct sockaddr_in6 sa;
    sa.sin6_family = AF_INET6;
    int i, j;

    memset(&sa.sin6_addr, 0x0, sizeof(sa.sin6_addr));
    for (i = prefix, j = 0; i > 0; i -= 8, j++)
    {
      if (i >= 8)
        sa.sin6_addr.s6_addr[j] = 0xff;
      else
        sa.sin6_addr.s6_addr[j] = (unsigned long)(0xffU << (8 - i));
    }
    result = GetIpStr(reinterpret_cast<struct sockaddr*>(&sa));
  }
  else // IPv4
  {
    if (prefix > 32) // invalid prefix
      return result;

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(~((1 << (32 - prefix)) - 1));;
    result = GetIpStr(reinterpret_cast<struct sockaddr*>(&sa));
  }

  return result;
}

CNetworkInterfaceWin10::CNetworkInterfaceWin10(CNetworkWin10 * network, const PIP_ADAPTER_ADDRESSES address, IUnknown* winRTadapter)
{
  m_network = network;
  m_adapterAddr = address;
  m_adaptername = address->AdapterName;
  winrt::attach_abi(m_winRT, winRTadapter);
  m_profile = nullptr;
}

CNetworkInterfaceWin10::~CNetworkInterfaceWin10(void) = default;

std::string& CNetworkInterfaceWin10::GetName(void)
{
  return m_adaptername;
}

bool CNetworkInterfaceWin10::IsWireless()
{
  return m_adapterAddr->IfType == IF_TYPE_IEEE80211;
}

bool CNetworkInterfaceWin10::IsEnabled()
{
  return true;
}

bool CNetworkInterfaceWin10::IsConnected()
{
  return m_adapterAddr->OperStatus == IF_OPER_STATUS::IfOperStatusUp;
}

std::string CNetworkInterfaceWin10::GetMacAddress()
{
  std::string result;
  unsigned char* mAddr = m_adapterAddr->PhysicalAddress;
  result = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", mAddr[0], mAddr[1], mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
  return result;
}

void CNetworkInterfaceWin10::GetMacAddressRaw(char rawMac[6])
{
  memcpy(rawMac, m_adapterAddr->PhysicalAddress, 6);
}

bool CNetworkInterfaceWin10::GetHostMacAddress(unsigned long host, std::string& mac)
{
  mac = "";
  //! @todo implement raw ARP requests
  return false;
}

void CNetworkInterfaceWin10::GetSettings(NetworkAssignment& assignment, std::string& ipAddress
                                       , std::string& networkMask, std::string& defaultGateway
                                       , std::string& essId, std::string& key, EncMode& encryptionMode)
{
  ipAddress = "0.0.0.0";
  networkMask = "0.0.0.0";
  defaultGateway = "0.0.0.0";
  essId = "";
  key = "";
  encryptionMode = ENC_NONE;
  assignment = NETWORK_DISABLED;

  const ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  PIP_ADAPTER_ADDRESSES adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (adapterAddresses == nullptr)
    return;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfIndex != m_adapterAddr->IfIndex)
        continue;

      if (adapter->Dhcpv4Enabled)
        assignment = NETWORK_DHCP;
      else
        assignment = NETWORK_STATIC;

      PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
      while (address)
      {
        if (address->Address.lpSockaddr->sa_family == AF_INET)
        {
          ipAddress = GetIpStr(address->Address.lpSockaddr);
          networkMask = GetMaskByPrefix(AF_INET, address->OnLinkPrefixLength);

          break;
        }
        address = address->Next;
      }

      PIP_ADAPTER_GATEWAY_ADDRESS_LH gwAddress = m_adapterAddr->FirstGatewayAddress;
      while (gwAddress)
      {
        if (gwAddress->Address.lpSockaddr->sa_family == AF_INET)
        {
          defaultGateway = GetIpStr(gwAddress->Address.lpSockaddr);
          break;
        }
        gwAddress = gwAddress->Next;
      }

      if (adapter->IfType == IF_TYPE_IEEE80211)
      {
        //! @todo get WLAN props
      }
      break;
    }
  }
  free(adapterAddresses);
}

void CNetworkInterfaceWin10::SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
}

std::vector<NetworkAccessPoint> CNetworkInterfaceWin10::GetAccessPoints(void)
{
  std::vector<NetworkAccessPoint> accessPoints;
  return accessPoints;
}

std::string CNetworkInterfaceWin10::GetCurrentIPAddress(void)
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = GetIpStr(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentNetmask(void)
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = GetMaskByPrefix(AF_INET, address->OnLinkPrefixLength);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentWirelessEssId(void)
{
  std::string result = "";
  if (!IsWireless() || !m_winRT)
    return result;

  if (IsConnected() && !m_profile)
  {
    m_profile = Wait(m_winRT.GetConnectedProfileAsync());
  }

  if (!m_profile)
    return result;

  if (!m_profile.IsWlanConnectionProfile())
    return result;

  auto ssid = m_profile.WlanConnectionProfileDetails().GetConnectedSsid();
  if (!ssid.empty())
    result = FromW(ssid.c_str());

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentDefaultGateway(void)
{
  std::string result = "";

  PIP_ADAPTER_GATEWAY_ADDRESS_LH address = m_adapterAddr->FirstGatewayAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = GetIpStr(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

CNetworkWin10::CNetworkWin10(CSettings &settings)
  : CNetworkBase(settings)
  , m_adapterAddresses(nullptr)
{
  queryInterfaceList();
  NetworkInformation::NetworkStatusChanged([this](auto&&) {
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
  free(m_adapterAddresses);
  m_adapterAddresses = nullptr;
}

std::vector<CNetworkInterface*>& CNetworkWin10::GetInterfaceList(void)
{
  CSingleLock lock (m_critSection);
  return m_interfaces;
}

CNetworkInterface* CNetworkWin10::GetFirstConnectedInterface()
{
  CSingleLock lock(m_critSection);
  for (CNetworkInterface* intf : m_interfaces)
  {
    if (intf->IsEnabled() && intf->IsConnected() && !intf->GetCurrentDefaultGateway().empty())
      return intf;
  }

  // fallback to default
  return CNetwork::GetFirstConnectedInterface();
}

void CNetworkWin10::queryInterfaceList()
{
  CleanInterfaceList();

  struct ci_less
  {
    // case-independent (ci) compare_less
    struct nocase_compare
    {
      bool operator() (const unsigned int& c1, const unsigned int& c2) const {
        return tolower(c1) < tolower(c2);
      }
    };
    bool operator()(const std::wstring & s1, const std::wstring & s2) const {
      return std::lexicographical_compare
      (s1.begin(), s1.end(),
       s2.begin(), s2.end(),
       nocase_compare());
    }
  };

  // collect all adapters from WinRT
  std::map<std::wstring, IUnknown*, ci_less> adapters;
  for (auto& profile : NetworkInformation::GetConnectionProfiles())
  {
    if (profile && profile.NetworkAdapter())
    {
      auto adapter = profile.NetworkAdapter();

      wchar_t* guidStr = nullptr;
      StringFromIID(adapter.NetworkAdapterId(), &guidStr);
      adapters[guidStr] = reinterpret_cast<IUnknown*>(winrt::detach_abi(adapter));

      CoTaskMemFree(guidStr);
    }
  }

  const ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
  ULONG ulOutBufLen;
  NetworkAdapter winRTAdapter = nullptr;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  m_adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (m_adapterAddresses == nullptr)
    return;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, m_adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = m_adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        continue;

      std::wstring name = ToW(adapter->AdapterName);
      m_interfaces.push_back(new CNetworkInterfaceWin10(this, adapter, adapters[name]));
      winRTAdapter = nullptr;
    }
  }
}

std::vector<std::string> CNetworkWin10::GetNameServers(void)
{
  std::vector<std::string> result;

  ULONG ulOutBufLen;
  if (GetNetworkParams(nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  PFIXED_INFO pInfo = static_cast<PFIXED_INFO>(malloc(ulOutBufLen));
  if (pInfo == nullptr)
    return result;

  if (GetNetworkParams(pInfo, &ulOutBufLen) == ERROR_SUCCESS)
  {
    PIP_ADDR_STRING addr = &pInfo->DnsServerList;
    while (addr)
    {
      std::string strIp = addr->IpAddress.String;
      if (!strIp.empty())
        result.push_back(strIp);

      addr = addr->Next;
    }
  }
  free(pInfo);

  return result;
}

void CNetworkWin10::SetNameServers(const std::vector<std::string>& nameServers)
{
  return;
}

bool CNetworkWin10::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  char SendData[] = "poke";
  HANDLE hIcmpFile = IcmpCreateFile();
  BYTE ReplyBuffer[sizeof(ICMP_ECHO_REPLY) + sizeof(SendData)];

  SetLastError(ERROR_SUCCESS);

  DWORD dwRetVal = IcmpSendEcho2(hIcmpFile, nullptr, nullptr, nullptr,
                                 host, SendData, sizeof(SendData), nullptr,
                                 ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);

  DWORD lastErr = GetLastError();
  if (lastErr != ERROR_SUCCESS && lastErr != IP_REQ_TIMED_OUT)
    CLog::LogF(LOGERROR, "IcmpSendEcho2 failed - {}", CWIN32Util::WUSysMsg(lastErr));

  IcmpCloseHandle(hIcmpFile);

  if (dwRetVal != 0)
  {
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
    return (pEchoReply->Status == IP_SUCCESS);
  }
  return false;
}
