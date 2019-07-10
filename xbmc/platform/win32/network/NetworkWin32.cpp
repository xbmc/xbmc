/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkWin32.h"

#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <errno.h>

#include <IcmpAPI.h>
#include <Mstcpip.h>
#include <iphlpapi.h>
#include <netinet/in.h>

#include "PlatformDefs.h"

#pragma comment(lib, "Ntdll.lib")

CNetworkInterfaceWin32::CNetworkInterfaceWin32(const IP_ADAPTER_ADDRESSES& adapter)
{
  m_adapter = adapter;
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
  std::string result;
  const unsigned char* mAddr = m_adapter.PhysicalAddress;
  result = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", mAddr[0], mAddr[1], mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
  return result;
}

void CNetworkInterfaceWin32::GetMacAddressRaw(char rawMac[6]) const
{
  memcpy(rawMac, m_adapter.PhysicalAddress, 6);
}

std::string CNetworkInterfaceWin32::GetCurrentIPAddress(void) const
{
  return m_adapter.FirstUnicastAddress != nullptr ? CNetworkBase::GetIpStr(m_adapter.FirstUnicastAddress->Address.lpSockaddr) : "";
}

std::string CNetworkInterfaceWin32::GetCurrentNetmask(void) const
{
  if (m_adapter.FirstUnicastAddress->Address.lpSockaddr->sa_family == AF_INET)
    return CNetworkBase::GetMaskByPrefixLength(m_adapter.FirstUnicastAddress->OnLinkPrefixLength);

  return StringUtils::Format("%u", m_adapter.FirstUnicastAddress->OnLinkPrefixLength);
}

std::string CNetworkInterfaceWin32::GetCurrentDefaultGateway(void) const
{
  return m_adapter.FirstGatewayAddress != nullptr ? CNetworkBase::GetIpStr(m_adapter.FirstGatewayAddress->Address.lpSockaddr) : "";
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
  CSingleLock lock (m_critSection);
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

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  PIP_ADAPTER_ADDRESSES adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (adapterAddresses == nullptr)
    return;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IF_OPER_STATUS::IfOperStatusUp)
        continue;
      m_interfaces.push_back(new CNetworkInterfaceWin32(*adapter));
    }
  }
  else
    CLog::Log(LOGDEBUG, "%s - GetAdaptersAddresses() failed ...", __FUNCTION__);
}

std::vector<std::string> CNetworkWin32::GetNameServers(void)
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
        std::string strIp = CNetworkBase::GetIpStr(dnsAddress->Address.lpSockaddr);
        if (!strIp.empty())
          result.push_back(strIp);
      }
    }
  }
  free(adapterAddresses);

  return result;
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
    CLog::Log(LOGERROR, "%s - %s failed - %s", __FUNCTION__, host.sa_family == AF_INET ? "IcmpSendEcho2" : "Icmp6SendEcho2", CWIN32Util::WUSysMsg(lastErr).c_str());

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
  struct sockaddr sockHost;
  sockHost.sa_family = AF_INET;
  reinterpret_cast<struct sockaddr_in&>(sockHost).sin_addr.S_un.S_addr = host;
  return GetHostMacAddress(sockHost, mac);
}

bool CNetworkInterfaceWin32::GetHostMacAddress(const struct sockaddr& host, std::string& mac) const
{
  DWORD InterfaceIndex;
  if (GetBestInterfaceEx(&static_cast<struct sockaddr>(host), &InterfaceIndex) != NO_ERROR)
    return false;

  NET_LUID luid = { 0 };
  if (ConvertInterfaceIndexToLuid(InterfaceIndex, &luid) != NO_ERROR)
    return false;

  MIB_IPNET_ROW2 neighborIp = { 0 };
  neighborIp.InterfaceLuid = luid;
  neighborIp.InterfaceIndex;
  neighborIp.Address.si_family = host.sa_family;
  switch (host.sa_family)
  {
    case AF_INET:
      neighborIp.Address.Ipv4 = reinterpret_cast<const struct sockaddr_in&>(host);
      break;
    case AF_INET6:
      neighborIp.Address.Ipv6 = reinterpret_cast<const struct sockaddr_in6&>(host);
      break;
    default:
      return false;
  }

  DWORD dwRetVal = ResolveIpNetEntry2(&neighborIp, nullptr);
  if (dwRetVal == NO_ERROR)
  {
    if (neighborIp.PhysicalAddressLength == 6)
    {
      mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
        neighborIp.PhysicalAddress[0], neighborIp.PhysicalAddress[1], neighborIp.PhysicalAddress[2],
        neighborIp.PhysicalAddress[3], neighborIp.PhysicalAddress[4], neighborIp.PhysicalAddress[5]);
      return true;
    }
    else
      CLog::Log(LOGERROR, "%s - ResolveIpNetEntry2 completed successfully, but mac address has length != 6 (%d)", __FUNCTION__, neighborIp.PhysicalAddressLength);
  }
  else
    CLog::Log(LOGERROR, "%s - ResolveIpNetEntry2 failed with error (%d)", __FUNCTION__, dwRetVal);

  return false;
}
