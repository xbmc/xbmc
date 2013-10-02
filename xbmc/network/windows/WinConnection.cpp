#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "WinConnection.h"
#include "xbmc/utils/StdString.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

CWinConnection::CWinConnection(IP_ADAPTER_INFO adapter)
{
  m_adapter = adapter;
  GetNameServerInternal();
}

CWinConnection::~CWinConnection()
{
}

std::string CWinConnection::GetName() const
{
  return m_adapter.Description;
}

std::string CWinConnection::GetAddress() const
{
  return m_adapter.IpAddressList.IpAddress.String;
}

std::string CWinConnection::GetNetmask() const
{
  return m_adapter.IpAddressList.IpMask.String;
}

std::string CWinConnection::GetGateway() const
{
  return m_adapter.GatewayList.IpAddress.String;
}

std::string CWinConnection::GetNameServer() const
{
  return m_nameserver;
}

std::string CWinConnection::GetMacAddress() const
{
  std::string result;
  result = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", m_adapter.Address[0], m_adapter.Address[1], m_adapter.Address[2], m_adapter.Address[3], m_adapter.Address[4], m_adapter.Address[5]);
  return result;
}

void CWinConnection::GetMacAddressRaw(char rawMac[6]) const
{
  memcpy(rawMac, m_adapter.Address, 6);
}

std::string CWinConnection::GetInterfaceName() const
{
  // fixme
  return "";
}

ConnectionType CWinConnection::GetType() const
{
  return NETWORK_CONNECTION_TYPE_WIRED;
}

ConnectionState CWinConnection::GetState() const
{
  CStdString strIP = m_adapter.IpAddressList.IpAddress.String;

  if (strIP != "0.0.0.0")
    return NETWORK_CONNECTION_STATE_CONNECTED;
  else
    return NETWORK_CONNECTION_STATE_DISCONNECTED;
}

IPConfigMethod CWinConnection::GetMethod() const
{
  return IP_CONFIG_DHCP;
}

unsigned int CWinConnection::GetStrength() const
{
  return 0;
}

EncryptionType CWinConnection::GetEncryption() const
{
  return NETWORK_CONNECTION_ENCRYPTION_NONE;
}

unsigned int CWinConnection::GetSpeed() const
{
  return 100;
}

bool CWinConnection::Connect(IPassphraseStorage *storage, const CIPConfig &ipconfig)
{
  return false;
}

void CWinConnection::GetNameServerInternal()
{
  m_nameserver = "127.0.0.1";

  FIXED_INFO *pFixedInfo;
  ULONG ulOutBufLen;
  //IP_ADDR_STRING *pIPAddr;

  pFixedInfo = (FIXED_INFO *) malloc(sizeof (FIXED_INFO));
  if (pFixedInfo == NULL)
  {
    CLog::Log(LOGERROR,"Error allocating memory needed to call GetNetworkParams");
    return;
  }
  ulOutBufLen = sizeof (FIXED_INFO);
  if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
  {
    free(pFixedInfo);
    pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
    if (pFixedInfo == NULL)
    {
      CLog::Log(LOGERROR,"Error allocating memory needed to call GetNetworkParams");
      return;
    }
  }

  if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR)
  {
    m_nameserver = pFixedInfo->DnsServerList.IpAddress.String;
    // keep if we want more than the first nameserver
    /*pIPAddr = pFixedInfo->DnsServerList.Next;
    while(pIPAddr)
    {
      result.push_back(pIPAddr->IpAddress.String);
      pIPAddr = pIPAddr->Next;
    }*/

  }
  free(pFixedInfo);

  return;
}