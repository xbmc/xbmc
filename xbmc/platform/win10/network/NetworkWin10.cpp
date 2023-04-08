/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkWin10.h"

#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/WIN32Util.h"

#include <errno.h>
#include <mutex>
#include <string.h>

#include <Ipexport.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <ppltasks.h>
#include <ws2ipdef.h>
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
#include <winrt/Windows.Networking.Connectivity.h>

namespace
{
constexpr int MAC_LENGTH = 6; // fixed MAC length used in CNetworkInterface
}

using namespace winrt::Windows::Networking::Connectivity;

CNetworkInterfaceWin10::CNetworkInterfaceWin10(const PIP_ADAPTER_ADDRESSES address)
{
  m_adapterAddr = address;
}

CNetworkInterfaceWin10::~CNetworkInterfaceWin10(void) = default;

bool CNetworkInterfaceWin10::IsEnabled() const
{
  return true;
}

bool CNetworkInterfaceWin10::IsConnected() const
{
  return m_adapterAddr->OperStatus == IF_OPER_STATUS::IfOperStatusUp;
}

std::string CNetworkInterfaceWin10::GetMacAddress() const
{
  if (m_adapterAddr->PhysicalAddressLength < MAC_LENGTH)
    return "";

  unsigned char* mAddr = m_adapterAddr->PhysicalAddress;
  return StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mAddr[0], mAddr[1],
                             mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
}

void CNetworkInterfaceWin10::GetMacAddressRaw(char rawMac[6]) const
{
  size_t len = (m_adapterAddr->PhysicalAddressLength > MAC_LENGTH)
                   ? MAC_LENGTH
                   : m_adapterAddr->PhysicalAddressLength;
  memcpy(rawMac, m_adapterAddr->PhysicalAddress, len);
}

bool CNetworkInterfaceWin10::GetHostMacAddress(unsigned long host, std::string& mac) const
{
  mac = "";
  //! @todo implement raw ARP requests
  return false;
}

std::string CNetworkInterfaceWin10::GetCurrentIPAddress(void) const
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = CNetworkBase::GetIpStr(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentNetmask(void) const
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = CNetworkBase::GetMaskByPrefixLength(address->OnLinkPrefixLength);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentDefaultGateway(void) const
{
  std::string result = "";

  PIP_ADAPTER_GATEWAY_ADDRESS_LH address = m_adapterAddr->FirstGatewayAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = CNetworkBase::GetIpStr(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

CNetworkWin10::CNetworkWin10() : CNetworkBase()
{
  queryInterfaceList();
  NetworkInformation::NetworkStatusChanged(
      [this](auto&&)
      {
        std::unique_lock<CCriticalSection> lock(m_critSection);
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_interfaces;
}

CNetworkInterface* CNetworkWin10::GetFirstConnectedInterface()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (CNetworkInterface* intf : m_interfaces)
  {
    if (intf->IsEnabled() && intf->IsConnected() && !intf->GetCurrentDefaultGateway().empty())
      return intf;
  }

  // fallback to default
  return CNetworkBase::GetFirstConnectedInterface();
}

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkWin10>();
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
  auto profiles = NetworkInformation::GetConnectionProfiles();
  for (const auto& profile : profiles)
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

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  m_adapterAddresses.resize(ulOutBufLen);
  auto adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(m_adapterAddresses.data());

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        continue;

      m_interfaces.push_back(new CNetworkInterfaceWin10(adapter));
    }
  }
}

std::vector<std::string> CNetworkWin10::GetNameServers(void)
{
  std::vector<std::string> result;

  ULONG ulOutBufLen;
  if (GetNetworkParams(nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  std::vector<uint8_t> buffer(ulOutBufLen);
  PFIXED_INFO pInfo = reinterpret_cast<PFIXED_INFO>(buffer.data());

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

  return result;
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
