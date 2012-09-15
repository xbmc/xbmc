#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "WinConnection.h"
#include "xbmc/utils/StdString.h"

CWinConnection::CWinConnection(IP_ADAPTER_INFO adapter)
{
  m_adapter = adapter;
}

CWinConnection::~CWinConnection()
{
}

std::string CWinConnection::GetName() const
{
  return adapter.Description;
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

std::string CPosixConnection::GetNameServer() const
{
  std::string nameserver("127.0.0.1");
  return nameserver;
}

std::string CWinConnection::GetMacAddress() const
{
  return std::string((char*)m_adapter.Address);
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

unsigned int CWinConnection::GetConnectionSpeed() const
{
  return 100;
}

bool CWinConnection::Connect(IPassphraseStorage *storage, const CIPConfig &ipconfig)
{
  return false;
}
