/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkWin32.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <errno.h>
#include <mutex>

#include <IcmpAPI.h>
#include <Mstcpip.h>
#include <iphlpapi.h>
#include <netinet/in.h>
#include <ws2ipdef.h>

#include "PlatformDefs.h"

#pragma comment(lib, "Ntdll.lib")

namespace
{
constexpr auto MAC_LENGTH = 6; // fixed MAC length used in CNetworkInterface
}

CNetworkInterfaceWin32::CNetworkInterfaceWin32(const IP_ADAPTER_ADDRESSES& adapter)
  : m_adapter(adapter)
{
}

CNetworkInterfaceWin32::~CNetworkInterfaceWin32(void)
{
}

bool CNetworkInterfaceWin32::IsEnabled() const
{
  return true;
}

bool CNetworkInterfaceWin32::IsConnected() const
{
  return m_adapter.OperStatus == IF_OPER_STATUS::IfOperStatusUp;
}

std::string CNetworkInterfaceWin32::GetMacAddress() const
{
  if (m_adapter.PhysicalAddressLength < MAC_LENGTH)
  {
    CLog::LogF(LOGERROR, "MAC address length is to small: current {} bytes, required {} bytes",
               m_adapter.PhysicalAddressLength, MAC_LENGTH);
    return "";
  }

  const unsigned char* mAddr = m_adapter.PhysicalAddress;

  return StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mAddr[0], mAddr[1],
                             mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
}

void CNetworkInterfaceWin32::GetMacAddressRaw(char rawMac[6]) const
{
  size_t len =
      (m_adapter.PhysicalAddressLength > MAC_LENGTH) ? MAC_LENGTH : m_adapter.PhysicalAddressLength;
  memcpy(rawMac, m_adapter.PhysicalAddress, len);
}

std::string CNetworkInterfaceWin32::GetCurrentIPv4Address(void) const
{
  for (PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapter.FirstUnicastAddress; address;
       address = address->Next)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
      return CNetworkBase::GetIpStr(address->Address.lpSockaddr);
  }

  return "";
}

std::string CNetworkInterfaceWin32::GetCurrentNetmask(void) const
{
  for (PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapter.FirstUnicastAddress; address;
       address = address->Next)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
      return CNetworkBase::GetMaskByPrefixLength(address->OnLinkPrefixLength);
  }

  return "";
}

std::string CNetworkInterfaceWin32::GetCurrentDefaultGateway(void) const
{
  for (PIP_ADAPTER_GATEWAY_ADDRESS_LH gateway = m_adapter.FirstGatewayAddress; gateway;
       gateway = gateway->Next)
  {
    if (gateway->Address.lpSockaddr->sa_family == AF_INET)
      return CNetworkBase::GetIpStr(gateway->Address.lpSockaddr);
  }

  return "";
}

// Gets the Highest Rank Permanent IPv6 Address on the current interface.
std::string CNetworkInterfaceWin32::GetCurrentIPv6Address(void) const
{
  std::string address;
  unsigned int bestRank = 0;

  for (PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapter.FirstUnicastAddress; address; address = address->Next)
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
std::string CNetworkInterfaceWin32::GetCurrentIPv6DefaultGateway(void) const
{
  for (PIP_ADAPTER_GATEWAY_ADDRESS_LH gateway = m_adapter.FirstGatewayAddress; gateway;
       gateway = gateway->Next)
  {
    if (gateway->Address.lpSockaddr->sa_family == AF_INET6)
      return CNetworkBase::GetIpStr(gateway->Address.lpSockaddr);
  }

  return "";
}

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkWin32>();
}

CNetworkWin32::CNetworkWin32()
 : CNetworkBase()
{
  queryInterfaceList();
}

CNetworkWin32::~CNetworkWin32(void)
{
  CleanInterfaceList();
  m_netrefreshTimer.Stop();
}

void CNetworkWin32::CleanInterfaceList()
{
  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while(it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkWin32::GetInterfaceList(void)
{
  std::unique_lock lock(m_critSection);
  if(m_netrefreshTimer.GetElapsedSeconds() >= 5.0f)
    queryInterfaceList();

  return m_interfaces;
}

void CNetworkWin32::queryInterfaceList()
{
  CleanInterfaceList();
  m_netrefreshTimer.StartZero();

  const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  m_adapterAddresses.resize(ulOutBufLen);
  auto adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(m_adapterAddresses.data());

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IF_OPER_STATUS::IfOperStatusUp)
        continue;
      m_interfaces.push_back(new CNetworkInterfaceWin32(*adapter));
    }
  }
  else
    CLog::LogF(LOGDEBUG, "GetAdaptersAddresses() failed ...");
}

namespace
{
std::vector<std::string> GetNameServersForFamily(ULONG family)
{
  std::vector<std::string> result;

  const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(family, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  std::vector<uint8_t> buffer(ulOutBufLen);
  auto adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

  if (GetAdaptersAddresses(family, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
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
} // namespace

std::vector<std::string> CNetworkWin32::GetNameServers(void)
{
  return GetNameServersForFamily(AF_INET);
}

std::vector<std::string> CNetworkWin32::GetIPv6NameServers(void)
{
  return GetNameServersForFamily(AF_INET6);
}

bool CNetworkWin32::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  struct sockaddr sockHost;
  sockHost.sa_family = AF_INET;
  reinterpret_cast<struct sockaddr_in&>(sockHost).sin_addr.S_un.S_addr = host;
  return PingHost(sockHost, timeout_ms);
}

bool CNetworkWin32::PingHost(const struct sockaddr& host, unsigned int timeout_ms /* = 2000 */)
{
  char SendData[]    = "poke";
  BYTE ReplyBuffer [sizeof(ICMP_ECHO_REPLY) + sizeof(SendData)];

  SetLastError(ERROR_SUCCESS);

  HANDLE hIcmpFile;
  DWORD dwRetVal;

  switch (host.sa_family)
  {
    case AF_INET:
      hIcmpFile = IcmpCreateFile();
      dwRetVal = IcmpSendEcho2(hIcmpFile, nullptr, nullptr, nullptr, reinterpret_cast<const struct sockaddr_in&>(host).sin_addr.S_un.S_addr, SendData, sizeof(SendData), nullptr, ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);
      break;

    case AF_INET6:
    {
      hIcmpFile = Icmp6CreateFile();
      struct sockaddr_in6 source = { AF_INET6, 0, 0, in6addr_any };
      dwRetVal = Icmp6SendEcho2(hIcmpFile, nullptr, nullptr, nullptr, &source, &const_cast<struct sockaddr_in6&>(reinterpret_cast<const struct sockaddr_in6&>(host)), SendData, sizeof(SendData), nullptr, ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);
      break;
    }

    default:
      return false;
  }

  DWORD lastErr = GetLastError();
  if (lastErr != ERROR_SUCCESS && lastErr != IP_REQ_TIMED_OUT)
    CLog::LogF(LOGERROR, "{} failed - {}",
               host.sa_family == AF_INET ? "IcmpSendEcho2" : "Icmp6SendEcho2",
               CWIN32Util::WUSysMsg(lastErr));

  IcmpCloseHandle (hIcmpFile);

  if (dwRetVal > 0U)
  {
    PICMP_ECHO_REPLY pEchoReply = reinterpret_cast<PICMP_ECHO_REPLY>(ReplyBuffer);
    return pEchoReply->Status == IP_SUCCESS;
  }
  return false;
}

bool CNetworkInterfaceWin32::GetHostMacAddress(unsigned long host, std::string& mac) const
{
  sockaddr sockHost{};
  sockHost.sa_family = AF_INET;
  reinterpret_cast<struct sockaddr_in&>(sockHost).sin_addr.S_un.S_addr = host;
  return GetHostMacAddress(&sockHost, mac);
}

bool CNetworkInterfaceWin32::GetHostMacAddress(struct sockaddr* host, std::string& mac) const
{
  DWORD InterfaceIndex;
  if (GetBestInterfaceEx(host, &InterfaceIndex) != NO_ERROR)
    return false;

  NET_LUID luid{};
  if (ConvertInterfaceIndexToLuid(InterfaceIndex, &luid) != NO_ERROR)
    return false;

  MIB_IPNET_ROW2 neighborIp{};
  neighborIp.InterfaceLuid = luid;
  switch (host->sa_family)
  {
    case AF_INET:
      memcpy(&neighborIp.Address.Ipv4, host, sizeof(sockaddr_in));
      break;
    case AF_INET6:
      memcpy(&neighborIp.Address.Ipv6, host, sizeof(sockaddr_in6));
      break;
    default:
      return false;
  }

  DWORD dwRetVal = GetIpNetEntry2(&neighborIp);

  if (dwRetVal != NO_ERROR)
  {
    CLog::LogF(LOGDEBUG, "Host not found in the cache (error {}), resolve the address.", dwRetVal);
    dwRetVal = ResolveIpNetEntry2(&neighborIp, nullptr);
  }

  if (dwRetVal != NO_ERROR)
  {
    CLog::LogF(LOGERROR, "ResolveIpNetEntry2 failed with error ({})", dwRetVal);
    return false;
  }

  if (neighborIp.PhysicalAddressLength < MAC_LENGTH)
  {
    CLog::LogF(LOGERROR,
               "ResolveIpNetEntry2 completed successfully, but mac address has length < {} ({})",
               MAC_LENGTH, neighborIp.PhysicalAddressLength);
    return false;
  }

  mac = StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                            neighborIp.PhysicalAddress[0], neighborIp.PhysicalAddress[1],
                            neighborIp.PhysicalAddress[2], neighborIp.PhysicalAddress[3],
                            neighborIp.PhysicalAddress[4], neighborIp.PhysicalAddress[5]);

  return true;
}
