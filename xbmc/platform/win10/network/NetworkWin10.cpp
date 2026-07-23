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
  : m_adapterAddr(address)
{
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

std::string CNetworkInterfaceWin10::GetCurrentIPv4Address(void) const
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

// Gets the Highest Rank Permanent IPv6 Address on the current interface.
std::string CNetworkInterfaceWin10::GetCurrentIPv6Address(void) const
{
  std::string address;
  unsigned int bestRank = 0;

  for (PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress; address; address = address->Next)
  {
    if (address->Address.lpSockaddr->sa_family != AF_INET6)
      continue;

    // Only fully configured (DAD passed) addresses are usable.
    if (address->DadState != IpDadStatePreferred)
      continue;

    const auto* sa6 = reinterpret_cast<const struct sockaddr_in6*>(address->Address.lpSockaddr);

    // Only globally scoped addresses. link-local (fe80::/10), loopback and
    // multicast are skipped.
    if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) || IN6_IS_ADDR_LOOPBACK(&sa6->sin6_addr) ||
        IN6_IS_ADDR_MULTICAST(&sa6->sin6_addr))
      continue;

    // Distinguish the stable public address from an RFC 4941 temporary
    // (privacy) address. On Windows the only reliable signal is the
    // DNS-eligible flag: the stable, publishable address carries it while
    // temporary addresses do not. Note IP_ADAPTER_ADDRESS_TRANSIENT is NOT a
    // temporary privacy address - it marks a failover-cluster address - so it
    // must not be used here. The suffix origin cannot separate them either,
    // because Windows assigns both a randomized interface identifier by
    // default (both report IpSuffixOriginRandom).
    const bool dnsEligible = (address->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE) != 0;

    // Rank by suffix origin, preferring explicitly configured addresses.
    unsigned int rank;
    switch (address->SuffixOrigin)
    {
      case IpSuffixOriginManual:
        rank = 3; // statically configured
        break;
      case IpSuffixOriginDhcp:
      case IpSuffixOriginLinkLayerAddress:
        rank = 2; // DHCPv6 or EUI-64 SLAAC
        break;
      default:
        rank = 1; // randomized interface identifier or other
        break;
    }

    // A DNS-eligible (stable public) address must always win over a temporary
    // privacy address, whatever their suffix origins.
    if (dnsEligible)
      rank += 10;

    // Keep the highest rank.
    if (rank <= bestRank)
      continue;

    std::string str = CNetworkBase::GetIpStr(address->Address.lpSockaddr);
    if (str.empty())
      continue;

    address = str;
    bestRank = rank;
  }

  return address;
}

// Gets the IPv6 default gateway on the current interface. Windows does not
// expose per-gateway route metrics here, so the first IPv6 gateway is returned.
std::string CNetworkInterfaceWin10::GetCurrentIPv6DefaultGateway(void) const
{
  for (PIP_ADAPTER_GATEWAY_ADDRESS_LH gateway = m_adapterAddr->FirstGatewayAddress; gateway;
       gateway = gateway->Next)
  {
    if (gateway->Address.lpSockaddr->sa_family == AF_INET6)
      return CNetworkBase::GetIpStr(gateway->Address.lpSockaddr);
  }

  return "";
}

CNetworkWin10::CNetworkWin10() : CNetworkBase()
{
  queryInterfaceList();
  NetworkInformation::NetworkStatusChanged(
      [this](auto&&)
      {
        std::unique_lock lock(m_critSection);
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
  std::unique_lock lock(m_critSection);
  return m_interfaces;
}

CNetworkInterface* CNetworkWin10::GetFirstConnectedInterface()
{
  std::unique_lock lock(m_critSection);
  for (CNetworkInterface* intf : m_interfaces)
  {
    if (intf->IsEnabled() && intf->IsConnected() &&
        (!intf->GetCurrentDefaultGateway().empty() ||
         !intf->GetCurrentIPv6DefaultGateway().empty()))
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

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  m_adapterAddresses.resize(ulOutBufLen);
  auto adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(m_adapterAddresses.data());

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
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

std::vector<std::string> CNetworkWin10::GetIPv6NameServers(void)
{
  std::vector<std::string> result;

  // GetNetworkParams only reports IPv4 DNS servers, so query the adapter
  // addresses directly to collect the IPv6 ones.
  const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_INET6, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  std::vector<uint8_t> buffer(ulOutBufLen);
  auto adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

  if (GetAdaptersAddresses(AF_INET6, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IF_OPER_STATUS::IfOperStatusUp)
        continue;
      for (PIP_ADAPTER_DNS_SERVER_ADDRESS dnsAddress = adapter->FirstDnsServerAddress; dnsAddress; dnsAddress = dnsAddress->Next)
      {
        std::string strIp = CNetworkBase::GetIpStr(dnsAddress->Address.lpSockaddr);
        if (!strIp.empty())
          result.push_back(strIp);
      }
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
